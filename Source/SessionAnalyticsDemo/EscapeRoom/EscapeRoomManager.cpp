#include "EscapeRoom/EscapeRoomManager.h"

#include "Components/LightComponent.h"
#include "Components/MeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "EscapeRoom/EscapeDesk.h"
#include "EscapeRoom/EscapeDoor.h"
#include "EscapeRoom/EscapeEasel.h"
#include "EscapeRoom/EscapeFrame.h"
#include "EscapeRoom/EscapePaint.h"
#include "EscapeRoom/EscapeSafe.h"
#include "EscapeRoom/EscapeVisuals.h"
#include "Misc/Paths.h"
#include "SessionAnalyticsSubsystem.h"

#define LOCTEXT_NAMESPACE "EscapeRoom"

DEFINE_LOG_CATEGORY_STATIC(LogEscapeRoom, Log, All);

namespace
{
	FEscapePuzzleConfig MakeConfig(const TCHAR* Name, int32 Puzzle, bool bSequential, const FText& Hint1, const FText& Hint2)
	{
		FEscapePuzzleConfig Config;
		Config.Name = Name;
		Config.Hint1Tag = *FString::Printf(TEXT("P%dH1"), Puzzle);
		Config.Hint2Tag = *FString::Printf(TEXT("P%dH2"), Puzzle);
		Config.LightTag = *FString::Printf(TEXT("Light_P%d"), Puzzle);
		Config.bHint2Sequential = bSequential;
		Config.Hint1Message = Hint1;
		Config.Hint2Message = Hint2;
		return Config;
	}

	FString Itoa(int32 Value)
	{
		return FString::FromInt(Value);
	}

	const FColor TextGlowColor(255, 220, 60);
}

AEscapeRoomManager::AEscapeRoomManager()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Puzzles.Add(MakeConfig(TEXT("find"), 1, false,
		LOCTEXT("P1H1", "책상 위 메모가 눈에 들어온다."),
		LOCTEXT("P1H2", "액자 아래 날짜들을 비교해 보자.")));
	Puzzles.Add(MakeConfig(TEXT("combine"), 2, false,
		LOCTEXT("P2H1", "문 위 램프와 같은 색을 만들 수 있을까?"),
		LOCTEXT("P2H2", "두 가지 물감이 반짝인다.")));
	Puzzles.Add(MakeConfig(TEXT("order"), 3, true,
		LOCTEXT("P3H1", "그림 속에 그려진 것들을 세어 보자."),
		LOCTEXT("P3H2", "작은 액자부터 차례로 빛난다.")));
}

// ---------------------------------------------------------------------------------------------
// Lifecycle

void AEscapeRoomManager::BeginPlay()
{
	Super::BeginPlay();

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (USessionAnalyticsSubsystem* Analytics = GameInstance->GetSubsystem<USessionAnalyticsSubsystem>())
		{
			if (!Analytics->IsSessionActive())
			{
				Analytics->StartSession();
			}
		}
	}

	StartTime = GetWorld()->GetTimeSeconds();

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!SafeCover && !SafeCoverTag.IsNone() && It->ActorHasTag(SafeCoverTag))
		{
			SafeCover = *It;
		}
		for (int32 Index = 0; Index < Puzzles.Num(); ++Index)
		{
			if (Puzzles[Index].LightTag.IsNone() || !It->ActorHasTag(Puzzles[Index].LightTag))
			{
				continue;
			}
			TArray<ULightComponent*> Lights;
			It->GetComponents(Lights);
			for (ULightComponent* Light : Lights)
			{
				PuzzleLights.Add({ Light, Light->Intensity, Index + 1 });
			}
		}
	}

	ValidateSafeCode();
	StartPuzzle(1);
}

void AEscapeRoomManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CurrentPuzzle >= 1 && CurrentPuzzle <= 3)
	{
		SecondsSinceStep += DeltaSeconds;
		if (HintLevel < 1 && SecondsSinceStep >= HintDelay1)
		{
			ShowHint(1);
		}
		else if (HintLevel == 1 && SecondsSinceStep >= HintDelay2)
		{
			ShowHint(2);
		}
	}

	if (GlowTargets.Num() == 0)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	const float Pulse = HintGlowStrength * (0.6f + 0.4f * FMath::Sin(Now * HintPulseSpeed * UE_TWO_PI));
	// One dark beat after the last target, so the loop start (the smallest frame) is easy to read.
	const int32 Step = SequenceCount > 0 ? FMath::FloorToInt(Now / SequenceStepSeconds) % (SequenceCount + 1) : 0;
	for (const FGlowTarget& Target : GlowTargets)
	{
		const float Amount = Target.SequenceIndex < 0 ? Pulse : (Target.SequenceIndex == Step ? HintGlowStrength : 0.f);
		SetGlow(Target.Component.Get(), Amount);
	}
}

// ---------------------------------------------------------------------------------------------
// Puzzle flow

void AEscapeRoomManager::StartPuzzle(int32 Puzzle)
{
	CurrentPuzzle = Puzzle;
	Attempts = 0;
	HintLevel = 0;
	SecondsSinceStep = 0.f;
	ClearGlow();
	ApplyLights();
	Record(TEXT("puzzle_started"), { { TEXT("puzzle"), Itoa(Puzzle) } });
}

void AEscapeRoomManager::SolveCurrentPuzzle()
{
	++Attempts;
	const int32 Solved = CurrentPuzzle;
	Record(TEXT("puzzle_solved"), { { TEXT("puzzle"), Itoa(Solved) }, { TEXT("attempts"), Itoa(Attempts) } });

	if (Solved == 2)
	{
		if (SafeCover)
		{
			SafeCover->SetActorHiddenInGame(true);
			SafeCover->SetActorEnableCollision(false);
		}
		AEscapeDoor* Door = nullptr;
		for (TActorIterator<AEscapeDoor> It(GetWorld()); It; ++It)
		{
			Door = *It;
		}
		for (TActorIterator<AEscapeEasel> It(GetWorld()); It; ++It)
		{
			It->FillPatch(Door ? Door->LampColor : FLinearColor::Green);
		}
	}

	if (Solved < 3)
	{
		StartPuzzle(Solved + 1);
	}
	else
	{
		CurrentPuzzle = 4;
		HintLevel = 0;
		ClearGlow();
		ApplyLights();
	}
}

void AEscapeRoomManager::WrongAttempt(const FString& Input)
{
	++Attempts;
	Record(TEXT("wrong_attempt"), { { TEXT("puzzle"), Itoa(CurrentPuzzle) }, { TEXT("input"), Input } });
}

void AEscapeRoomManager::NotifyInspected(const AEscapeInteractable* Source, const FString& Id)
{
	Record(TEXT("object_inspected"), { { TEXT("object"), Id } });
}

// ---------------------------------------------------------------------------------------------
// Puzzle 1: find

AEscapeFrame* AEscapeRoomManager::GetLatestFrame() const
{
	AEscapeFrame* Latest = nullptr;
	for (TActorIterator<AEscapeFrame> It(GetWorld()); It; ++It)
	{
		if (!Latest || It->DateLabel > Latest->DateLabel)
		{
			Latest = *It;
		}
	}
	return Latest;
}

void AEscapeRoomManager::HandleFrame(AEscapeFrame* Frame)
{
	if (CurrentPuzzle != 1)
	{
		NotifyInspected(Frame, Frame->ObjectId);
		ShowMessage(Frame->InspectMessage);
		return;
	}

	if (Frame == GetLatestFrame())
	{
		Frame->Lift(true);
		bHasDrawerKey = true;
		ShowMessage(LOCTEXT("FoundDrawerKey", "액자 뒤에서 작은 열쇠를 찾았다! 서랍 열쇠 같다."));
		SolveCurrentPuzzle();
	}
	else
	{
		Frame->Lift(false);
		ShowMessage(Frame->EmptyMessage);
		WrongAttempt(Frame->ObjectId);
	}
}

