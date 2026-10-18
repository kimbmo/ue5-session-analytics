#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "GameFramework/HUD.h"
#include "EscapeHUD.generated.h"

class AEscapeCharacter;
class AEscapeRoomManager;
class SWidget;

/** Slate overlay: crosshair, interaction prompt, 2-second messages, puzzle/time, inventory and palette. */
UCLASS()
class AEscapeHUD : public AHUD
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	AEscapeRoomManager* GetManager() const;
	FText GetPrompt() const;
	FText GetMessage() const;
	FText GetHintMessage() const;
	const AEscapeCharacter* GetPlayerCharacter() const;

	TSharedPtr<SWidget> Overlay;
	/** Dark backing behind the top-left status block, so it reads as UI and not as writing on the ceiling. */
	FSlateRoundedBoxBrush PanelBrush = FSlateRoundedBoxBrush(FLinearColor(0.f, 0.f, 0.f, 0.55f), 6.f);
	mutable TWeakObjectPtr<AEscapeRoomManager> CachedManager;
};
