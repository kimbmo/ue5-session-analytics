#include "EscapeRoom/EscapePaint.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EscapeRoom/EscapeRoomManager.h"
#include "EscapeRoom/EscapeVisuals.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "EscapeRoom"

AEscapePaint::AEscapePaint()
{
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Mesh (from the base class) is the can: 10 cm wide, 12 cm tall, standing on the actor origin.
	Mesh->SetupAttachment(SceneRoot);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CanMesh(EscapeVisuals::Cylinder);
	if (CanMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CanMesh.Object);
	}
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, 6.f));
	Mesh->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.12f));

	PaintTop = EscapeVisuals::CreatePart(this, TEXT("PaintTop"), EscapeVisuals::Cylinder);
	PaintTop->SetupAttachment(SceneRoot);
	PaintTop->SetRelativeLocation(FVector(0.f, 0.f, 12.2f));
	PaintTop->SetRelativeScale3D(FVector(0.09f, 0.09f, 0.005f));

	ObjectId = TEXT("paint");
	PuzzleIds = { 2 };
	BaseColor = FLinearColor(0.55f, 0.55f, 0.58f);
	DisplayName = LOCTEXT("PaintName", "물감");
	ColorName = LOCTEXT("PaintColorName", "파랑");
}

void AEscapePaint::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	EscapeVisuals::SetColor(PaintTop, PaintColor);
}

void AEscapePaint::BeginPlay()
{
	Super::BeginPlay();
	EscapeVisuals::SetColor(PaintTop, PaintColor);
}

void AEscapePaint::Interact(AEscapeCharacter* Player, UPrimitiveComponent* HitComponent)
{
	if (AEscapeRoomManager* Manager = GetManager())
	{
		Manager->HandlePaint(this);
	}
}

#undef LOCTEXT_NAMESPACE
