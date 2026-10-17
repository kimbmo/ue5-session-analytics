#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EscapeRoomManager.generated.h"

class AEscapeDesk;
class AEscapeDoor;
class AEscapeFrame;
class AEscapeInteractable;
class AEscapePaint;
class AEscapeSafe;
class ULightComponent;
class UPrimitiveComponent;
class UTextRenderComponent;

/** Per-puzzle hint and light setup. Tags can be on actors (whole actor glows) or on components (only that part glows). */
USTRUCT(BlueprintType)
struct FEscapePuzzleConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape")
	FName Hint1Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape")
	FName Hint2Tag;

	/** Lights with this actor tag are bright only while this puzzle is current. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape")
	FName LightTag;

	/** Hint 2 targets light up one at a time (frames: smallest first) instead of all together. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape")
	bool bHint2Sequential = false;

	/** Optional line shown with each hint. Empty = visual hint only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape")
	FText Hint1Message;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Escape")
	FText Hint2Message;
};

/**
 * Owns the puzzle sequence: which puzzle is current, attempts, hints, lights, inventory and analytics.
 * Place exactly one in the level; every tuning value is in its Details panel.
 */
UCLASS()
class AEscapeRoomManager : public AActor
{
	GENERATED_BODY()

public:
	AEscapeRoomManager();

	// ---- Tuning ----

	/** Seconds without progress before hint 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Hints", meta = (ClampMin = "1"))
	float HintDelay1 = 30.f;

	/** Seconds without progress before hint 2. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Hints", meta = (ClampMin = "1"))
	float HintDelay2 = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Hints", meta = (ClampMin = "0"))
	float HintGlowStrength = 2.5f;

	/** Pulses per second of a glowing hint target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Hints", meta = (ClampMin = "0"))
	float HintPulseSpeed = 1.2f;

	/** Seconds each target stays lit in a sequential hint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Hints", meta = (ClampMin = "0.1"))
	float SequenceStepSeconds = 0.9f;

	/** Index 0 = puzzle 1 (find), 1 = puzzle 2 (combine), 2 = puzzle 3 (order). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Puzzles")
	TArray<FEscapePuzzleConfig> Puzzles;

	/** Puzzle 2: the two paint ids that make the lamp color (order does not matter). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Puzzles")
	FString CorrectPaintA = TEXT("blue");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Puzzles")
	FString CorrectPaintB = TEXT("yellow");

	/** Puzzle 3: object counts of the paintings, smallest frame to largest. A mismatch with the frames is logged at BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Puzzles")
	FString SafeCode = TEXT("3715");

	/** Hidden when puzzle 2 is solved. If empty, the first actor tagged SafeCoverTag is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Puzzles")
	TObjectPtr<AActor> SafeCover;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Puzzles")
	FName SafeCoverTag = TEXT("SafeCover");

	/** Puzzle lights of other puzzles are scaled by this (0 = off). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Lighting", meta = (ClampMin = "0", ClampMax = "1"))
	float InactiveLightScale = 0.08f;

	/** Show the elapsed time next to the puzzle number in the HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|UI")
	bool bShowElapsedTime = false;

	/** Seconds an on-screen message stays. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|UI", meta = (ClampMin = "0.5"))
	float MessageSeconds = 2.f;

	// ---- Called by interactables ----

	/** Logs object_inspected{object}. Counts as progress if the object belongs to the current puzzle (or PuzzleOverride does). */
	void NotifyInspected(const AEscapeInteractable* Source, const FString& Id, int32 PuzzleOverride = 0);
	/** Progress without a log row (e.g. turning a safe dial). */
	void NotifyProgress(int32 Puzzle);

	void HandleFrame(AEscapeFrame* Frame);
	void HandleDrawer(AEscapeDesk* Desk);
	void HandlePaint(AEscapePaint* Paint);
	void HandleSafeHandle(AEscapeSafe* Safe, const FString& Code);
	void HandleDoor(AEscapeDoor* Door);

	void ShowMessage(const FText& Message);

	// ---- State for HUD / autoplay ----

	UFUNCTION(BlueprintPure, Category = "Escape")
	int32 GetCurrentPuzzle() const { return CurrentPuzzle; }

	UFUNCTION(BlueprintPure, Category = "Escape")
	bool HasEscaped() const { return bEscaped; }

	/** 0 = no hint yet for the current puzzle, 1 or 2 = hint level shown. */
	int32 GetHintLevel() const { return HintLevel; }
	float GetElapsedSeconds() const;
	int32 GetHintsUsed() const { return HintsUsed; }
	const FText& GetMessage() const { return Message; }
	bool IsMessageVisible() const;
	FText GetStatusText() const;
	FText GetInventoryText() const;
	/** Palette slots, or empty text when the player has no palette. */
	FText GetPaletteText() const;

	/** Frames sorted smallest to largest (by area). */
	TArray<AEscapeFrame*> GetFramesBySize() const;
	AEscapeFrame* GetLatestFrame() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void StartPuzzle(int32 Puzzle);
	void SolveCurrentPuzzle();
	void WrongAttempt(const FString& Input);
	void ShowHint(int32 Level);
	void ApplyLights();
	void ClearGlow();
	void AddGlowTargets(FName Tag, bool bSequential);
	void SetGlow(UPrimitiveComponent* Component, float Amount);
	void Record(FName Event, const TMap<FString, FString>& Params) const;
	const FEscapePuzzleConfig* GetConfig(int32 Puzzle) const;
	void ValidateSafeCode() const;

	struct FGlowTarget
	{
		TWeakObjectPtr<UPrimitiveComponent> Component;
		/** -1 = always lit, otherwise position in a sequential hint. */
		int32 SequenceIndex = -1;
	};
	TArray<FGlowTarget> GlowTargets;
	int32 SequenceCount = 0;
	TMap<TWeakObjectPtr<UTextRenderComponent>, FColor> TextBaseColors;

	struct FPuzzleLight
	{
		TWeakObjectPtr<ULightComponent> Light;
		float BaseIntensity = 0.f;
		int32 Puzzle = 0;
	};
	TArray<FPuzzleLight> PuzzleLights;

	/** 1-3 = puzzle, 4 = door unlocked, 5 = escaped. */
	UPROPERTY(VisibleInstanceOnly, Category = "Escape|State")
	int32 CurrentPuzzle = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Escape|State")
	int32 Attempts = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Escape|State")
	int32 HintLevel = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Escape|State")
	float IdleSeconds = 0.f;

	UPROPERTY(VisibleInstanceOnly, Category = "Escape|State")
	int32 HintsUsed = 0;

	bool bHasDrawerKey = false;
	bool bHasPalette = false;
	bool bHasDoorKey = false;
	bool bEscaped = false;
	TArray<TWeakObjectPtr<AEscapePaint>> PaletteSlots;

	double StartTime = 0.0;
	double EscapeTime = 0.0;
	double MessageUntil = 0.0;
	FText Message;
};
