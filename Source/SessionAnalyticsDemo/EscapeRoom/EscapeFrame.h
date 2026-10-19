#pragma once

#include "CoreMinimal.h"
#include "EscapeRoom/EscapeInteractable.h"
#include "EscapeFrame.generated.h"

class UInstancedStaticMeshComponent;
class UTextRenderComponent;

/**
 * A picture frame on the wall. Puzzle 1: lift it (the latest DateLabel hides the drawer key).
 * Puzzle 3: the painting shows ObjectCount objects; counts read smallest frame to largest give the safe code.
 * Local +X points out of the wall.
 */
UCLASS()
class AEscapeFrame : public AEscapeInteractable
{
	GENERATED_BODY()

public:
	AEscapeFrame();

	virtual void Interact(AEscapeCharacter* Player, UPrimitiveComponent* HitComponent) override;
	virtual FText GetPrompt(const UPrimitiveComponent* HitComponent) const override;

	/** Lifts the frame and drops it back after a moment (nothing behind it). */
	void Lift(bool bRevealKey);

	/**
	 * v5: lifts the frame and keeps it up with the key showing (glowing softly) until the player picks the key up with E.
	 * v4 tester: the key was taken automatically and the player never noticed, so they kept searching with it in hand.
	 */
	void RevealKey();
	/** Hides the key and drops the frame back. */
	void TakeKey();
	bool IsKeyRevealed() const { return bKeyRevealed; }

	float GetArea() const { return FrameSize.X * FrameSize.Y; }

	/** YYYY.MM, compared as text to find the latest frame. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Frame")
	FString DateLabel = TEXT("2020.01");

	/** Width x height of the painting in cm. Sizes must all differ (puzzle 3 reads them in size order). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Frame")
	FVector2D FrameSize = FVector2D(40.f, 50.f);

	/** Number of countable objects painted on the canvas (one digit of the safe code). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Frame", meta = (ClampMin = "0", ClampMax = "9"))
	int32 ObjectCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Frame")
	FLinearColor CanvasColor = FLinearColor(0.85f, 0.82f, 0.72f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Frame")
	FLinearColor ObjectColor = FLinearColor(0.7f, 0.1f, 0.1f);

	/** Height of the date digits in cm (v1: 6 cm was only readable from about 2 m; v2: 10 cm). The plate scales with it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Frame", meta = (ClampMin = "2"))
	float LabelTextSize = 10.f;

	/** Message when the frame is lifted and nothing is behind it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Frame")
	FText EmptyMessage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Frame", meta = (ClampMin = "0.05"))
	float LiftSeconds = 0.35f;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<USceneComponent> LiftRoot;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> Canvas;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UInstancedStaticMeshComponent> Objects;

	/** Tagged P1H2 by default (date-label hint). */
	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> LabelPlate;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UTextRenderComponent> LabelText;

	/** Key shaft. With KeyBow and KeyTooth it forms the drawer key hanging behind the frame. */
	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> Key;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> KeyBow;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> KeyTooth;

	bool IsKeyPart(const UPrimitiveComponent* Component) const { return Component && (Component == Key || Component == KeyBow || Component == KeyTooth); }

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void Layout();
	void SetKeyShown(bool bShown);
	float GetLiftHeight() const { return FrameSize.Y * 0.5f + 14.f; }

	bool bKeyRevealed = false;
	float LiftAlpha = 0.f;
	float LiftTarget = 0.f;
	/** World time at which the frame starts dropping back. */
	double DropAt = -1.0;
};
