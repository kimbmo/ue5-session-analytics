#include "EscapeRoom/EscapeSafe.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EngineUtils.h"
#include "EscapeRoom/EscapeFrame.h"
#include "EscapeRoom/EscapeRoomManager.h"
#include "EscapeRoom/EscapeVisuals.h"

#define LOCTEXT_NAMESPACE "EscapeRoom"

namespace
{
	// Body: 60 deep (X), 70 wide (Y), 80 tall. The door hinge is on the front-left edge.
	constexpr float DoorWidth = 70.f;
	constexpr float DialSpacing = 13.f;
	constexpr float OpenYaw = -100.f;
	constexpr float SilhouetteBottom = 17.f;
	constexpr float SilhouetteLine = 1.f;

	/** Y of dial (and silhouette) Index relative to the hinge. Seen from the front local +Y is on the left, so slot 0 gets the highest Y. */
	float SlotY(int32 Index)
	{
		return DoorWidth * 0.5f - (Index - (AEscapeSafe::NumDials - 1) * 0.5f) * DialSpacing;
	}
	const FLinearColor SafeColor(0.14f, 0.16f, 0.17f);
	const FLinearColor DialColor(0.05f, 0.05f, 0.05f);
	const FLinearColor MetalColor(0.7f, 0.68f, 0.6f);
	const FLinearColor KeyColor(1.f, 0.72f, 0.15f);
}

AEscapeSafe::AEscapeSafe()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Mesh (from the base class) is the body.
	Mesh->SetupAttachment(SceneRoot);
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	Mesh->SetRelativeScale3D(FVector(0.6f, 0.7f, 0.8f));

	DoorHinge = CreateDefaultSubobject<USceneComponent>(TEXT("DoorHinge"));
	DoorHinge->SetupAttachment(SceneRoot);
	DoorHinge->SetRelativeLocation(FVector(30.f, -DoorWidth * 0.5f, 40.f));

	SafeDoor = EscapeVisuals::CreatePart(this, TEXT("SafeDoor"), EscapeVisuals::Cube);
	SafeDoor->SetupAttachment(DoorHinge);
	SafeDoor->SetRelativeLocation(FVector(1.5f, DoorWidth * 0.5f, 0.f));
	SafeDoor->SetRelativeScale3D(FVector(0.03f, 0.66f, 0.76f));

	DialValues.Init(0, NumDials);
	for (int32 Index = 0; Index < NumDials; ++Index)
	{
		const float Y = SlotY(Index);

		UStaticMeshComponent* Dial = EscapeVisuals::CreatePart(this, *FString::Printf(TEXT("Dial%d"), Index), EscapeVisuals::Cylinder);
		Dial->SetupAttachment(DoorHinge);
		Dial->SetRelativeLocation(FVector(4.5f, Y, 10.f));
		Dial->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
		Dial->SetRelativeScale3D(FVector(0.10f, 0.10f, 0.03f));
		Dials.Add(Dial);

		UTextRenderComponent* Text = CreateDefaultSubobject<UTextRenderComponent>(*FString::Printf(TEXT("DialText%d"), Index));
		Text->SetupAttachment(DoorHinge);
		Text->SetRelativeLocation(FVector(6.3f, Y, 10.f));
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		Text->SetWorldSize(7.f);
		Text->SetTextRenderColor(FColor::White);
		DialTexts.Add(Text);

		UStaticMeshComponent* Outline = EscapeVisuals::CreatePart(this, *FString::Printf(TEXT("SilhouetteOutline%d"), Index), EscapeVisuals::Cube);
		Outline->SetupAttachment(DoorHinge);
		Outline->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SilhouetteOutlines.Add(Outline);

		UStaticMeshComponent* Fill = EscapeVisuals::CreatePart(this, *FString::Printf(TEXT("SilhouetteFill%d"), Index), EscapeVisuals::Cube);
		Fill->SetupAttachment(DoorHinge);
		Fill->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SilhouetteFills.Add(Fill);
	}

	Handle = EscapeVisuals::CreatePart(this, TEXT("Handle"), EscapeVisuals::Cube);
	Handle->SetupAttachment(DoorHinge);
	Handle->SetRelativeLocation(FVector(5.f, DoorWidth * 0.5f, -16.f));
	Handle->SetRelativeScale3D(FVector(0.05f, 0.24f, 0.05f));

	DoorKey = EscapeVisuals::CreatePart(this, TEXT("DoorKey"), EscapeVisuals::Cube);
	DoorKey->SetupAttachment(SceneRoot);
	DoorKey->SetRelativeLocation(FVector(33.f, 0.f, 45.f));
	DoorKey->SetRelativeScale3D(FVector(0.04f, 0.12f, 0.04f));
	DoorKey->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DoorKey->SetVisibility(false);

	ObjectId = TEXT("safe");
	PuzzleIds = { 3 };
	BaseColor = SafeColor;
	DisplayName = LOCTEXT("SafeName", "금고");
	InspectMessage = LOCTEXT("SafeInspect", "네 자리 다이얼 금고다. 다이얼을 맞추고 손잡이를 당기자.");
}

void AEscapeSafe::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyColors();
}

void AEscapeSafe::BeginPlay()
{
	Super::BeginPlay();
	ApplyColors();
}

