#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "SessionAnalyticsSettings.h"
#include "SessionAnalyticsSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SessionAnalyticsTest
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** Lets a test change the settings CDO and puts the original values back when it goes out of scope. */
	struct FScopedSettings
	{
		USessionAnalyticsSettings* Settings = GetMutableDefault<USessionAnalyticsSettings>();
		const float Window = Settings->DuplicateWindowSeconds;
		const int32 Threshold = Settings->FlushThreshold;
		const float Interval = Settings->FlushIntervalSeconds;

		~FScopedSettings()
		{
			Settings->DuplicateWindowSeconds = Window;
			Settings->FlushThreshold = Threshold;
			Settings->FlushIntervalSeconds = Interval;
		}
	};

	USessionAnalyticsSubsystem* MakeSubsystem()
	{
		// UGameInstanceSubsystem is Within=GameInstance (and UGameInstance is Within=Engine), so give it a valid outer.
		UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
		return NewObject<USessionAnalyticsSubsystem>(GameInstance);
	}

	TArray<FString> ReadLines(const FString& Path)
	{
		TArray<FString> Lines;
		FFileHelper::LoadFileToStringArray(Lines, *Path);
		return Lines;
	}

	FString ParamsColumn(const TMap<FString, FString>& Params)
	{
		FAnalyticsEventRecord Record;
		Record.EventName = TEXT("e");
		Record.Params = Params;
		Record.TimestampUtc = FDateTime(2026, 10, 14);

		FString Line = USessionAnalyticsSubsystem::ToCsvLine(TEXT("S"), Record);
		Line.RemoveFromStart(TEXT("S,0,2026-10-14T00:00:00.000Z,0.000,e,"));
		return Line;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionAnalyticsCsvEscapeTest, "SessionAnalytics.CsvEscape", SessionAnalyticsTest::Flags)

