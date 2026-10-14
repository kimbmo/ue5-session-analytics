#include "SessionAnalyticsSubsystem.h"

#include "SessionAnalyticsSettings.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogSessionAnalytics, Log, All);

void USessionAnalyticsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (GetDefault<USessionAnalyticsSettings>()->bAutoStartSession)
	{
		StartSession();
	}
}

void USessionAnalyticsSubsystem::Deinitialize()
{
	EndSession(TEXT("Shutdown"));
	Super::Deinitialize();
}

void USessionAnalyticsSubsystem::StartSession()
{
	if (bSessionActive)
	{
		EndSession(TEXT("Restart"));
	}

	// Records can still be pending if the previous session's last write failed: retry before starting over.
	if (!PendingRecords.IsEmpty())
	{
		Flush();
		if (!PendingRecords.IsEmpty())
		{
			UE_LOG(LogSessionAnalytics, Warning, TEXT("Dropping %d unwritten events of session %s"), PendingRecords.Num(), *GetSessionId());
		}
	}

	const USessionAnalyticsSettings* Settings = GetDefault<USessionAnalyticsSettings>();

	SessionId = FGuid::NewGuid();
	SessionStartSeconds = FPlatformTime::Seconds();
	NextSequence = 0;
	DuplicateCount = 0;
	PendingRecords.Reset();
	LastRecordedTime.Reset();

	const FString FileName = FString::Printf(TEXT("%s_%s.csv"),
		*FDateTime::UtcNow().ToString(TEXT("%Y%m%d%H%M%S")), *SessionId.ToString(EGuidFormats::Digits));
	OutputFilePath = FPaths::Combine(FPaths::ProjectSavedDir(), Settings->OutputSubdirectory, FileName);

	if (!FFileHelper::SaveStringToFile(CsvHeader() + LINE_TERMINATOR, *OutputFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogSessionAnalytics, Error, TEXT("Could not create %s"), *OutputFilePath);
	}

	if (Settings->FlushIntervalSeconds > 0.f)
	{
		FlushTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &USessionAnalyticsSubsystem::OnFlushTick), Settings->FlushIntervalSeconds);
	}

	bSessionActive = true;
	RecordEvent(TEXT("session_start"), {});
	UE_LOG(LogSessionAnalytics, Log, TEXT("Session %s started -> %s"), *GetSessionId(), *OutputFilePath);
}

void USessionAnalyticsSubsystem::EndSession(const FString& Reason)
{
	if (!bSessionActive)
	{
		return;
	}

	TMap<FString, FString> Params;
	Params.Add(TEXT("reason"), Reason);
	Params.Add(TEXT("duplicates_suppressed"), FString::FromInt(DuplicateCount));
	RecordEvent(TEXT("session_end"), Params);

	FTSTicker::RemoveTicker(FlushTickerHandle);
	FlushTickerHandle.Reset();

	Flush();
	bSessionActive = false;
	UE_LOG(LogSessionAnalytics, Log, TEXT("Session %s ended (%s): %d events, %d duplicates"),
		*GetSessionId(), *Reason, NextSequence, DuplicateCount);
}

EAnalyticsRecordResult USessionAnalyticsSubsystem::RecordEvent(FName EventName, const TMap<FString, FString>& Params)
{
	if (EventName.IsNone())
	{
		UE_LOG(LogSessionAnalytics, Warning, TEXT("RecordEvent called with an empty name"));
		return EAnalyticsRecordResult::InvalidName;
	}
	if (!bSessionActive)
	{
		UE_LOG(LogSessionAnalytics, Warning, TEXT("'%s' ignored: no active session"), *EventName.ToString());
		return EAnalyticsRecordResult::NoSession;
	}

	const double Now = FPlatformTime::Seconds();
	const FString ParamsKey = SerializeParams(Params);
	if (IsDuplicate(EventName, ParamsKey, Now))
	{
		++DuplicateCount;
		return EAnalyticsRecordResult::Duplicate;
	}

	FAnalyticsEventRecord& Record = PendingRecords.AddDefaulted_GetRef();
	Record.Sequence = NextSequence++;
	Record.EventName = EventName;
	Record.Params = Params;
	Record.TimestampUtc = FDateTime::UtcNow();
	Record.ElapsedSeconds = static_cast<float>(Now - SessionStartSeconds);

	OnEventRecorded.Broadcast(Record);

	if (PendingRecords.Num() >= GetDefault<USessionAnalyticsSettings>()->FlushThreshold)
	{
		Flush();
	}
	return EAnalyticsRecordResult::Recorded;
}

