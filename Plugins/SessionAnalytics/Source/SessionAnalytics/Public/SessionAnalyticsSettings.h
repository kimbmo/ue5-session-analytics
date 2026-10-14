#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SessionAnalyticsSettings.generated.h"

/** Project Settings > Plugins > Session Analytics */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Session Analytics"))
class SESSIONANALYTICS_API USessionAnalyticsSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USessionAnalyticsSettings() { CategoryName = TEXT("Plugins"); }

	/** Start a session automatically when the game instance starts. */
	UPROPERTY(Config, EditAnywhere, Category = "Session")
	bool bAutoStartSession = true;

	/** Same event with the same params inside this window (seconds) is treated as a duplicate. 0 disables. */
	UPROPERTY(Config, EditAnywhere, Category = "Events", meta = (ClampMin = "0.0"))
	float DuplicateWindowSeconds = 0.5f;

	/** Buffered events are written to disk once this many are queued. */
	UPROPERTY(Config, EditAnywhere, Category = "Output", meta = (ClampMin = "1"))
	int32 FlushThreshold = 20;

	/** Buffered events are also written to disk every this many seconds. 0 disables. Read when a session starts. */
	UPROPERTY(Config, EditAnywhere, Category = "Output", meta = (ClampMin = "0.0"))
	float FlushIntervalSeconds = 2.0f;

	/** Output folder, relative to the project's Saved directory. */
	UPROPERTY(Config, EditAnywhere, Category = "Output")
	FString OutputSubdirectory = TEXT("Analytics");
};
