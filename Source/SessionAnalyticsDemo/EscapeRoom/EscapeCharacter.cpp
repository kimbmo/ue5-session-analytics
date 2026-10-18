#include "EscapeRoom/EscapeCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "EscapeRoom/EscapeInteractable.h"
#include "EscapeRoom/EscapeVisuals.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetSystemLibrary.h"

#define LOCTEXT_NAMESPACE "EscapeRoom"

AEscapeCharacter::AEscapeCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 90.f);
	bUseControllerRotationYaw = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	// Eye at 152 cm: v1 testers found desk, paints and safe too far below the view at 160 cm.
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 62.f));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(85.f);
}

void AEscapeCharacter::BeginPlay()
{
	Super::BeginPlay();
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->SetShowMouseCursor(false);
	}
}

void AEscapeCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	ReadInput();
	UpdateFocus();
}

void AEscapeCharacter::ReadInput()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	const float Forward = (PC->IsInputKeyDown(EKeys::W) ? 1.f : 0.f) - (PC->IsInputKeyDown(EKeys::S) ? 1.f : 0.f);
	const float Right = (PC->IsInputKeyDown(EKeys::D) ? 1.f : 0.f) - (PC->IsInputKeyDown(EKeys::A) ? 1.f : 0.f);
	if (Forward != 0.f)
	{
		AddMovementInput(GetActorForwardVector(), Forward);
	}
	if (Right != 0.f)
	{
		AddMovementInput(GetActorRightVector(), Right);
	}

	float MouseX = 0.f;
	float MouseY = 0.f;
	PC->GetInputMouseDelta(MouseX, MouseY);
	AddControllerYawInput(MouseX * MouseSensitivity);
	AddControllerPitchInput(-MouseY * MouseSensitivity);

	if (PC->WasInputKeyJustPressed(EKeys::E))
	{
		Interact();
	}

	// Playtest builds have no menu: Esc quits. The analytics session is closed on shutdown (reason recorded by the plugin).
	if (PC->WasInputKeyJustPressed(EKeys::Escape))
	{
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
	}
}

bool AEscapeCharacter::TraceView(FHitResult& OutHit) const
{
	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + GetControlRotation().Vector() * InteractDistance;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EscapeInteract), false, this);
	return GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, Params);
}

bool AEscapeCharacter::Interact()
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastInteractTime < InteractCooldownSeconds)
	{
		return false;
	}
	LastInteractTime = Now;

	FHitResult Hit;
	AEscapeInteractable* Target = TraceView(Hit) ? Cast<AEscapeInteractable>(Hit.GetActor()) : nullptr;
	bLastInteractHit = Target != nullptr;
	if (!Target)
	{
		return false;
	}
	Target->Interact(this, Hit.GetComponent());
	return true;
}

float AEscapeCharacter::GetSecondsSinceInteract() const
{
	return GetWorld() ? static_cast<float>(GetWorld()->GetTimeSeconds() - LastInteractTime) : 1000.f;
}

void AEscapeCharacter::SetFocus(UPrimitiveComponent* Component)
{
	if (FocusComponent.Get() == Component)
	{
		return;
	}
	// The room manager rewrites the glow of active hint targets every tick, so clearing here never hides a hint.
	if (UMeshComponent* Previous = Cast<UMeshComponent>(FocusComponent.Get()))
	{
		EscapeVisuals::SetHintGlow(Previous, 0.f);
	}
	FocusComponent = Component;
	if (UMeshComponent* Current = Cast<UMeshComponent>(Component))
	{
		EscapeVisuals::SetHintGlow(Current, FocusHighlight);
	}
}

void AEscapeCharacter::UpdateFocus()
{
	FHitResult Hit;
	const AEscapeInteractable* Target = TraceView(Hit) ? Cast<AEscapeInteractable>(Hit.GetActor()) : nullptr;
	FocusPrompt = Target ? FText::Format(LOCTEXT("Prompt", "[E] {0}"), Target->GetPrompt(Hit.GetComponent())) : FText::GetEmpty();
	SetFocus(Target ? Hit.GetComponent() : nullptr);
}

#undef LOCTEXT_NAMESPACE
