#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EscapeRoomManager.generated.h"

class AEscapeCharacter;
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

	/**
	 * v4: hints are shown on request (H). These delays are only a safety net for players who never ask.
	 * Seconds since the last real step before hint 1. Real steps are only the puzzle starting and the drawer opening:
	 * inspecting objects, turning dials and wrong answers do not reset the timer (v1 playtest: players who kept
	 * pressing E on things never got a hint).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Hints", meta = (ClampMin = "1"))
	float HintDelay1 = 90.f;

	/** Seconds since the last real step before the automatic hint 2. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Hints", meta = (ClampMin = "1"))
	float HintDelay2 = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Hints", meta = (ClampMin = "0"))
	float HintGlowStrength = 5.f;

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

	/**
	 * Puzzle lights of other puzzles are scaled by this (0 = off). v4: 0.6 (v2 was 0.08), so the room brightness changes
	 * by at most ~10% between puzzles instead of jumping when a puzzle is solved.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Lighting", meta = (ClampMin = "0", ClampMax = "1"))
	float InactiveLightScale = 0.6f;

	/** Written to the build_info event at session start, so every CSV shows which build was played. */
	/**
	 * v5: seconds after picking up the drawer key without opening the drawer before the drawer starts to glow softly.
	 * Not a hint (not counted, no text); logged once as nudge_shown{target=drawer}. 0 = off.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Hints", meta = (ClampMin = "0"))
	float DrawerNudgeDelay = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Hints", meta = (ClampMin = "0"))
	float DrawerNudgeStrength = 2.f;

	/** Seconds the big "...획득!" line and the item icon flash stay after picking something up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|UI", meta = (ClampMin = "0.5"))
	float ItemToastSeconds = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Analytics")
	FString BuildVersion = TEXT("v5");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Analytics")
	FString BuildName = TEXT("work-pc");

	/** Range of the mouse sensitivity slider in the Esc menu. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|UI", meta = (ClampMin = "0.01"))
	float MinMouseSensitivity = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|UI", meta = (ClampMin = "0.01"))
	float MaxMouseSensitivity = 1.5f;

	/** Show the elapsed time next to the puzzle number in the HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|UI")
	bool bShowElapsedTime = false;

	/** Seconds an on-screen message stays. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|UI", meta = (ClampMin = "0.5"))
	float MessageSeconds = 3.f;

	/** Seconds a hint line stays near the screen centre. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|UI", meta = (ClampMin = "0.5"))
	float HintMessageSeconds = 4.f;

	/** Pulls of the safe handle inside this many seconds are ignored (stops double-logged wrong codes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape|Puzzles", meta = (ClampMin = "0"))
	float HandleCooldownSeconds = 1.f;

	// ---- Called by interactables ----

	/** Logs object_inspected{object}. Does not reset the hint timer. */
	void NotifyInspected(const AEscapeInteractable* Source, const FString& Id);

	void HandleFrame(AEscapeFrame* Frame);
	/** E on the key behind the lifted latest frame: takes the drawer key and solves puzzle 1. */
	void HandleFrameKey(AEscapeFrame* Frame);
	void HandleDrawer(AEscapeDesk* Desk);
	/** E on the palette in the open drawer. */
	void HandlePalette(AEscapeDesk* Desk);
	/** E on the door key inside the open safe: takes it and solves puzzle 3. */
	void HandleSafeKey(AEscapeSafe* Safe);
	void HandlePaint(AEscapePaint* Paint);
	void HandleSafeHandle(AEscapeSafe* Safe, const FString& Code);
	void HandleDoor(AEscapeDoor* Door);

	void ShowMessage(const FText& Message);

	// ---- Called by the player (keys) and the HUD (menu buttons) ----

	/** H: shows the next hint level (1, then 2) of the current puzzle. */
	void RequestHint();

	/** Esc menu. While it is open the game is paused, so puzzle and hint timers stop. */
	void OpenMenu();
	void CloseMenu();
	/** Q or the quit button: asks "정말 종료할까요?". */
	void RequestQuit();
	void CancelQuit();
	/** Y or the confirm button: ends the analytics session (reason=Quit) and quits. */
	void ConfirmQuit();
	bool IsMenuOpen() const { return bMenuOpen; }
	bool IsQuitConfirmOpen() const { return bMenuOpen && bQuitConfirm; }

	float GetMouseSensitivity() const;
	/** Applies right away; logged as setting_changed once the slider is released (see CommitMouseSensitivity). */
	void SetMouseSensitivity(float Value);
	void CommitMouseSensitivity();

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
	const FText& GetHintMessage() const { return HintMessage; }
	bool IsHintMessageVisible() const;
	bool IsMessageVisible() const;
	FText GetStatusText() const;
	/** "[H] 힌트 n/2" for the current puzzle, empty when no puzzle is running. */
	FText GetHintCounterText() const;

	/** Held items, for the icons in the bottom-left corner. */
	/** The drawer key is used up when the drawer opens. */
	bool HasDrawerKeyItem() const { return bHasDrawerKey && !bDrawerOpened; }
	bool HasPaletteItem() const { return bHasPalette && CurrentPuzzle <= 2; }
	/** The two paint slots are shown while the palette is used (puzzle 2). */
	bool ShowsPaintSlots() const { return bHasPalette && CurrentPuzzle == 2; }
	/** Colour in paint slot 0 or 1; false when the slot is empty. */
	bool GetPaintSlotColor(int32 Slot, FLinearColor& OutColor) const;
	bool HasDoorKeyItem() const { return bHasDoorKey && !bEscaped; }

	/** Big centre line after picking up an item ("서랍 열쇠 획득!"), empty when not showing. */
	FText GetItemToast() const;
	/** 0..1 flash of the item icon (drawer_key, palette, door_key) right after it was picked up. */
	float GetItemFlash(FName Item) const;
	/** True while the drawer glows because the key was not used for DrawerNudgeDelay seconds. */
	bool IsDrawerNudgeActive() const { return bDrawerNudgeShown && bHasDrawerKey && !bDrawerOpened; }

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
	void ShowHint(int32 Level, bool bRequested);
	void ClearHintMessage();
	void PickUpItem(FName Item, const FText& Toast);
	void UpdateDrawerNudge();
	AEscapeCharacter* GetPlayer() const;
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
	float SecondsSinceStep = 0.f;

	UPROPERTY(VisibleInstanceOnly, Category = "Escape|State")
	int32 HintsUsed = 0;

	bool bHasDrawerKey = false;
	bool bHasPalette = false;
	bool bDrawerOpened = false;
	bool bHasDoorKey = false;
	bool bEscaped = false;
	TArray<TWeakObjectPtr<AEscapePaint>> PaletteSlots;

	double StartTime = 0.0;
	double EscapeTime = 0.0;
	double MessageUntil = 0.0;
	FText Message;
	double HintMessageUntil = 0.0;
	FText HintMessage;
	double LastHandleTime = -1000.0;

	bool bMenuOpen = false;
	bool bQuitConfirm = false;
	/** Real time (FPlatformTime) the menu was opened; the world clock is paused while it is open. */
	double MenuOpenedAt = 0.0;
	float LoggedMouseSensitivity = -1.f;

	FText ItemToast;
	double ItemToastUntil = 0.0;
	FName FlashItem;
	double KeyPickedAt = 0.0;
	bool bDrawerNudgeShown = false;
	TWeakObjectPtr<AEscapeDesk> NudgeDesk;
};