// ---------------------------------------------------------------------------------------------
// Puzzle 2: combine

void AEscapeRoomManager::HandleDrawer(AEscapeDesk* Desk)
{
	NotifyInspected(Desk, TEXT("drawer"));
	if (Desk->IsDrawerOpen())
	{
		ShowMessage(LOCTEXT("DrawerEmpty", "서랍은 이제 비어 있다."));
		return;
	}
	if (!bHasDrawerKey)
	{
		ShowMessage(LOCTEXT("DrawerLocked", "잠겨 있다. 작은 열쇠 구멍이 보인다."));
		return;
	}
	Desk->OpenDrawer();
	bHasPalette = true;
	// Opening the drawer is a real step inside puzzle 2: restart its hint timer.
	SecondsSinceStep = 0.f;
	ShowMessage(LOCTEXT("GotPalette", "서랍 안에 팔레트가 있다. 물감 두 가지를 섞어 보자."));
}

void AEscapeRoomManager::HandlePaint(AEscapePaint* Paint)
{
	NotifyInspected(Paint, TEXT("paint_") + Paint->PaintId);
	if (!bHasPalette)
	{
		ShowMessage(LOCTEXT("NeedPalette", "물감을 덜어 둘 팔레트가 필요하다."));
		return;
	}
	if (CurrentPuzzle != 2)
	{
		ShowMessage(LOCTEXT("ColorDone", "필요한 색은 이미 만들었다."));
		return;
	}

	// A second press on the same can is almost always E being mashed, not an answer.
	if (PaletteSlots.Num() == 1 && PaletteSlots[0].Get() == Paint)
	{
		ShowMessage(FText::Format(LOCTEXT("PaintSame", "{0} 물감은 이미 팔레트에 있다. 다른 물감을 고르자."), Paint->ColorName));
		return;
	}

	PaletteSlots.Add(Paint);
	if (PaletteSlots.Num() < 2)
	{
		ShowMessage(FText::Format(LOCTEXT("PaintAdded", "팔레트에 {0} 물감을 덜었다. 하나 더 고르자."), Paint->ColorName));
		return;
	}

	const AEscapePaint* First = PaletteSlots[0].Get();
	const FString A = First ? First->PaintId : FString();
	const FString B = Paint->PaintId;
	const FText NameA = First ? First->ColorName : FText::GetEmpty();
	PaletteSlots.Reset();

	const bool bCorrect = (A == CorrectPaintA && B == CorrectPaintB) || (A == CorrectPaintB && B == CorrectPaintA);
	if (bCorrect)
	{
		ShowMessage(LOCTEXT("MixCorrect", "초록색이 됐다! 램프와 같은 색이다. ...어딘가에서 덜컥 소리가 났다."));
		SolveCurrentPuzzle();
	}
	else
	{
		ShowMessage(FText::Format(LOCTEXT("MixWrong", "{0} + {1}... 램프 색과 다르다. 팔레트를 닦았다."), NameA, Paint->ColorName));
		WrongAttempt(A + TEXT("+") + B);
	}
}

// ---------------------------------------------------------------------------------------------
// Puzzle 3: order

TArray<AEscapeFrame*> AEscapeRoomManager::GetFramesBySize() const
{
	TArray<AEscapeFrame*> Frames;
	for (TActorIterator<AEscapeFrame> It(GetWorld()); It; ++It)
	{
		Frames.Add(*It);
	}
	Frames.Sort([](const AEscapeFrame& L, const AEscapeFrame& R) { return L.GetArea() < R.GetArea(); });
	return Frames;
}

void AEscapeRoomManager::ValidateSafeCode() const
{
	FString FromFrames;
	for (const AEscapeFrame* Frame : GetFramesBySize())
	{
		FromFrames.AppendInt(Frame->ObjectCount);
	}
	if (FromFrames != SafeCode)
	{
		const FString Warning = FString::Printf(TEXT("EscapeRoom: SafeCode is %s but the frames (smallest to largest) read %s"), *SafeCode, *FromFrames);
		UE_LOG(LogEscapeRoom, Warning, TEXT("%s"), *Warning);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Orange, Warning);
		}
	}
}