void AEscapeSafe::ApplyColors()
{
	EscapeVisuals::SetColor(SafeDoor, SafeColor * 1.3f);
	for (UStaticMeshComponent* Dial : Dials)
	{
		EscapeVisuals::SetColor(Dial, DialColor);
	}
	EscapeVisuals::SetColor(Handle, MetalColor);
	EscapeVisuals::SetColor(DoorKey, KeyColor, 0.6f);
	for (int32 Index = 0; Index < SilhouetteOutlines.Num(); ++Index)
	{
		EscapeVisuals::SetColor(SilhouetteOutlines[Index], SilhouetteColor);
		EscapeVisuals::SetColor(SilhouetteFills[Index], SafeColor * 1.3f);
	}
	UpdateDialTexts();
	UpdateSilhouettes();
}

void AEscapeSafe::UpdateSilhouettes()
{
	TArray<const AEscapeFrame*> Frames;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AEscapeFrame> It(World); It; ++It)
		{
			Frames.Add(*It);
		}
	}
	Frames.Sort([](const AEscapeFrame& L, const AEscapeFrame& R) { return L.GetArea() < R.GetArea(); });

	// One scale for all slots so the relative sizes match the real frames.
	FVector2D Largest(1.f, 1.f);
	for (const AEscapeFrame* Frame : Frames)
	{
		Largest = FVector2D(FMath::Max(Largest.X, Frame->FrameSize.X), FMath::Max(Largest.Y, Frame->FrameSize.Y));
	}
	const float Scale = FMath::Min(SilhouetteMaxSize.X / Largest.X, SilhouetteMaxSize.Y / Largest.Y);

	for (int32 Index = 0; Index < SilhouetteOutlines.Num(); ++Index)
	{
		const bool bHasFrame = Frames.IsValidIndex(Index);
		SilhouetteOutlines[Index]->SetVisibility(bHasFrame);
		SilhouetteFills[Index]->SetVisibility(bHasFrame);
		if (!bHasFrame)
		{
			continue;
		}
		const FVector2D Size = Frames[Index]->FrameSize * Scale;
		const float CenterZ = SilhouetteBottom + Size.Y * 0.5f;
		SilhouetteOutlines[Index]->SetRelativeLocation(FVector(3.2f, SlotY(Index), CenterZ));
		SilhouetteOutlines[Index]->SetRelativeScale3D(FVector(0.004f, Size.X / 100.f, Size.Y / 100.f));
		SilhouetteFills[Index]->SetRelativeLocation(FVector(3.5f, SlotY(Index), CenterZ));
		SilhouetteFills[Index]->SetRelativeScale3D(FVector(0.004f,
			FMath::Max(Size.X - 2.f * SilhouetteLine, 0.5f) / 100.f, FMath::Max(Size.Y - 2.f * SilhouetteLine, 0.5f) / 100.f));
	}
}

void AEscapeSafe::UpdateDialTexts()
{
	for (int32 Index = 0; Index < DialTexts.Num(); ++Index)
	{
		DialTexts[Index]->SetText(FText::AsNumber(GetDialValue(Index)));
	}
}

int32 AEscapeSafe::GetDialIndex(const UPrimitiveComponent* Component) const
{
	for (int32 Index = 0; Index < Dials.Num(); ++Index)
	{
		if (Dials[Index] == Component)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

FString AEscapeSafe::GetCode() const
{
	FString Code;
	for (int32 Value : DialValues)
	{
		Code.AppendInt(Value);
	}
	return Code;
}

FText AEscapeSafe::GetPrompt(const UPrimitiveComponent* HitComponent) const
{
	const int32 Dial = GetDialIndex(HitComponent);
	if (Dial != INDEX_NONE)
	{
		return FText::Format(LOCTEXT("DialPrompt", "다이얼 {0} 돌리기 (현재 {1})"), Dial + 1, GetDialValue(Dial));
	}
	if (HitComponent == Handle)
	{
		return FText::Format(LOCTEXT("HandlePrompt", "손잡이 당기기 ({0})"), FText::FromString(GetCode()));
	}
	return DisplayName;
}

void AEscapeSafe::Interact(AEscapeCharacter* Player, UPrimitiveComponent* HitComponent)
{
	AEscapeRoomManager* Manager = GetManager();
	const int32 Dial = GetDialIndex(HitComponent);
	if (Dial != INDEX_NONE && !bOpen)
	{
		DialValues[Dial] = (DialValues[Dial] + 1) % 10;
		UpdateDialTexts();
		if (Manager)
		{
			Manager->NotifyProgress(3);
		}
		return;
	}
	if (HitComponent == Handle)
	{
		if (Manager)
		{
			Manager->HandleSafeHandle(this, GetCode());
		}
		return;
	}
	Super::Interact(Player, HitComponent);
}

void AEscapeSafe::OpenDoor()
{
	bOpen = true;
	DoorKey->SetVisibility(true);
	SetActorTickEnabled(true);
}

void AEscapeSafe::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	OpenAlpha = FMath::FInterpConstantTo(OpenAlpha, 1.f, DeltaSeconds, 1.2f);
	DoorHinge->SetRelativeRotation(FRotator(0.f, OpenYaw * FMath::InterpEaseOut(0.f, 1.f, OpenAlpha, 2.f), 0.f));
	if (OpenAlpha >= 1.f)
	{
		SetActorTickEnabled(false);
	}
}

#undef LOCTEXT_NAMESPACE
