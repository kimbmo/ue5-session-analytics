#pragma once

#include "CoreMinimal.h"
#include "EscapeRoom/EscapeInteractable.h"
#include "EscapeSafe.generated.h"

class UTextRenderComponent;

/** Safe with four 0-9 dials and a handle (puzzle 3). Local +X is the front. */
UCLASS()
class AEscapeSafe : public AEscapeInteractable
{
	GENERATED_BODY()

public:
	AEscapeSafe();

	static constexpr int32 NumDials = 4;

	virtual void Interact(AEscapeCharacter* Player, UPrimitiveComponent* HitComponent) override;
	virtual FText GetPrompt(const UPrimitiveComponent* HitComponent) const override;

	void OpenDoor();
	bool IsOpen() const { return bOpen; }
	FString GetCode() const;
	/** Dial index for a component, or INDEX_NONE. */
	int32 GetDialIndex(const UPrimitiveComponent* Component) const;
	int32 GetDialValue(int32 Index) const { return DialValues.IsValidIndex(Index) ? DialValues[Index] : 0; }

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<USceneComponent> DoorHinge;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> SafeDoor;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> Handle;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> DoorKey;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TArray<TObjectPtr<UStaticMeshComponent>> Dials;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TArray<TObjectPtr<UTextRenderComponent>> DialTexts;

	/**
	 * Order clue: one frame-outline silhouette above each dial, shaped like the frame whose count goes into that dial
	 * (smallest frame above dial 1) and filled with that painting's background colour.
	 * Sizes and colours are taken from the AEscapeFrame actors in the level.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TArray<TObjectPtr<UStaticMeshComponent>> SilhouetteOutlines;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TArray<TObjectPtr<UStaticMeshComponent>> SilhouetteFills;

	/** Largest silhouette size in cm (the biggest frame is scaled to fit inside this box). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Safe")
	FVector2D SilhouetteMaxSize = FVector2D(11.f, 16.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Safe")
	FLinearColor SilhouetteColor = FLinearColor(0.85f, 0.72f, 0.4f);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void ApplyColors();
	void UpdateDialTexts();
	void UpdateSilhouettes();

	TArray<int32> DialValues;
	bool bOpen = false;
	float OpenAlpha = 0.f;
};
