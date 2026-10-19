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
	const FLinearColor ThumbHoleColor(0.03f, 0.02f, 0.015f);

	// Painter's palette from basic shapes: a flat oval board (30 x 22 cm, 1 cm thick), a thumb hole near the front-right edge and five
	// paint blobs along the back edge. Positions are relative to PaletteRoot (X = towards the drawer front).
	constexpr float BoardThickness = 1.f;
	const FVector ThumbHoleOffset(4.5f, 8.5f, BoardThickness * 0.5f + 0.05f);
	struct FBlob { FVector Offset; FLinearColor Color; };
	const FBlob Blobs[] = {
		{ FVector(-1.f, -12.f, 0.75f), FLinearColor(0.75f, 0.06f, 0.05f) },
		{ FVector(-6.5f, -7.f, 0.75f), FLinearColor(0.04f, 0.12f, 0.75f) },
		{ FVector(-8.f, 0.f, 0.75f), FLinearColor(0.9f, 0.9f, 0.88f) },
		{ FVector(-6.5f, 7.f, 0.75f), FLinearColor(0.95f, 0.75f, 0.05f) },
		{ FVector(-1.f, 12.f, 0.75f), FLinearColor(0.03f, 0.03f, 0.03f) },
	};
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

	PaletteRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PaletteRoot"));
	PaletteRoot->SetupAttachment(SceneRoot);
	PaletteRoot->SetRelativeLocation(PaletteClosed);

	// Oval board: a flattened cylinder, wider (Y) than deep (X).
	Palette = EscapeVisuals::CreatePart(this, TEXT("PaletteBoard"), EscapeVisuals::Cylinder);
	Palette->SetupAttachment(PaletteRoot);
	Palette->SetRelativeScale3D(FVector(0.22f, 0.30f, BoardThickness / 100.f));
	Palette->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PaletteThumbHole = EscapeVisuals::CreatePart(this, TEXT("PaletteThumbHole"), EscapeVisuals::Cylinder);
	PaletteThumbHole->SetupAttachment(PaletteRoot);
	PaletteThumbHole->SetRelativeLocation(ThumbHoleOffset);
	PaletteThumbHole->SetRelativeScale3D(FVector(0.04f, 0.045f, 0.002f));
	PaletteThumbHole->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Blobs); ++Index)
	{
		UStaticMeshComponent* Blob = EscapeVisuals::CreatePart(this, *FString::Printf(TEXT("PaletteBlob%d"), Index), EscapeVisuals::Sphere);
		Blob->SetupAttachment(PaletteRoot);
		Blob->SetRelativeLocation(Blobs[Index].Offset);
		Blob->SetRelativeScale3D(FVector(0.045f, 0.05f, 0.012f));
		Blob->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		PaletteBlobs.Add(Blob);
	}

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
	EscapeVisuals::SetColor(PaletteThumbHole, ThumbHoleColor);
	for (int32 Index = 0; Index < PaletteBlobs.Num() && Index < UE_ARRAY_COUNT(Blobs); ++Index)
	{
		EscapeVisuals::SetColor(PaletteBlobs[Index], Blobs[Index].Color);
	}
}

bool AEscapeDesk::IsDrawerPart(const UPrimitiveComponent* Component) const
{
	return Component && (Component == Drawer || Component == DrawerHandle);
}

bool AEscapeDesk::IsPalettePart(const UPrimitiveComponent* Component) const
{
	return Component && (Component == Palette || Component == PaletteThumbHole
		|| PaletteBlobs.ContainsByPredicate([Component](const UStaticMeshComponent* Blob) { return Blob == Component; }));
}

FText AEscapeDesk::GetPrompt(const UPrimitiveComponent* HitComponent) const
{
	if (bPaletteInDrawer && IsPalettePart(HitComponent))
	{
		return LOCTEXT("TakePalettePrompt", "팔레트 집기");
	}
	return IsDrawerPart(HitComponent) ? DrawerName : DisplayName;
}

void AEscapeDesk::Interact(AEscapeCharacter* Player, UPrimitiveComponent* HitComponent)
{
	if (bDrawerOpen && bPaletteInDrawer && IsPalettePart(HitComponent))
	{
		if (AEscapeRoomManager* Manager = GetManager())
		{
			Manager->HandlePalette(this);
		}
		return;
	}
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
	// v5: the palette can be picked up (E) once the drawer is open.
	for (UStaticMeshComponent* Part : GetPaletteParts())
	{
		Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Part->SetCollisionResponseToAllChannels(ECR_Block);
	}
	SetActorTickEnabled(true);
}

void AEscapeDesk::TakePalette()
{
	bPaletteInDrawer = false;
	for (UStaticMeshComponent* Part : GetPaletteParts())
	{
		Part->SetVisibility(false);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

TArray<UStaticMeshComponent*> AEscapeDesk::GetPaletteParts() const
{
	TArray<UStaticMeshComponent*> Parts = { Palette.Get(), PaletteThumbHole.Get() };
	for (UStaticMeshComponent* Blob : PaletteBlobs)
	{
		Parts.Add(Blob);
	}
	return Parts;
}

void AEscapeDesk::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	DrawerAlpha = FMath::FInterpConstantTo(DrawerAlpha, bDrawerOpen ? 1.f : 0.f, DeltaSeconds, 2.5f);
	const FVector Offset(DrawerSlideDistance * FMath::InterpEaseOut(0.f, 1.f, DrawerAlpha, 2.f), 0.f, 0.f);
	Drawer->SetRelativeLocation(DrawerClosed + Offset);
	DrawerHandle->SetRelativeLocation(HandleClosed + Offset);
	PaletteRoot->SetRelativeLocation(PaletteClosed + Offset);

	if (FMath::IsNearlyEqual(DrawerAlpha, bDrawerOpen ? 1.f : 0.f))
	{
		SetActorTickEnabled(false);
	}
}

#undef LOCTEXT_NAMESPACE
