#pragma once

#include "CoreMinimal.h"
#include "EscapeRoom/EscapeInteractable.h"
#include "EscapeDoor.generated.h"

/** Exit door with the green lamp above it. Local +X faces into the room. */
UCLASS()
class AEscapeDoor : public AEscapeInteractable
{
	GENERATED_BODY()

public:
	AEscapeDoor();

	virtual void Interact(AEscapeCharacter* Player, UPrimitiveComponent* HitComponent) override;
	virtual FText GetPrompt(const UPrimitiveComponent* HitComponent) const override;

	void Open();

	/** The color the player has to mix in puzzle 2. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Door")
	FLinearColor LampColor = FLinearColor(0.05f, 0.75f, 0.12f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Door")
	float LampGlow = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Door")
	FText LampName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Door", meta = (MultiLine = true))
	FText LampMessage;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<USceneComponent> Hinge;

	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> Knob;

	/** Tagged P2H1 by default (lamp hint). */
	UPROPERTY(VisibleAnywhere, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> Lamp;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void ApplyColors();

	bool bOpen = false;
	float OpenAlpha = 0.f;
};
