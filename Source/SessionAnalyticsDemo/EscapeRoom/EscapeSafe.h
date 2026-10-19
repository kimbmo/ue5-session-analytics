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

	/** Opens the door; the door key then waits inside until TakeDoorKey (E on the key, v5). */
	void OpenDoor();
	bool IsOpen() const { return bOpen; }
	bool HasDoorKeyInside() const { return bKeyInside; }
	void TakeDoorKey();
	bool IsDoorKeyPart(const UPrimitiveComponent* Component) const;
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

	/** Door key shaft; with DoorKeyBow and DoorKeyTooth it forms the key. */
	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> DoorKey;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> DoorKeyBow;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> DoorKeyTooth;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TArray<TObjectPtr<UStaticMeshComponent>> Dials;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TArray<TObjectPtr<UTextRenderComponent>> DialTexts;

	static constexpr int32 DotsPerSilhouette = 2;

	/**
	 * Order clue (v4: colour, not size). Dial n reads the painting whose background colour is on the plate behind it
	 * and on the silhouette above it; the silhouette's dots have the colour of the things painted on it.
	 * Dial n = n-th smallest frame, so the code is unchanged. Colours are taken from the AEscapeFrame actors in the level.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TArray<TObjectPtr<UStaticMeshComponent>> DialPlates;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TArray<TObjectPtr<UStaticMeshComponent>> SilhouetteOutlines;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TArray<TObjectPtr<UStaticMeshComponent>> SilhouetteFills;

	/** DotsPerSilhouette per dial, dial by dial. */
	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TArray<TObjectPtr<UStaticMeshComponent>> SilhouetteDots;

	/** Size of every silhouette in cm (all the same since v4, so size is not a clue). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Safe")
	FVector2D SilhouetteSize = FVector2D(12.f, 15.f);

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
	void SetDoorKeyShown(bool bShown);

	TArray<int32> DialValues;
	bool bOpen = false;
	bool bKeyInside = false;
	float OpenAlpha = 0.f;
};
