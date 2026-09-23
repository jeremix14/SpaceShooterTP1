#include "SpaceShooterGameMode.h"
#include "SpaceShooterHUD.h"
// Configure le HUD C++ utilisé par le jeu.
ASpaceShooterGameMode::ASpaceShooterGameMode(){
	DefaultPawnClass = nullptr;
	HUDClass = ASpaceShooterHUD::StaticClass();
}
