#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SessionAnalyticsSubsystem.generated.h"

USTRUCT(BlueprintType)
struct SESSIONANALYTICS_API FAnalyticsEventRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Analytics")
	int32 Sequence = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Analytics")
	FName EventName;

	UPROPERTY(BlueprintReadOnly, Category = "Analytics")
	TMap<FString, FString> Params;

	UPROPERTY(BlueprintReadOnly, Category = "Analytics")
	FDateTime TimestampUtc;

	/** Seconds since the session started. */
	UPROPERTY(BlueprintReadOnly, Category = "Analytics")
	float ElapsedSeconds = 0.f;
};

UENUM(BlueprintType)
enum class EAnalyticsRecordResult : uint8
{
	Recorded,
	/** Same event + params arrived inside the duplicate window. */
	Duplicate,
	/** No session is running. */
	NoSession,
	InvalidName
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAnalyticsEventRecorded, const FAnalyticsEventRecord&, Record);

/**
 * Collects gameplay/UI events for one session and exports them as CSV.
 * Session start/end and event collection are separated so callers never
 * have to care about file I/O or ordering.
 */
UCLASS()
class SESSIONANALYTICS_API USessionAnalyticsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Analytics")
	void StartSession();

	/** Writes remaining events and closes the session. Safe to call twice. */
	UFUNCTION(BlueprintCallable, Category = "Analytics")
	void EndSession(const FString& Reason = TEXT("Manual"));

	UFUNCTION(BlueprintCallable, Category = "Analytics", meta = (AutoCreateRefTerm = "Params"))
	EAnalyticsRecordResult RecordEvent(FName EventName, const TMap<FString, FString>& Params);

	/** Writes buffered events to disk now. */
	UFUNCTION(BlueprintCallable, Category = "Analytics")
	void Flush();

	UFUNCTION(BlueprintPure, Category = "Analytics")
	bool IsSessionActive() const { return bSessionActive; }

	UFUNCTION(BlueprintPure, Category = "Analytics")
	FString GetSessionId() const { return SessionId.ToString(EGuidFormats::DigitsWithHyphens); }

	UFUNCTION(BlueprintPure, Category = "Analytics")
	FString GetOutputFilePath() const { return OutputFilePath; }

	UFUNCTION(BlueprintPure, Category = "Analytics")
	int32 GetRecordedCount() const { return NextSequence; }

	UFUNCTION(BlueprintPure, Category = "Analytics")
	int32 GetDuplicateCount() const { return DuplicateCount; }

	UPROPERTY(BlueprintAssignable, Category = "Analytics")
	FOnAnalyticsEventRecorded OnEventRecorded;

	/** Exposed for tests: builds one CSV line for a record. */
	static FString ToCsvLine(const FString& InSessionId, const FAnalyticsEventRecord& Record);
	static FString CsvHeader();

private:
	static FString EscapeCsv(const FString& Value);
	/** Backslash-escapes '\\', ';' and '=' so keys/values cannot be confused with the separators. */
	static FString EscapeParam(const FString& Value);
	static FString SerializeParams(const TMap<FString, FString>& Params);

	bool IsDuplicate(FName EventName, const FString& ParamsKey, double Now);
	/** Drops duplicate-history entries that are older than the window. */
	void PruneDuplicateHistory(double Now);
	bool OnFlushTick(float DeltaTime);

	/** Periodic flush. Core ticker, so it does not depend on a world. */
	FTSTicker::FDelegateHandle FlushTickerHandle;

	bool bSessionActive = false;
	FGuid SessionId;
	FString OutputFilePath;
	double SessionStartSeconds = 0.0;

	int32 NextSequence = 0;
	int32 DuplicateCount = 0;

	TArray<FAnalyticsEventRecord> PendingRecords;

	/** Last time each event+params key was recorded, for duplicate suppression. */
	TMap<FString, double> LastRecordedTime;
};
