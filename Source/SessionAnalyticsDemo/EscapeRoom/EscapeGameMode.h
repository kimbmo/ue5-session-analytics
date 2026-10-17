#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EscapeGameMode.generated.h"

/** Uses the first-person EscapeCharacter and the Slate EscapeHUD. */
UCLASS()
class AEscapeGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEscapeGameMode();
};
