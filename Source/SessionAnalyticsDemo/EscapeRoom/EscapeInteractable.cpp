#include "EscapeRoom/EscapeInteractable.h"

#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "EscapeRoom/EscapeRoomManager.h"
#include "EscapeRoom/EscapeVisuals.h"

AEscapeInteractable::AEscapeInteractable()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = EscapeVisuals::CreatePart(this, TEXT("Mesh"), EscapeVisuals::Cube);
	RootComponent = Mesh;
	DisplayName = NSLOCTEXT("Escape", "Inspect", "살펴보기");
}

void AEscapeInteractable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	EscapeVisuals::SetColor(Mesh, BaseColor);
}

void AEscapeInteractable::BeginPlay()
{
	Super::BeginPlay();
	EscapeVisuals::SetColor(Mesh, BaseColor);
}

void AEscapeInteractable::Interact(AEscapeCharacter* Player, UPrimitiveComponent* HitComponent)
{
	ReportInspected(ObjectId);
	if (!InspectMessage.IsEmpty())
	{
		ShowMessage(InspectMessage);
	}
}

void AEscapeInteractable::ReportInspected(const FString& Id) const
{
	if (AEscapeRoomManager* Manager = GetManager())
	{
		Manager->NotifyInspected(this, Id);
	}
}

void AEscapeInteractable::ShowMessage(const FText& Message) const
{
	if (AEscapeRoomManager* Manager = GetManager())
	{
		Manager->ShowMessage(Message);
	}
}

AEscapeRoomManager* AEscapeInteractable::GetManager() const
{
	if (!CachedManager.IsValid() && GetWorld())
	{
		for (TActorIterator<AEscapeRoomManager> It(GetWorld()); It; ++It)
		{
			CachedManager = *It;
			break;
		}
	}
	return CachedManager.Get();
}
