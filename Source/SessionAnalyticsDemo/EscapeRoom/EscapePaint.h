#pragma once

#include "CoreMinimal.h"
#include "EscapeRoom/EscapeInteractable.h"
#include "EscapePaint.generated.h"

/** A paint can. With the palette, pressing E puts this color on the palette (puzzle 2). */
UCLASS()
class AEscapePaint : public AEscapeInteractable
{
	GENERATED_BODY()

public:
	AEscapePaint();

	virtual void Interact(AEscapeCharacter* Player, UPrimitiveComponent* HitComponent) override;

	/** Id used in the analytics input ("blue+yellow"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Paint")
	FString PaintId = TEXT("blue");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Paint")
	FLinearColor PaintColor = FLinearColor(0.05f, 0.15f, 0.8f);

	/** Short color name shown on the palette, e.g. 파랑. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Paint")
	FText ColorName;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> PaintTop;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
};
