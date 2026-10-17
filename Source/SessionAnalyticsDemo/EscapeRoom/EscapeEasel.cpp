#include "EscapeRoom/EscapeEasel.h"

#include "Components/StaticMeshComponent.h"
#include "EscapeRoom/EscapeVisuals.h"

#define LOCTEXT_NAMESPACE "EscapeRoom"

namespace
{
	const FLinearColor WoodColor(0.45f, 0.3f, 0.16f);
	const FLinearColor CanvasColor(0.9f, 0.88f, 0.8f);
	const FLinearColor PatchColor(1.f, 1.f, 1.f);

	struct FPaintedBlock { FVector Location; FVector Scale; FLinearColor Color; };
	// A few painted color blocks; the top-right cell (local -Y, seen from the front) is left empty.
	const FPaintedBlock PaintedBlocks[] = {
		{ FVector(5.7f, 12.f, 140.f), FVector(0.005f, 0.22f, 0.16f), FLinearColor(0.7f, 0.12f, 0.1f) },
		{ FVector(5.7f, 12.f, 118.f), FVector(0.005f, 0.22f, 0.16f), FLinearColor(0.06f, 0.15f, 0.7f) },
		{ FVector(5.7f, -12.f, 118.f), FVector(0.005f, 0.22f, 0.16f), FLinearColor(0.85f, 0.7f, 0.08f) },
	};
}

AEscapeEasel::AEscapeEasel()
{
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Mesh (from the base class) is the canvas.
	Mesh->SetupAttachment(SceneRoot);
	Mesh->SetRelativeLocation(FVector(4.f, 0.f, 129.f));
	Mesh->SetRelativeScale3D(FVector(0.03f, 0.56f, 0.46f));

	struct FLeg { FVector Location; FRotator Rotation; FVector Scale; };
	const FLeg LegSetup[] = {
		{ FVector(0.f, -24.f, 80.f), FRotator(0.f, 0.f, -6.f), FVector(0.04f, 0.04f, 1.6f) },
		{ FVector(0.f, 24.f, 80.f), FRotator(0.f, 0.f, 6.f), FVector(0.04f, 0.04f, 1.6f) },
		{ FVector(-25.f, 0.f, 72.f), FRotator(-20.f, 0.f, 0.f), FVector(0.04f, 0.04f, 1.5f) },
		{ FVector(6.f, 0.f, 103.f), FRotator::ZeroRotator, FVector(0.08f, 0.62f, 0.03f) },
	};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(LegSetup); ++Index)
	{
		UStaticMeshComponent* Leg = EscapeVisuals::CreatePart(this, *FString::Printf(TEXT("Leg%d"), Index), EscapeVisuals::Cube);
		Leg->SetupAttachment(SceneRoot);
		Leg->SetRelativeLocation(LegSetup[Index].Location);
		Leg->SetRelativeRotation(LegSetup[Index].Rotation);
		Leg->SetRelativeScale3D(LegSetup[Index].Scale);
		Legs.Add(Leg);
	}

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(PaintedBlocks); ++Index)
	{
		UStaticMeshComponent* Block = EscapeVisuals::CreatePart(this, *FString::Printf(TEXT("Painted%d"), Index), EscapeVisuals::Cube);
		Block->SetupAttachment(SceneRoot);
		Block->SetRelativeLocation(PaintedBlocks[Index].Location);
		Block->SetRelativeScale3D(PaintedBlocks[Index].Scale);
		Block->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PaintedParts.Add(Block);
	}

	EmptyPatch = EscapeVisuals::CreatePart(this, TEXT("EmptyPatch"), EscapeVisuals::Cube);
	EmptyPatch->SetupAttachment(SceneRoot);
	EmptyPatch->SetRelativeLocation(FVector(5.7f, -12.f, 140.f));
	EmptyPatch->SetRelativeScale3D(FVector(0.005f, 0.22f, 0.16f));
	EmptyPatch->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ObjectId = TEXT("easel");
	PuzzleIds = { 2 };
	BaseColor = CanvasColor;
	DisplayName = LOCTEXT("EaselName", "이젤");
	InspectMessage = LOCTEXT("EaselInspect", "그리다 만 그림이다. 오른쪽 위 한 칸만 비어 있다.");
}

void AEscapeEasel::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyColors();
}

void AEscapeEasel::BeginPlay()
{
	Super::BeginPlay();
	ApplyColors();
}

void AEscapeEasel::ApplyColors()
{
	for (UStaticMeshComponent* Leg : Legs)
	{
		EscapeVisuals::SetColor(Leg, WoodColor);
	}
	for (int32 Index = 0; Index < PaintedParts.Num() && Index < UE_ARRAY_COUNT(PaintedBlocks); ++Index)
	{
		EscapeVisuals::SetColor(PaintedParts[Index], PaintedBlocks[Index].Color);
	}
	EscapeVisuals::SetColor(EmptyPatch, bFilled ? FillColor : PatchColor);
}

void AEscapeEasel::FillPatch(const FLinearColor& Color)
{
	bFilled = true;
	FillColor = Color;
	ApplyColors();
}

#undef LOCTEXT_NAMESPACE