void AEscapeRoomManager::HandleSafeHandle(AEscapeSafe* Safe, const FString& Code)
{
	if (CurrentPuzzle != 3)
	{
		NotifyInspected(Safe, TEXT("safe_handle"));
		ShowMessage(Safe->IsOpen() ? LOCTEXT("SafeAlreadyOpen", "금고는 이미 열려 있다.") : LOCTEXT("SafeStuck", "꿈쩍도 하지 않는다."));
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastHandleTime < HandleCooldownSeconds)
	{
		return;
	}
	LastHandleTime = Now;

	if (Code == SafeCode)
	{
		Safe->OpenDoor();
		bHasDoorKey = true;
		ShowMessage(LOCTEXT("SafeOpen", "철컥! 금고 안에서 문 열쇠를 얻었다."));
		SolveCurrentPuzzle();
	}
	else
	{
		ShowMessage(FText::Format(LOCTEXT("SafeWrong", "{0}... 열리지 않는다."), FText::FromString(Code)));
		WrongAttempt(Code);
	}
}

// ---------------------------------------------------------------------------------------------
// Door

void AEscapeRoomManager::HandleDoor(AEscapeDoor* Door)
{
	if (bEscaped)
	{
		return;
	}
	if (!bHasDoorKey)
	{
		NotifyInspected(Door, Door->ObjectId);
		ShowMessage(Door->InspectMessage);
		return;
	}

	Door->Open();
	bEscaped = true;
	CurrentPuzzle = 5;
	EscapeTime = GetWorld()->GetTimeSeconds();
	ClearGlow();

	const float TotalSeconds = GetElapsedSeconds();
	Record(TEXT("escaped"), { { TEXT("total_sec"), FString::Printf(TEXT("%.1f"), TotalSeconds) }, { TEXT("hints_used"), Itoa(HintsUsed) } });
	FString CsvPath;
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (USessionAnalyticsSubsystem* Analytics = GameInstance->GetSubsystem<USessionAnalyticsSubsystem>())
		{
			CsvPath = FPaths::ConvertRelativePathToFull(Analytics->GetOutputFilePath());
			FPaths::MakePlatformFilename(CsvPath);
			UE_LOG(LogEscapeRoom, Display, TEXT("Escaped in %.1f s, %d hints -> %s"), TotalSeconds, HintsUsed, *CsvPath);
			Analytics->EndSession(TEXT("Escaped"));
		}
	}

	FNumberFormattingOptions OneDecimal;
	OneDecimal.SetMaximumFractionalDigits(1);
	// Set directly: ShowMessage ignores messages once escaped, so this one stays on screen.
	// The full CSV path is shown so playtesters can find the file to send.
	Message = FText::Format(LOCTEXT("Escaped", "탈출 성공! {0}초 · 힌트 {1}회\n결과 파일: {2}\nEsc를 누르면 종료돼요."),
		FText::AsNumber(TotalSeconds, &OneDecimal), HintsUsed, FText::FromString(CsvPath));
	MessageUntil = GetWorld()->GetTimeSeconds() + 3600.0;
}

// ---------------------------------------------------------------------------------------------
// Hints and lights

const FEscapePuzzleConfig* AEscapeRoomManager::GetConfig(int32 Puzzle) const
{
	return Puzzles.IsValidIndex(Puzzle - 1) ? &Puzzles[Puzzle - 1] : nullptr;
}

void AEscapeRoomManager::ShowHint(int32 Level)
{
	const FEscapePuzzleConfig* Config = GetConfig(CurrentPuzzle);
	HintLevel = Level;
	if (!Config)
	{
		return;
	}
	++HintsUsed;
	Record(TEXT("hint_shown"), { { TEXT("puzzle"), Itoa(CurrentPuzzle) }, { TEXT("level"), Itoa(Level) } });

	AddGlowTargets(Level == 1 ? Config->Hint1Tag : Config->Hint2Tag, Level == 2 && Config->bHint2Sequential);
	const FText& Line = Level == 1 ? Config->Hint1Message : Config->Hint2Message;
	if (!Line.IsEmpty())
	{
		HintMessage = Line;
		HintMessageUntil = GetWorld()->GetTimeSeconds() + HintMessageSeconds;
	}
}