void USessionAnalyticsSubsystem::Flush()
{
	PruneDuplicateHistory(FPlatformTime::Seconds());

	if (PendingRecords.IsEmpty() || OutputFilePath.IsEmpty())
	{
		return;
	}

	const FString Id = GetSessionId();
	FString Chunk;
	for (const FAnalyticsEventRecord& Record : PendingRecords)
	{
		Chunk += ToCsvLine(Id, Record);
		Chunk += LINE_TERMINATOR;
	}

	if (FFileHelper::SaveStringToFile(Chunk, *OutputFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
		&IFileManager::Get(), FILEWRITE_Append))
	{
		PendingRecords.Reset();
	}
	else
	{
		// Keep the records so the next flush can retry.
		UE_LOG(LogSessionAnalytics, Error, TEXT("Failed to append %d events to %s"), PendingRecords.Num(), *OutputFilePath);
	}
}

bool USessionAnalyticsSubsystem::OnFlushTick(float DeltaTime)
{
	Flush();
	return true;
}

bool USessionAnalyticsSubsystem::IsDuplicate(FName EventName, const FString& ParamsKey, double Now)
{
	const float Window = GetDefault<USessionAnalyticsSettings>()->DuplicateWindowSeconds;
	if (Window <= 0.f)
	{
		return false;
	}

	const FString Key = EventName.ToString() + TEXT("|") + ParamsKey;
	if (const double* Last = LastRecordedTime.Find(Key); Last && Now - *Last < Window)
	{
		return true;
	}
	LastRecordedTime.Add(Key, Now);
	return false;
}

void USessionAnalyticsSubsystem::PruneDuplicateHistory(double Now)
{
	const float Window = GetDefault<USessionAnalyticsSettings>()->DuplicateWindowSeconds;
	for (auto It = LastRecordedTime.CreateIterator(); It; ++It)
	{
		if (Now - It.Value() >= Window)
		{
			It.RemoveCurrent();
		}
	}
}

FString USessionAnalyticsSubsystem::CsvHeader()
{
	return TEXT("session_id,seq,timestamp_utc,elapsed_sec,event,params");
}

FString USessionAnalyticsSubsystem::ToCsvLine(const FString& InSessionId, const FAnalyticsEventRecord& Record)
{
	return FString::Printf(TEXT("%s,%d,%s,%.3f,%s,%s"),
		*EscapeCsv(InSessionId),
		Record.Sequence,
		*Record.TimestampUtc.ToIso8601(),
		Record.ElapsedSeconds,
		*EscapeCsv(Record.EventName.ToString()),
		*EscapeCsv(SerializeParams(Record.Params)));
}

FString USessionAnalyticsSubsystem::EscapeCsv(const FString& Value)
{
	if (Value.Contains(TEXT(",")) || Value.Contains(TEXT("\"")) || Value.Contains(TEXT("\n")) || Value.Contains(TEXT("\r")))
	{
		return TEXT("\"") + Value.Replace(TEXT("\""), TEXT("\"\"")) + TEXT("\"");
	}
	return Value;
}

FString USessionAnalyticsSubsystem::EscapeParam(const FString& Value)
{
	// Backslash first, so the escapes added below are not escaped again.
	return Value.Replace(TEXT("\\"), TEXT("\\\\"), ESearchCase::CaseSensitive)
		.Replace(TEXT(";"), TEXT("\\;"), ESearchCase::CaseSensitive)
		.Replace(TEXT("="), TEXT("\\="), ESearchCase::CaseSensitive);
}

FString USessionAnalyticsSubsystem::SerializeParams(const TMap<FString, FString>& Params)
{
	// Sorted so the same params always produce the same key/output.
	TArray<FString> Keys;
	Params.GetKeys(Keys);
	Keys.Sort();

	TArray<FString> Pairs;
	for (const FString& Key : Keys)
	{
		Pairs.Add(EscapeParam(Key) + TEXT("=") + EscapeParam(Params[Key]));
	}
	return FString::Join(Pairs, TEXT(";"));
}
