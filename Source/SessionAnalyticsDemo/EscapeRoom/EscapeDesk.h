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

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> Palette;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TArray<TObjectPtr<UStaticMeshComponent>> Legs;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void ApplyColors();

	bool bDrawerOpen = false;
	float DrawerAlpha = 0.f;
};
