#include "EscapeRoom/EscapeFrame.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "EscapeRoom/EscapeRoomManager.h"
#include "EscapeRoom/EscapeVisuals.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "EscapeRoom"

namespace
{
	constexpr float BorderWidth = 5.f;
	const FLinearColor WoodColor(0.16f, 0.09f, 0.05f);
	const FLinearColor PlateColor(0.04f, 0.035f, 0.03f);
	const FLinearColor KeyColor(1.f, 0.72f, 0.15f);
}

AEscapeFrame::AEscapeFrame()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	LiftRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LiftRoot"));
	LiftRoot->SetupAttachment(SceneRoot);

	// Mesh (from the base class) is the wooden border.
	Mesh->SetupAttachment(LiftRoot);

	Canvas = EscapeVisuals::CreatePart(this, TEXT("Canvas"), EscapeVisuals::Cube);
	Canvas->SetupAttachment(LiftRoot);

	Objects = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Objects"));
	Objects->SetupAttachment(LiftRoot);
	Objects->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(EscapeVisuals::Sphere);
	if (SphereMesh.Succeeded())
	{
		Objects->SetStaticMesh(SphereMesh.Object);
	}

	LabelPlate = EscapeVisuals::CreatePart(this, TEXT("LabelPlate"), EscapeVisuals::Cube);
	LabelPlate->SetupAttachment(SceneRoot);
	LabelPlate->ComponentTags.Add(TEXT("P1H2"));

	LabelText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LabelText"));
	LabelText->SetupAttachment(SceneRoot);
	LabelText->SetHorizontalAlignment(EHTA_Center);
	LabelText->SetVerticalAlignment(EVRTA_TextCenter);
	LabelText->SetWorldSize(6.f);
	LabelText->SetTextRenderColor(FColor(235, 225, 200));
	LabelText->ComponentTags.Add(TEXT("P1H2"));

	Key = EscapeVisuals::CreatePart(this, TEXT("Key"), EscapeVisuals::Cube);
	Key->SetupAttachment(SceneRoot);
	Key->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Key->SetVisibility(false);

	Tags.Add(TEXT("P3H1"));
	Tags.Add(TEXT("P3H2"));

	ObjectId = TEXT("frame");
	PuzzleIds = { 1, 3 };
	BaseColor = WoodColor;
	DisplayName = LOCTEXT("FrameName", "액자");
	InspectMessage = LOCTEXT("FrameInspect", "그림을 자세히 들여다본다.");
	EmptyMessage = LOCTEXT("FrameEmpty", "액자 뒤에는 아무것도 없다.");
}

void AEscapeFrame::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Layout();
}

void AEscapeFrame::BeginPlay()
{
	Super::BeginPlay();
	Layout();
}

void AEscapeFrame::Layout()
{
	const float W = FMath::Max(FrameSize.X, 10.f);
	const float H = FMath::Max(FrameSize.Y, 10.f);

	// Engine cube is 100 cm, pivot in the center. X = depth out of the wall.
	Mesh->SetRelativeLocation(FVector(2.f, 0.f, 0.f));
	Mesh->SetRelativeScale3D(FVector(0.04f, (W + 2.f * BorderWidth) / 100.f, (H + 2.f * BorderWidth) / 100.f));

	Canvas->SetRelativeLocation(FVector(4.2f, 0.f, 0.f));
	Canvas->SetRelativeScale3D(FVector(0.005f, W / 100.f, H / 100.f));
	EscapeVisuals::SetColor(Canvas, CanvasColor);

	// Countable objects: one per grid cell with a stable jitter, so the layout does not change between runs.
	Objects->ClearInstances();
	const int32 Count = FMath::Clamp(ObjectCount, 0, 9);
	if (Count > 0)
	{
		const int32 Cols = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Count)));
		const int32 Rows = FMath::DivideAndRoundUp(Count, Cols);
		const float Margin = FMath::Min(W, H) * 0.12f;
		const float CellW = (W - 2.f * Margin) / Cols;
		const float CellH = (H - 2.f * Margin) / Rows;
		const float Diameter = FMath::Clamp(FMath::Min(CellW, CellH) * 0.55f, 3.f, 10.f);
		FRandomStream Random(GetTypeHash(ObjectId) + Count);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const int32 Col = Index % Cols;
			const int32 Row = Index / Cols;
			const float Y = -W * 0.5f + Margin + CellW * (Col + 0.5f) + Random.FRandRange(-0.2f, 0.2f) * CellW;
			const float Z = H * 0.5f - Margin - CellH * (Row + 0.5f) + Random.FRandRange(-0.2f, 0.2f) * CellH;
			const FVector Scale(Diameter * 0.5f / 100.f, Diameter / 100.f, Diameter / 100.f);
			Objects->AddInstance(FTransform(FRotator::ZeroRotator, FVector(4.6f, Y, Z), Scale));
		}
	}
	EscapeVisuals::SetColor(Objects, ObjectColor);

	const float LabelZ = -(H * 0.5f + BorderWidth + 9.f);
	LabelPlate->SetRelativeLocation(FVector(0.6f, 0.f, LabelZ));
	LabelPlate->SetRelativeScale3D(FVector(0.012f, 0.32f, 0.09f));
	EscapeVisuals::SetColor(LabelPlate, PlateColor);

	LabelText->SetRelativeLocation(FVector(1.4f, 0.f, LabelZ));
	LabelText->SetText(FText::FromString(DateLabel));

	Key->SetRelativeLocation(FVector(1.f, 0.f, 0.f));
	Key->SetRelativeScale3D(FVector(0.01f, 0.10f, 0.035f));
	EscapeVisuals::SetColor(Key, KeyColor, 0.6f);

	LiftRoot->SetRelativeLocation(FVector(0.f, 0.f, GetLiftHeight() * LiftAlpha));
}

void AEscapeFrame::Interact(AEscapeCharacter* Player, UPrimitiveComponent* HitComponent)
{
	if (AEscapeRoomManager* Manager = GetManager())
	{
		Manager->HandleFrame(this);
	}
}

void AEscapeFrame::Lift(bool bRevealKey)
{
	LiftTarget = 1.f;
	DropAt = GetWorld()->GetTimeSeconds() + (bRevealKey ? 1.8 : 0.9);
	Key->SetVisibility(bRevealKey);
	SetActorTickEnabled(true);
}

void AEscapeFrame::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (DropAt >= 0.0 && GetWorld()->GetTimeSeconds() >= DropAt)
	{
		DropAt = -1.0;
		LiftTarget = 0.f;
		Key->SetVisibility(false);
	}

	LiftAlpha = FMath::FInterpConstantTo(LiftAlpha, LiftTarget, DeltaSeconds, 1.f / LiftSeconds);
	const float Eased = FMath::InterpEaseInOut(0.f, 1.f, LiftAlpha, 2.f);
	LiftRoot->SetRelativeLocation(FVector(0.f, 0.f, GetLiftHeight() * Eased));

	if (DropAt < 0.0 && FMath::IsNearlyEqual(LiftAlpha, LiftTarget))
	{
		SetActorTickEnabled(false);
	}
}

#undef LOCTEXT_NAMESPACE