bool FSessionAnalyticsCsvEscapeTest::RunTest(const FString& Parameters)
{
	FAnalyticsEventRecord Record;
	Record.Sequence = 3;
	Record.EventName = TEXT("menu_click");
	Record.Params.Add(TEXT("label"), TEXT("Save, \"now\""));
	Record.Params.Add(TEXT("a"), TEXT("1"));
	Record.TimestampUtc = FDateTime(2026, 10, 14);
	Record.ElapsedSeconds = 1.5f;

	const FString Line = USessionAnalyticsSubsystem::ToCsvLine(TEXT("S"), Record);
	TestEqual(TEXT("Params are sorted and quoted"), Line,
		FString(TEXT("S,3,2026-10-14T00:00:00.000Z,1.500,menu_click,\"a=1;label=Save, \"\"now\"\"\"")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionAnalyticsParamEscapeTest, "SessionAnalytics.ParamEscape", SessionAnalyticsTest::Flags)

bool FSessionAnalyticsParamEscapeTest::RunTest(const FString& Parameters)
{
	using namespace SessionAnalyticsTest;

	TestEqual(TEXT("Separators and backslash are escaped"),
		ParamsColumn({ { TEXT("k=1"), TEXT("a;b\\c") } }), FString(TEXT("k\\=1=a\\;b\\\\c")));

	// Without escaping these two would serialize to the same text.
	const FString OneParam = ParamsColumn({ { TEXT("a"), TEXT("1;b=2") } });
	const FString TwoParams = ParamsColumn({ { TEXT("a"), TEXT("1") }, { TEXT("b"), TEXT("2") } });
	TestEqual(TEXT("One param containing separators"), OneParam, FString(TEXT("a=1\\;b\\=2")));
	TestEqual(TEXT("Two plain params"), TwoParams, FString(TEXT("a=1;b=2")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionAnalyticsParamOrderTest, "SessionAnalytics.ParamOrderIndependence", SessionAnalyticsTest::Flags)

bool FSessionAnalyticsParamOrderTest::RunTest(const FString& Parameters)
{
	using namespace SessionAnalyticsTest;

	TMap<FString, FString> Forward;
	Forward.Add(TEXT("part"), TEXT("exterior"));
	Forward.Add(TEXT("color"), TEXT("Red"));

	TMap<FString, FString> Backward;
	Backward.Add(TEXT("color"), TEXT("Red"));
	Backward.Add(TEXT("part"), TEXT("exterior"));

	TestEqual(TEXT("Same output for either insertion order"), ParamsColumn(Forward), ParamsColumn(Backward));
	TestEqual(TEXT("Keys are sorted"), ParamsColumn(Forward), FString(TEXT("color=Red;part=exterior")));

	USessionAnalyticsSubsystem* Analytics = MakeSubsystem();
	Analytics->StartSession();
	const FString Path = Analytics->GetOutputFilePath();

	TestEqual(TEXT("First order"), Analytics->RecordEvent(TEXT("color_selected"), Forward), EAnalyticsRecordResult::Recorded);
	TestEqual(TEXT("Other order is the same event"), Analytics->RecordEvent(TEXT("color_selected"), Backward), EAnalyticsRecordResult::Duplicate);

	Analytics->EndSession(TEXT("Test"));
	IFileManager::Get().Delete(*Path);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionAnalyticsRecordTest, "SessionAnalytics.RecordAndDuplicate", SessionAnalyticsTest::Flags)

bool FSessionAnalyticsRecordTest::RunTest(const FString& Parameters)
{
	AddExpectedMessage(TEXT("'early' ignored: no active session"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);
	AddExpectedMessage(TEXT("RecordEvent called with an empty name"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);

	USessionAnalyticsSubsystem* Analytics = SessionAnalyticsTest::MakeSubsystem();

	TestEqual(TEXT("No session"), Analytics->RecordEvent(TEXT("early"), {}), EAnalyticsRecordResult::NoSession);

	Analytics->StartSession(); // records session_start as seq 0

	TMap<FString, FString> Params;
	Params.Add(TEXT("color"), TEXT("Red"));
	TestEqual(TEXT("First"), Analytics->RecordEvent(TEXT("color_selected"), Params), EAnalyticsRecordResult::Recorded);
	TestEqual(TEXT("Immediate repeat"), Analytics->RecordEvent(TEXT("color_selected"), Params), EAnalyticsRecordResult::Duplicate);

	Params.Add(TEXT("color"), TEXT("Blue"));
	TestEqual(TEXT("Different params"), Analytics->RecordEvent(TEXT("color_selected"), Params), EAnalyticsRecordResult::Recorded);
	TestEqual(TEXT("Empty name"), Analytics->RecordEvent(NAME_None, {}), EAnalyticsRecordResult::InvalidName);

	const FString Path = Analytics->GetOutputFilePath();
	Analytics->EndSession(TEXT("Test"));

	TestEqual(TEXT("Recorded count"), Analytics->GetRecordedCount(), 4); // start, Red, Blue, end
	TestEqual(TEXT("Duplicate count"), Analytics->GetDuplicateCount(), 1);

	TArray<FString> Lines;
	TestTrue(TEXT("CSV readable"), FFileHelper::LoadFileToStringArray(Lines, *Path));
	TestEqual(TEXT("Header + 4 rows"), Lines.Num(), 5);

	IFileManager::Get().Delete(*Path);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionAnalyticsWindowTest, "SessionAnalytics.RecordAgainAfterWindow", SessionAnalyticsTest::Flags)

bool FSessionAnalyticsWindowTest::RunTest(const FString& Parameters)
{
	SessionAnalyticsTest::FScopedSettings Scoped;
	Scoped.Settings->DuplicateWindowSeconds = 0.05f;

	USessionAnalyticsSubsystem* Analytics = SessionAnalyticsTest::MakeSubsystem();
	Analytics->StartSession();
	const FString Path = Analytics->GetOutputFilePath();

	TestEqual(TEXT("First"), Analytics->RecordEvent(TEXT("tap"), {}), EAnalyticsRecordResult::Recorded);
	TestEqual(TEXT("Inside window"), Analytics->RecordEvent(TEXT("tap"), {}), EAnalyticsRecordResult::Duplicate);

	FPlatformProcess::Sleep(0.15f);
	TestEqual(TEXT("After window"), Analytics->RecordEvent(TEXT("tap"), {}), EAnalyticsRecordResult::Recorded);

	Scoped.Settings->DuplicateWindowSeconds = 0.f;
	TestEqual(TEXT("Window 0 disables suppression"), Analytics->RecordEvent(TEXT("tap"), {}), EAnalyticsRecordResult::Recorded);

	Analytics->EndSession(TEXT("Test"));
	TestEqual(TEXT("Duplicate count"), Analytics->GetDuplicateCount(), 1);

	IFileManager::Get().Delete(*Path);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionAnalyticsFlushThresholdTest, "SessionAnalytics.FlushThreshold", SessionAnalyticsTest::Flags)

bool FSessionAnalyticsFlushThresholdTest::RunTest(const FString& Parameters)
{
	using namespace SessionAnalyticsTest;

	FScopedSettings Scoped;
	Scoped.Settings->FlushThreshold = 3;
	Scoped.Settings->FlushIntervalSeconds = 0.f; // only the threshold may write during this test

	USessionAnalyticsSubsystem* Analytics = MakeSubsystem();
	Analytics->StartSession(); // session_start is buffered
	const FString Path = Analytics->GetOutputFilePath();

	TestEqual(TEXT("Only the header before the threshold"), ReadLines(Path).Num(), 1);

	Analytics->RecordEvent(TEXT("one"), {});
	TestEqual(TEXT("Still buffered at 2 events"), ReadLines(Path).Num(), 1);

	Analytics->RecordEvent(TEXT("two"), {});
	TestEqual(TEXT("Written at 3 events"), ReadLines(Path).Num(), 4);

	Analytics->RecordEvent(TEXT("three"), {});
	TestEqual(TEXT("Next event is buffered again"), ReadLines(Path).Num(), 4);

	Analytics->Flush();
	TestEqual(TEXT("Manual flush writes it"), ReadLines(Path).Num(), 5);

	Analytics->EndSession(TEXT("Test"));
	TestEqual(TEXT("session_end is written on end"), ReadLines(Path).Num(), 6);

	IFileManager::Get().Delete(*Path);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSessionAnalyticsRestartTest, "SessionAnalytics.SessionRestart", SessionAnalyticsTest::Flags)

bool FSessionAnalyticsRestartTest::RunTest(const FString& Parameters)
{
	using namespace SessionAnalyticsTest;

	USessionAnalyticsSubsystem* Analytics = MakeSubsystem();
	Analytics->StartSession();
	const FString FirstPath = Analytics->GetOutputFilePath();
	const FString FirstId = Analytics->GetSessionId();
	Analytics->RecordEvent(TEXT("tap"), {});

	Analytics->StartSession(); // ends the first session with reason=Restart
	const FString SecondPath = Analytics->GetOutputFilePath();

	TestTrue(TEXT("Still active"), Analytics->IsSessionActive());
	TestNotEqual(TEXT("New session id"), Analytics->GetSessionId(), FirstId);
	TestNotEqual(TEXT("New file"), SecondPath, FirstPath);
	TestEqual(TEXT("Sequence restarts (only session_start so far)"), Analytics->GetRecordedCount(), 1);
	TestEqual(TEXT("Same event is not a duplicate in the new session"), Analytics->RecordEvent(TEXT("tap"), {}), EAnalyticsRecordResult::Recorded);

	const TArray<FString> FirstLines = ReadLines(FirstPath);
	TestEqual(TEXT("First file: header, start, tap, end"), FirstLines.Num(), 4);
	if (FirstLines.Num() == 4)
	{
		TestTrue(TEXT("First file belongs to the first session"), FirstLines[3].StartsWith(FirstId));
		TestTrue(TEXT("First session ended with reason=Restart"), FirstLines[3].EndsWith(TEXT("session_end,duplicates_suppressed=0;reason=Restart")));
	}

	Analytics->EndSession(TEXT("Test"));
	TestEqual(TEXT("Second file: header, start, tap, end"), ReadLines(SecondPath).Num(), 4);

	IFileManager::Get().Delete(*FirstPath);
	IFileManager::Get().Delete(*SecondPath);
	return true;
}

#endif
