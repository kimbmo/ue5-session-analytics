#pragma once

#include "CoreMinimal.h"
#include "EscapeRoom/EscapeInteractable.h"
#include "EscapeEasel.generated.h"

/** Easel with an unfinished canvas. Inspect-only; Mesh is the canvas. Local +X is the canvas front. */
UCLASS()
class AEscapeEasel : public AEscapeInteractable
{
	GENERATED_BODY()

public:
	AEscapeEasel();

	/** Colors the unpainted patch (called when puzzle 2 is solved). */
	void FillPatch(const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TArray<TObjectPtr<UStaticMeshComponent>> Legs;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TArray<TObjectPtr<UStaticMeshComponent>> PaintedParts;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> EmptyPatch;

	/** Border around the empty patch in the lamp's green: links "fill this" to "the lamp's colour". */
	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> EmptyPatchBorder;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Easel")
	FLinearColor PatchBorderColor = FLinearColor(0.05f, 0.75f, 0.12f);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	void ApplyColors();
	bool bFilled = false;
	FLinearColor FillColor = FLinearColor::White;
};
