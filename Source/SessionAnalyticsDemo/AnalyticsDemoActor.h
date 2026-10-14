#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AnalyticsDemoActor.generated.h"

/** Drop into a level and press Play: records a scripted set of events, including duplicates. */
UCLASS()
class AAnalyticsDemoActor : public AActor
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

private:
	void FireNext();

	FTimerHandle Timer;
	int32 Step = 0;
};
