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
	constexpr float DialSpacing = 16.f;
	constexpr float DialZ = 10.f;
	/** Square plate behind each dial; leaves a 2 cm gap between neighbours. */
	constexpr float PlateSize = 14.f;
	constexpr float OpenYaw = -100.f;
	constexpr float SilhouetteBottom = 19.f;
	constexpr float SilhouetteLine = 1.f;
	constexpr float DotDiameter = 3.f;
	/** Dot offsets (Y, Z) from the silhouette centre. Two dots on every silhouette: no code digit is 2, so they are never read as a count. */
	const FVector2D DotOffsets[AEscapeSafe::DotsPerSilhouette] = { FVector2D(2.2f, 2.6f), FVector2D(-2.2f, -2.4f) };

	/** Y of dial (and silhouette) Index relative to the hinge. Seen from the front local +Y is on the left, so slot 0 gets the highest Y. */
	float SlotY(int32 Index)
	{
		return DoorWidth * 0.5f - (Index - (AEscapeSafe::NumDials - 1) * 0.5f) * DialSpacing;
	}
	const FLinearColor SafeColor(0.14f, 0.16f, 0.17f);
	const FLinearColor DialColor(0.05f, 0.05f, 0.05f);
	const FLinearColor MetalColor(0.7f, 0.68f, 0.6f);
	// Silver, matching the door key icon in the HUD (the drawer key is gold).
	const FLinearColor KeyColor(0.75f, 0.75f, 0.8f);
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
		Dial->SetRelativeLocation(FVector(4.5f, Y, DialZ));
		Dial->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
		Dial->SetRelativeScale3D(FVector(0.10f, 0.10f, 0.03f));
		Dials.Add(Dial);

		UStaticMeshComponent* Plate = EscapeVisuals::CreatePart(this, *FString::Printf(TEXT("DialPlate%d"), Index), EscapeVisuals::Cube);
		Plate->SetupAttachment(DoorHinge);
		Plate->SetRelativeLocation(FVector(3.2f, Y, DialZ));
		Plate->SetRelativeScale3D(FVector(0.004f, PlateSize / 100.f, PlateSize / 100.f));
		Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DialPlates.Add(Plate);

		UTextRenderComponent* Text = CreateDefaultSubobject<UTextRenderComponent>(*FString::Printf(TEXT("DialText%d"), Index));
		Text->SetupAttachment(DoorHinge);
		Text->SetRelativeLocation(FVector(6.3f, Y, DialZ));
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

		for (int32 Dot = 0; Dot < DotsPerSilhouette; ++Dot)
		{
			UStaticMeshComponent* DotPart = EscapeVisuals::CreatePart(this, *FString::Printf(TEXT("SilhouetteDot%d_%d"), Index, Dot), EscapeVisuals::Sphere);
			DotPart->SetupAttachment(DoorHinge);
			DotPart->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			SilhouetteDots.Add(DotPart);
		}
	}

	Handle = EscapeVisuals::CreatePart(this, TEXT("Handle"), EscapeVisuals::Cube);
	Handle->SetupAttachment(DoorHinge);
	Handle->SetRelativeLocation(FVector(5.f, DoorWidth * 0.5f, -16.f));
	Handle->SetRelativeScale3D(FVector(0.05f, 0.24f, 0.05f));

	// v5: key-shaped door key (bow, shaft, tooth) on the body face behind the door, picked up with E.
	DoorKey = EscapeVisuals::CreatePart(this, TEXT("DoorKey"), EscapeVisuals::Cube);
	DoorKey->SetupAttachment(SceneRoot);
	DoorKey->SetRelativeLocation(FVector(30.7f, 2.f, 45.f));
	DoorKey->SetRelativeScale3D(FVector(0.01f, 0.10f, 0.022f));

	DoorKeyBow = EscapeVisuals::CreatePart(this, TEXT("DoorKeyBow"), EscapeVisuals::Cylinder);
	DoorKeyBow->SetupAttachment(SceneRoot);
	DoorKeyBow->SetRelativeLocation(FVector(30.7f, -5.5f, 45.f));
	DoorKeyBow->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
	DoorKeyBow->SetRelativeScale3D(FVector(0.06f, 0.06f, 0.01f));

	DoorKeyTooth = EscapeVisuals::CreatePart(this, TEXT("DoorKeyTooth"), EscapeVisuals::Cube);
	DoorKeyTooth->SetupAttachment(SceneRoot);
	DoorKeyTooth->SetRelativeLocation(FVector(30.7f, 5.5f, 43.f));
	DoorKeyTooth->SetRelativeScale3D(FVector(0.01f, 0.03f, 0.035f));

	for (UStaticMeshComponent* Part : { DoorKey.Get(), DoorKeyBow.Get(), DoorKeyTooth.Get() })
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetVisibility(false);
	}

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
	for (UStaticMeshComponent* Part : { DoorKey.Get(), DoorKeyBow.Get(), DoorKeyTooth.Get() })
	{
		EscapeVisuals::SetColor(Part, KeyColor, 0.6f);
	}
	for (int32 Index = 0; Index < SilhouetteOutlines.Num(); ++Index)
	{
		EscapeVisuals::SetColor(SilhouetteOutlines[Index], SilhouetteColor);
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
	// Dial n belongs to the n-th smallest frame (unchanged from v2), but since v4 the link is made by colour, not size:
	// every silhouette has the same size, the plate behind the dial and the silhouette use the painting's background
	// colour, and the dots use the colour of the things painted on it.
	Frames.Sort([](const AEscapeFrame& L, const AEscapeFrame& R) { return L.GetArea() < R.GetArea(); });

	const FVector2D Size = SilhouetteSize;
	const float CenterZ = SilhouetteBottom + Size.Y * 0.5f;
	for (int32 Index = 0; Index < SilhouetteOutlines.Num(); ++Index)
	{
		const AEscapeFrame* Frame = Frames.IsValidIndex(Index) ? Frames[Index] : nullptr;
		const bool bHasFrame = Frame != nullptr;
		SilhouetteOutlines[Index]->SetVisibility(bHasFrame);
		SilhouetteFills[Index]->SetVisibility(bHasFrame);
		if (DialPlates.IsValidIndex(Index))
		{
			EscapeVisuals::SetColor(DialPlates[Index], bHasFrame ? Frame->CanvasColor : SafeColor * 1.3f);
		}
		for (int32 Dot = 0; Dot < DotsPerSilhouette; ++Dot)
		{
			if (UStaticMeshComponent* DotPart = SilhouetteDots.IsValidIndex(Index * DotsPerSilhouette + Dot) ? SilhouetteDots[Index * DotsPerSilhouette + Dot].Get() : nullptr)
			{
				DotPart->SetVisibility(bHasFrame);
				DotPart->SetRelativeLocation(FVector(3.8f, SlotY(Index) + DotOffsets[Dot].X, CenterZ + DotOffsets[Dot].Y));
				DotPart->SetRelativeScale3D(FVector(0.004f, DotDiameter / 100.f, DotDiameter / 100.f));
				if (bHasFrame)
				{
					EscapeVisuals::SetColor(DotPart, Frame->ObjectColor);
				}
			}
		}
		if (!bHasFrame)
		{
			continue;
		}
		SilhouetteOutlines[Index]->SetRelativeLocation(FVector(3.2f, SlotY(Index), CenterZ));
		SilhouetteOutlines[Index]->SetRelativeScale3D(FVector(0.004f, Size.X / 100.f, Size.Y / 100.f));
		SilhouetteFills[Index]->SetRelativeLocation(FVector(3.5f, SlotY(Index), CenterZ));
		EscapeVisuals::SetColor(SilhouetteFills[Index], Frame->CanvasColor);
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
	if (bKeyInside && IsDoorKeyPart(HitComponent))
	{
		return LOCTEXT("TakeDoorKeyPrompt", "열쇠 집기");
	}
	return DisplayName;
}

bool AEscapeSafe::IsDoorKeyPart(const UPrimitiveComponent* Component) const
{
	return Component && (Component == DoorKey || Component == DoorKeyBow || Component == DoorKeyTooth);
}

void AEscapeSafe::SetDoorKeyShown(bool bShown)
{
	for (UStaticMeshComponent* Part : { DoorKey.Get(), DoorKeyBow.Get(), DoorKeyTooth.Get() })
	{
		Part->SetVisibility(bShown);
		Part->SetCollisionEnabled(bShown ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		Part->SetCollisionResponseToAllChannels(ECR_Block);
		if (!bShown)
		{
			EscapeVisuals::SetHintGlow(Part, 0.f);
		}
	}
}

void AEscapeSafe::TakeDoorKey()
{
	bKeyInside = false;
	SetDoorKeyShown(false);
}

void AEscapeSafe::Interact(AEscapeCharacter* Player, UPrimitiveComponent* HitComponent)
{
	AEscapeRoomManager* Manager = GetManager();
	const int32 Dial = GetDialIndex(HitComponent);
	if (Dial != INDEX_NONE && !bOpen)
	{
		DialValues[Dial] = (DialValues[Dial] + 1) % 10;
		UpdateDialTexts();
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
	if (bKeyInside && IsDoorKeyPart(HitComponent))
	{
		if (Manager)
		{
			Manager->HandleSafeKey(this);
		}
		return;
	}
	Super::Interact(Player, HitComponent);
}

void AEscapeSafe::OpenDoor()
{
	bOpen = true;
	bKeyInside = true;
	SetDoorKeyShown(true);
	SetActorTickEnabled(true);
}

void AEscapeSafe::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	OpenAlpha = FMath::FInterpConstantTo(OpenAlpha, 1.f, DeltaSeconds, 1.2f);
	DoorHinge->SetRelativeRotation(FRotator(0.f, OpenYaw * FMath::InterpEaseOut(0.f, 1.f, OpenAlpha, 2.f), 0.f));
	if (bKeyInside)
	{
		// Soft pulse until the key is taken, same as the drawer key behind the frame.
		const float Glow = 1.5f + 1.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 2.0 * UE_TWO_PI);
		for (UStaticMeshComponent* Part : { DoorKey.Get(), DoorKeyBow.Get(), DoorKeyTooth.Get() })
		{
			EscapeVisuals::SetHintGlow(Part, Glow);
		}
		return;
	}
	if (OpenAlpha >= 1.f)
	{
		SetActorTickEnabled(false);
	}
}

#undef LOCTEXT_NAMESPACE