void AEscapeRoomManager::AddGlowTargets(FName Tag, bool bSequential)
{
	if (Tag.IsNone())
	{
		return;
	}

	// Group the tagged parts per actor: an actor tag lights the whole actor, a component tag only that part.
	TArray<TPair<AActor*, TArray<UPrimitiveComponent*>>> Groups;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		const bool bWholeActor = It->ActorHasTag(Tag);
		TArray<UPrimitiveComponent*> Components;
		It->GetComponents(Components);
		TArray<UPrimitiveComponent*> Parts;
		for (UPrimitiveComponent* Component : Components)
		{
			const bool bGlowable = Component->IsA<UMeshComponent>() || Component->IsA<UTextRenderComponent>();
			if (bGlowable && (bWholeActor || Component->ComponentHasTag(Tag)))
			{
				Parts.Add(Component);
			}
		}
		if (Parts.Num() > 0)
		{
			Groups.Emplace(*It, MoveTemp(Parts));
		}
	}

	if (bSequential)
	{
		// Frames light up smallest first; anything else keeps a stable order by name.
		Groups.Sort([](const TPair<AActor*, TArray<UPrimitiveComponent*>>& L, const TPair<AActor*, TArray<UPrimitiveComponent*>>& R)
		{
			const AEscapeFrame* FrameL = Cast<AEscapeFrame>(L.Key);
			const AEscapeFrame* FrameR = Cast<AEscapeFrame>(R.Key);
			if (FrameL && FrameR)
			{
				return FrameL->GetArea() < FrameR->GetArea();
			}
			return L.Key->GetName() < R.Key->GetName();
		});
	}

	for (int32 GroupIndex = 0; GroupIndex < Groups.Num(); ++GroupIndex)
	{
		for (UPrimitiveComponent* Component : Groups[GroupIndex].Value)
		{
			// A sequential hint replaces an always-on glow of the same part.
			GlowTargets.RemoveAll([Component](const FGlowTarget& Target) { return Target.Component.Get() == Component; });
			GlowTargets.Add({ Component, bSequential ? GroupIndex : -1 });
		}
	}
	if (bSequential)
	{
		SequenceCount = Groups.Num();
	}
}

void AEscapeRoomManager::SetGlow(UPrimitiveComponent* Component, float Amount)
{
	if (UTextRenderComponent* Text = Cast<UTextRenderComponent>(Component))
	{
		const FColor& Base = TextBaseColors.FindOrAdd(TWeakObjectPtr<UTextRenderComponent>(Text), Text->TextRenderColor);
		const float Alpha = HintGlowStrength > 0.f ? FMath::Clamp(Amount / HintGlowStrength, 0.f, 1.f) : 0.f;
		Text->SetTextRenderColor(FLinearColor::LerpUsingHSV(FLinearColor(Base), FLinearColor(TextGlowColor), Alpha).ToFColor(true));
	}
	else if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(Component))
	{
		EscapeVisuals::SetHintGlow(MeshComponent, Amount);
	}
}

void AEscapeRoomManager::ClearGlow()
{
	for (const FGlowTarget& Target : GlowTargets)
	{
		SetGlow(Target.Component.Get(), 0.f);
	}
	for (const TPair<TWeakObjectPtr<UTextRenderComponent>, FColor>& Pair : TextBaseColors)
	{
		if (UTextRenderComponent* Text = Pair.Key.Get())
		{
			Text->SetTextRenderColor(Pair.Value);
		}
	}
	GlowTargets.Reset();
	TextBaseColors.Reset();
	SequenceCount = 0;
}

