#pragma once

#include "CoreMinimal.h"
#include "EscapeRoom/EscapeInteractable.h"
#include "EscapeDesk.generated.h"

/** Desk with a locked drawer. The drawer holds the palette (puzzle 2). Local +X is the drawer side. */
UCLASS()
class AEscapeDesk : public AEscapeInteractable
{
	GENERATED_BODY()

public:
	AEscapeDesk();

	virtual void Interact(AEscapeCharacter* Player, UPrimitiveComponent* HitComponent) override;
	virtual FText GetPrompt(const UPrimitiveComponent* HitComponent) const override;

	void OpenDrawer();
	bool IsDrawerOpen() const { return bDrawerOpen; }
	bool IsDrawerPart(const UPrimitiveComponent* Component) const;
	bool IsPalettePart(const UPrimitiveComponent* Component) const;
	/** v5: the palette is picked up with E and disappears from the drawer. */
	void TakePalette();
	bool IsPaletteInDrawer() const { return bPaletteInDrawer; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Desk")
	FText DrawerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Desk")
	float DrawerSlideDistance = 35.f;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> Drawer;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> DrawerHandle;

	/** Moves with the drawer; holds the palette board, thumb hole and paint blobs. */
	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<USceneComponent> PaletteRoot;

	/** The oval palette board. */
	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> Palette;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> PaletteThumbHole;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TArray<TObjectPtr<UStaticMeshComponent>> PaletteBlobs;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TArray<TObjectPtr<UStaticMeshComponent>> Legs;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void ApplyColors();
	TArray<UStaticMeshComponent*> GetPaletteParts() const;

	bool bDrawerOpen = false;
	bool bPaletteInDrawer = true;
	float DrawerAlpha = 0.f;
};
