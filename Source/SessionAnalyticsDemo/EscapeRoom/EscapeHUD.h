#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "GameFramework/HUD.h"
#include "EscapeHUD.generated.h"

class AEscapeCharacter;
class AEscapeRoomManager;
class SWidget;

/**
 * Slate overlay: crosshair, interaction prompt, messages, puzzle status, hint counter (top right),
 * held-item icons (bottom left) and the Esc menu.
 */
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

	/** Rounded box brush owned by the HUD (Slate keeps raw pointers to brushes). */
	const FSlateBrush* MakeBrush(const FLinearColor& Fill, float Radius, const FLinearColor& Outline = FLinearColor::Transparent, float OutlineWidth = 0.f);

	/** Item icons drawn from rounded boxes, so the prototype needs no texture assets. */
	TSharedRef<SWidget> MakeKeyIcon(const FLinearColor& Color);
	TSharedRef<SWidget> MakePaletteIcon();
	TSharedRef<SWidget> MakePaintSlotIcon(int32 Slot);
	/** Item slot; Item names the icon that flashes right after pickup (None = never flashes). */
	TSharedRef<SWidget> MakeItem(TSharedRef<SWidget> Icon, const FText& Label, TFunction<bool()> IsVisible, FName Item = NAME_None);
	TSharedRef<SWidget> MakeMenu();

	TSharedPtr<SWidget> Overlay;
	/** Dark backing behind the top-left status block, so it reads as UI and not as writing on the ceiling. */
	FSlateRoundedBoxBrush PanelBrush = FSlateRoundedBoxBrush(FLinearColor(0.f, 0.f, 0.f, 0.55f), 6.f);
	TArray<TSharedRef<FSlateBrush>> Brushes;
	mutable TWeakObjectPtr<AEscapeRoomManager> CachedManager;
};