void AEscapeRoomManager::ApplyLights()
{
	// After puzzle 3 the last puzzle's lights stay on.
	const int32 LitPuzzle = FMath::Min(CurrentPuzzle, Puzzles.Num());
	for (const FPuzzleLight& Entry : PuzzleLights)
	{
		if (ULightComponent* Light = Entry.Light.Get())
		{
			Light->SetIntensity(Entry.BaseIntensity * (Entry.Puzzle == LitPuzzle ? 1.f : InactiveLightScale));
		}
	}
}

// ---------------------------------------------------------------------------------------------
// UI and analytics

void AEscapeRoomManager::ShowMessage(const FText& InMessage)
{
	if (InMessage.IsEmpty() || bEscaped)
	{
		return;
	}
	Message = InMessage;
	MessageUntil = GetWorld()->GetTimeSeconds() + MessageSeconds;
}

bool AEscapeRoomManager::IsHintMessageVisible() const
{
	return !bEscaped && GetWorld() && GetWorld()->GetTimeSeconds() < HintMessageUntil;
}

bool AEscapeRoomManager::IsMessageVisible() const
{
	return GetWorld() && GetWorld()->GetTimeSeconds() < MessageUntil;
}

float AEscapeRoomManager::GetElapsedSeconds() const
{
	const double End = bEscaped ? EscapeTime : (GetWorld() ? GetWorld()->GetTimeSeconds() : StartTime);
	return static_cast<float>(End - StartTime);
}

FText AEscapeRoomManager::GetStatusText() const
{
	FText Status;
	if (CurrentPuzzle >= 1 && CurrentPuzzle <= 3)
	{
		Status = FText::Format(LOCTEXT("StatusPuzzle", "퍼즐 {0}/3"), CurrentPuzzle);
	}
	else
	{
		Status = CurrentPuzzle == 4 ? LOCTEXT("StatusDoor", "문을 열자!") : LOCTEXT("StatusEscaped", "탈출 완료");
	}
	if (!bShowElapsedTime)
	{
		return Status;
	}
	const int32 Seconds = FMath::FloorToInt(GetElapsedSeconds());
	const FText Clock = FText::FromString(FString::Printf(TEXT("%02d:%02d"), Seconds / 60, Seconds % 60));
	return FText::Format(LOCTEXT("StatusWithClock", "{0}   {1}"), Status, Clock);
}

FText AEscapeRoomManager::GetInventoryText() const
{
	TArray<FText> Items;
	if (bHasDrawerKey && !bHasPalette)
	{
		Items.Add(LOCTEXT("ItemDrawerKey", "서랍 열쇠"));
	}
	if (bHasPalette && CurrentPuzzle <= 2)
	{
		Items.Add(LOCTEXT("ItemPalette", "팔레트"));
	}
	if (bHasDoorKey && !bEscaped)
	{
		Items.Add(LOCTEXT("ItemDoorKey", "문 열쇠"));
	}
	if (Items.Num() == 0)
	{
		return FText::GetEmpty();
	}
	return FText::Format(LOCTEXT("Inventory", "소지품: {0}"), FText::Join(FText::FromString(TEXT(", ")), Items));
}

FText AEscapeRoomManager::GetPaletteText() const
{
	if (!bHasPalette || CurrentPuzzle != 2)
	{
		return FText::GetEmpty();
	}
	const AEscapePaint* First = PaletteSlots.Num() > 0 ? PaletteSlots[0].Get() : nullptr;
	return FText::Format(LOCTEXT("Palette", "팔레트: [ {0} ] + [ {1} ]"),
		First ? First->ColorName : LOCTEXT("EmptySlot", "  "), LOCTEXT("EmptySlot", "  "));
}

void AEscapeRoomManager::Record(FName Event, const TMap<FString, FString>& Params) const
{
	UE_LOG(LogEscapeRoom, Log, TEXT("%s"), *Event.ToString());
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (USessionAnalyticsSubsystem* Analytics = GameInstance->GetSubsystem<USessionAnalyticsSubsystem>())
		{
			Analytics->RecordEvent(Event, Params);
		}
	}
}

#undef LOCTEXT_NAMESPACE
