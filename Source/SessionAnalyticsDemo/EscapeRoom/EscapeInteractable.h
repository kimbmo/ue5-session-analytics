#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EscapeInteractable.generated.h"

class AEscapeCharacter;
class AEscapeRoomManager;
class UStaticMeshComponent;

/**
 * Anything the player can press E on. The base class is a plain inspectable object:
 * it logs object_inspected and shows InspectMessage. Puzzle objects override Interact.
 */
UCLASS()
class AEscapeInteractable : public AActor
{
	GENERATED_BODY()

public:
	AEscapeInteractable();

	/** Called by the character's E trace. HitComponent is the part that was hit (dial, drawer, lamp...). */
	virtual void Interact(AEscapeCharacter* Player, UPrimitiveComponent* HitComponent);

	/** Text shown next to the crosshair while this object is in reach. */
	virtual FText GetPrompt(const UPrimitiveComponent* HitComponent) const { return DisplayName; }

	/** True if this object belongs to the given puzzle (1-3). Touching it then counts as progress. */
	bool BelongsToPuzzle(int32 Puzzle) const { return PuzzleIds.Contains(Puzzle); }

	/** Id written to the analytics log (object_inspected{object}). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape")
	FString ObjectId = TEXT("object");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape")
	FText DisplayName;

	/** Puzzles (1-3) this object belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape")
	TArray<int32> PuzzleIds;

	/** Shown for 2 seconds when inspected. Empty = no message. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape", meta = (MultiLine = true))
	FText InspectMessage;

	/** Color of the main mesh (gray-box material). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape|Look")
	FLinearColor BaseColor = FLinearColor(0.5f, 0.5f, 0.5f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Escape")
	TObjectPtr<UStaticMeshComponent> Mesh;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Logs object_inspected and counts as progress for the current puzzle if this object belongs to it. */
	void ReportInspected(const FString& Id) const;
	void ShowMessage(const FText& Message) const;

	AEscapeRoomManager* GetManager() const;

private:
	mutable TWeakObjectPtr<AEscapeRoomManager> CachedManager;
};
