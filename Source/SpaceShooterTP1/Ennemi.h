#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ennemi.generated.h"

class ASpaceShipPawn;
class UStaticMeshComponent;

UCLASS()
class SPACESHOOTERTP1_API AEnnemi : public AActor{
	GENERATED_BODY()

public:

	AEnnemi();
	virtual void Tick(float DeltaTime) override;

	void ConfigurerEnnemi(ASpaceShipPawn* NouvelleCible, float NouvelleVitesse, int32 NouvelleVague);
	bool RecevoirDegats(int32 Degats);

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* MaillageEnnemi;

	// Corps arrondi principal de l'alien.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* CorpsAlien;

	// Oeil lumineux placé vers la caméra.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* OeilAlien;

	// Deux appendices donnent une silhouette immédiatement différente d'un cube.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* TentaculeGauche;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* TentaculeDroite;

	// Petite coque inférieure qui donne une apparence de drone alien.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* BaseAlien;

private:

	ASpaceShipPawn* Cible = nullptr;

	float VitesseDeplacement = 150.0f;
	float TempsAvantTir = 1.0f;
	float CadenceTirMin = 0.95f;
	float CadenceTirMax = 1.75f;
	float VitesseLaser = 900.0f;

	int32 PointsVie = 3;
	int32 Vague = 1;

	void TirerSurJoueur();
};
