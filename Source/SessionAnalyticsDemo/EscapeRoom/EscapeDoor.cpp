#include "EscapeRoom/EscapeDoor.h"

#include "Components/StaticMeshComponent.h"
#include "EscapeRoom/EscapeRoomManager.h"
#include "EscapeRoom/EscapeVisuals.h"

#define LOCTEXT_NAMESPACE "EscapeRoom"

namespace
{
	// Door panel: 100 wide, 210 tall, 6 thick; hinge on the local -Y edge, opens outward (away from the room).
	constexpr float DoorWidth = 100.f;
	constexpr float OpenYaw = 95.f;
	const FLinearColor DoorColor(0.42f, 0.3f, 0.2f);
	const FLinearColor KnobColor(0.75f, 0.65f, 0.35f);
}

AEscapeDoor::AEscapeDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	Hinge = CreateDefaultSubobject<USceneComponent>(TEXT("Hinge"));
	Hinge->SetupAttachment(SceneRoot);
	Hinge->SetRelativeLocation(FVector(0.f, -DoorWidth * 0.5f, 0.f));

	// Mesh (from the base class) is the door panel.
	Mesh->SetupAttachment(Hinge);
	Mesh->SetRelativeLocation(FVector(0.f, DoorWidth * 0.5f, 105.f));
	Mesh->SetRelativeScale3D(FVector(0.06f, DoorWidth / 100.f, 2.1f));

	Knob = EscapeVisuals::CreatePart(this, TEXT("Knob"), EscapeVisuals::Sphere);
	Knob->SetupAttachment(Hinge);
	Knob->SetRelativeLocation(FVector(5.f, DoorWidth * 0.88f, 100.f));
	Knob->SetRelativeScale3D(FVector(0.07f));

	Lamp = EscapeVisuals::CreatePart(this, TEXT("Lamp"), EscapeVisuals::Sphere);
	Lamp->SetupAttachment(SceneRoot);
	Lamp->SetRelativeLocation(FVector(8.f, 0.f, 245.f));
	Lamp->SetRelativeScale3D(FVector(0.16f));
	Lamp->ComponentTags.Add(TEXT("P2H1"));

	ObjectId = TEXT("door");
	BaseColor = DoorColor;
	DisplayName = LOCTEXT("DoorName", "문");
	InspectMessage = LOCTEXT("DoorLocked", "잠겨 있다. 열쇠가 필요하다.");
	LampName = LOCTEXT("LampName", "초록 램프");
	LampMessage = LOCTEXT("LampInspect", "문 위에서 초록 불빛이 은은하게 빛난다.");
}

void AEscapeDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyColors();
}

void AEscapeDoor::BeginPlay()
{
	Super::BeginPlay();
	ApplyColors();
}

void AEscapeDoor::ApplyColors()
{
	EscapeVisuals::SetColor(Knob, KnobColor);
	EscapeVisuals::SetColor(Lamp, LampColor, LampGlow);
}

FText AEscapeDoor::GetPrompt(const UPrimitiveComponent* HitComponent) const
{
	return HitComponent == Lamp ? LampName : DisplayName;
}

void AEscapeDoor::Interact(AEscapeCharacter* Player, UPrimitiveComponent* HitComponent)
{
	AEscapeRoomManager* Manager = GetManager();
	if (!Manager)
	{
		return;
	}
	if (HitComponent == Lamp)
	{
		Manager->NotifyInspected(this, TEXT("lamp"));
		Manager->ShowMessage(LampMessage);
		return;
	}
	Manager->HandleDoor(this);
}

void AEscapeDoor::Open()
{
	bOpen = true;
	SetActorTickEnabled(true);
}

void AEscapeDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	OpenAlpha = FMath::FInterpConstantTo(OpenAlpha, 1.f, DeltaSeconds, 0.8f);
	Hinge->SetRelativeRotation(FRotator(0.f, OpenYaw * FMath::InterpEaseInOut(0.f, 1.f, OpenAlpha, 2.f), 0.f));
	if (OpenAlpha >= 1.f)
	{
		SetActorTickEnabled(false);
	}
}

#undef LOCTEXT_NAMESPACE
