#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EscapeCharacter.generated.h"

class UCameraComponent;

/**
 * First-person player: WASD + mouse look, E fires a line trace from the camera.
 * Input is read directly from the player controller each tick, so the prototype needs no input assets.
 */
UCLASS()
class AEscapeCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AEscapeCharacter();

	/** Trace from the camera and interact with what is hit. Returns false if nothing interactable was in reach. */
	UFUNCTION(BlueprintCallable, Category = "Escape")
	bool Interact();

	/** Prompt for the object under the crosshair, or empty. */
	const FText& GetFocusPrompt() const { return FocusPrompt; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Player", meta = (ClampMin = "50"))
	float InteractDistance = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Player", meta = (ClampMin = "0.05"))
	float MouseSensitivity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Player", meta = (ClampMin = "50"))
	float WalkSpeed = 320.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Escape")
	TObjectPtr<UCameraComponent> Camera;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	bool TraceView(FHitResult& OutHit) const;
	void ReadInput();
	void UpdateFocus();

	FText FocusPrompt;
};
