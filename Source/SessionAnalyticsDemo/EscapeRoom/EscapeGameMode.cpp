#include "EscapeRoom/EscapeGameMode.h"

#include "EscapeRoom/EscapeCharacter.h"
#include "EscapeRoom/EscapeHUD.h"

AEscapeGameMode::AEscapeGameMode()
{
	DefaultPawnClass = AEscapeCharacter::StaticClass();
	HUDClass = AEscapeHUD::StaticClass();
}
