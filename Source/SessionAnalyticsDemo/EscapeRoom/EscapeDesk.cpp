#include "EscapeRoom/EscapeDesk.h"

#include "Components/StaticMeshComponent.h"
#include "EscapeRoom/EscapeRoomManager.h"
#include "EscapeRoom/EscapeVisuals.h"

#define LOCTEXT_NAMESPACE "EscapeRoom"

namespace
{
	// Desk: 60 deep (X), 140 wide (Y), top at 75 cm. The drawer front is flush with the +X edge.
	const FVector DrawerClosed(4.f, 30.f, 63.f);
	const FVector HandleClosed(31.5f, 30.f, 63.f);
	const FVector PaletteClosed(4.f, 30.f, 69.6f);
	const FLinearColor DeskColor(0.32f, 0.2f, 0.11f);
	const FLinearColor DrawerColor(0.38f, 0.25f, 0.14f);
	const FLinearColor MetalColor(0.6f, 0.6f, 0.62f);
	const FLinearColor PaletteColor(0.8f, 0.62f, 0.38f);
}

AEscapeDesk::AEscapeDesk()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Mesh (from the base class) is the desk top.
	Mesh->SetupAttachment(SceneRoot);
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, 73.f));
	Mesh->SetRelativeScale3D(FVector(0.6f, 1.4f, 0.04f));

	for (int32 Index = 0; Index < 4; ++Index)
	{
		UStaticMeshComponent* Leg = EscapeVisuals::CreatePart(this, *FString::Printf(TEXT("Leg%d"), Index), EscapeVisuals::Cube);
		Leg->SetupAttachment(SceneRoot);
		Leg->SetRelativeLocation(FVector((Index % 2 ? 26.f : -26.f), (Index / 2 ? 66.f : -66.f), 35.5f));
		Leg->SetRelativeScale3D(FVector(0.05f, 0.05f, 0.71f));
		Legs.Add(Leg);
	}

	Drawer = EscapeVisuals::CreatePart(this, TEXT("Drawer"), EscapeVisuals::Cube);
	Drawer->SetupAttachment(SceneRoot);
	Drawer->SetRelativeLocation(DrawerClosed);
	Drawer->SetRelativeScale3D(FVector(0.52f, 0.6f, 0.12f));

	DrawerHandle = EscapeVisuals::CreatePart(this, TEXT("DrawerHandle"), EscapeVisuals::Cube);
	DrawerHandle->SetupAttachment(SceneRoot);
	DrawerHandle->SetRelativeLocation(HandleClosed);
	DrawerHandle->SetRelativeScale3D(FVector(0.03f, 0.14f, 0.025f));

	Palette = EscapeVisuals::CreatePart(this, TEXT("Palette"), EscapeVisuals::Cube);
	Palette->SetupAttachment(SceneRoot);
	Palette->SetRelativeLocation(PaletteClosed);
	Palette->SetRelativeScale3D(FVector(0.25f, 0.32f, 0.01f));
	Palette->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ObjectId = TEXT("desk");
	BaseColor = DeskColor;
	DisplayName = LOCTEXT("DeskName", "책상");
	DrawerName = LOCTEXT("DrawerName", "서랍");
	InspectMessage = LOCTEXT("DeskInspect", "오래된 작업 책상이다. 서랍이 하나 있다.");
}

void AEscapeDesk::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyColors();
}

void AEscapeDesk::BeginPlay()
{
	Super::BeginPlay();
	ApplyColors();
}

void AEscapeDesk::ApplyColors()
{
	for (UStaticMeshComponent* Leg : Legs)
	{
		EscapeVisuals::SetColor(Leg, BaseColor);
	}
	EscapeVisuals::SetColor(Drawer, DrawerColor);
	EscapeVisuals::SetColor(DrawerHandle, MetalColor);
	EscapeVisuals::SetColor(Palette, PaletteColor);
}

bool AEscapeDesk::IsDrawerPart(const UPrimitiveComponent* Component) const
{
	return Component && (Component == Drawer || Component == DrawerHandle);
}

FText AEscapeDesk::GetPrompt(const UPrimitiveComponent* HitComponent) const
{
	return IsDrawerPart(HitComponent) ? DrawerName : DisplayName;
}

void AEscapeDesk::Interact(AEscapeCharacter* Player, UPrimitiveComponent* HitComponent)
{
	if (!IsDrawerPart(HitComponent))
	{
		Super::Interact(Player, HitComponent);
		return;
	}
	if (AEscapeRoomManager* Manager = GetManager())
	{
		Manager->HandleDrawer(this);
	}
}

void AEscapeDesk::OpenDrawer()
{
	bDrawerOpen = true;
	SetActorTickEnabled(true);
}

void AEscapeDesk::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	DrawerAlpha = FMath::FInterpConstantTo(DrawerAlpha, bDrawerOpen ? 1.f : 0.f, DeltaSeconds, 2.5f);
	const FVector Offset(DrawerSlideDistance * FMath::InterpEaseOut(0.f, 1.f, DrawerAlpha, 2.f), 0.f, 0.f);
	Drawer->SetRelativeLocation(DrawerClosed + Offset);
	DrawerHandle->SetRelativeLocation(HandleClosed + Offset);
	Palette->SetRelativeLocation(PaletteClosed + Offset);

	if (FMath::IsNearlyEqual(DrawerAlpha, bDrawerOpen ? 1.f : 0.f))
	{
		SetActorTickEnabled(false);
	}
}

#undef LOCTEXT_NAMESPACE
