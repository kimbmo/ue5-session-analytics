#include "EscapeRoom/EscapeCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EscapeRoom/EscapeInteractable.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "EscapeRoom"

AEscapeCharacter::AEscapeCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 90.f);
	bUseControllerRotationYaw = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
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
	FHitResult Hit;
	if (!TraceView(Hit))
	{
		return false;
	}
	AEscapeInteractable* Target = Cast<AEscapeInteractable>(Hit.GetActor());
	if (!Target)
	{
		return false;
	}
	Target->Interact(this, Hit.GetComponent());
	return true;
}

void AEscapeCharacter::UpdateFocus()
{
	FHitResult Hit;
	const AEscapeInteractable* Target = TraceView(Hit) ? Cast<AEscapeInteractable>(Hit.GetActor()) : nullptr;
	FocusPrompt = Target ? FText::Format(LOCTEXT("Prompt", "[E] {0}"), Target->GetPrompt(Hit.GetComponent())) : FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
