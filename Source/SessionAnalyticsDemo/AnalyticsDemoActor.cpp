#include "AnalyticsDemoActor.h"

#include "SessionAnalyticsSubsystem.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"

void AAnalyticsDemoActor::BeginPlay()
{
	Super::BeginPlay();
	GetWorldTimerManager().SetTimer(Timer, this, &AAnalyticsDemoActor::FireNext, 0.3f, true);
}

void AAnalyticsDemoActor::FireNext()
{
	const UGameInstance* GameInstance = GetGameInstance();
	USessionAnalyticsSubsystem* Analytics = GameInstance ? GameInstance->GetSubsystem<USessionAnalyticsSubsystem>() : nullptr;
	if (!Analytics)
	{
		GetWorldTimerManager().ClearTimer(Timer);
		UE_LOG(LogTemp, Warning, TEXT("AnalyticsDemoActor: SessionAnalyticsSubsystem not available"));
		return;
	}

	struct FDemoEvent { const TCHAR* Name; const TCHAR* Key; const TCHAR* Value; };
	static const FDemoEvent Script[] = {
		{ TEXT("view_changed"), TEXT("view"), TEXT("exterior") },
		{ TEXT("color_selected"), TEXT("color"), TEXT("Red") },
		{ TEXT("color_selected"), TEXT("color"), TEXT("Red") },   // duplicate inside window
		{ TEXT("color_selected"), TEXT("color"), TEXT("Blue") },
		{ TEXT("view_changed"), TEXT("view"), TEXT("interior") },
	};

	if (Step >= UE_ARRAY_COUNT(Script))
	{
		GetWorldTimerManager().ClearTimer(Timer);
		const FString Path = Analytics->GetOutputFilePath();
		Analytics->EndSession(TEXT("DemoFinished"));
		UE_LOG(LogTemp, Display, TEXT("Demo done: %d recorded, %d duplicates -> %s"),
			Analytics->GetRecordedCount(), Analytics->GetDuplicateCount(), *Path);
		return;
	}

	const FDemoEvent& Event = Script[Step++];
	const EAnalyticsRecordResult Result = Analytics->RecordEvent(Event.Name, { { Event.Key, Event.Value } });
	UE_LOG(LogTemp, Display, TEXT("%s(%s=%s) -> %s"), Event.Name, Event.Key, Event.Value,
		*UEnum::GetValueAsString(Result));
}
