#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EscapeCharacter.generated.h"

class AEscapeRoomManager;
class UCameraComponent;

/**
 * First-person player: WASD + mouse look, E fires a line trace from the camera, H asks for a hint, Esc opens the menu.
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
	bool HasFocus() const { return FocusComponent.IsValid(); }
	/** Seconds since E was last pressed, and whether that press hit something interactable (for crosshair feedback). */
	float GetSecondsSinceInteract() const;
	bool LastInteractHit() const { return bLastInteractHit; }
	/** Lets scripted input (autoplay) press E several times in one frame. */
	void ResetInteractCooldown() { LastInteractTime = -1000.0; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Player", meta = (ClampMin = "50"))
	float InteractDistance = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Player", meta = (ClampMin = "0.05"))
	float MouseSensitivity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Player", meta = (ClampMin = "50"))
	float WalkSpeed = 320.f;

	/** Glow added to the part under the crosshair so the player sees what E will use. 0 = off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Player", meta = (ClampMin = "0"))
	float FocusHighlight = 0.6f;

	/** E presses closer together than this are ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Player", meta = (ClampMin = "0"))
	float InteractCooldownSeconds = 0.2f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Escape")
	TObjectPtr<UCameraComponent> Camera;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	bool TraceView(FHitResult& OutHit) const;
	void ReadInput();
	void UpdateFocus();

	void SetFocus(UPrimitiveComponent* Component);
	AEscapeRoomManager* GetManager() const;
	bool IsMenuOpen() const;

	mutable TWeakObjectPtr<AEscapeRoomManager> CachedManager;
	FText FocusPrompt;
	TWeakObjectPtr<UPrimitiveComponent> FocusComponent;
	double LastInteractTime = -1000.0;
	bool bLastInteractHit = false;
};
