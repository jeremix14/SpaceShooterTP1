#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaserProjectile.generated.h"

class UPointLightComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class SPACESHOOTERTP1_API ALaserProjectile : public AActor{
	GENERATED_BODY()

public:

	ALaserProjectile();
	virtual void Tick(float DeltaTime) override;

	void ConfigurerProjectile(const FVector& NouvelleVitesse, bool EstProjectileEnnemi, int32 NouveauxDegats);

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	USphereComponent* CollisionLaser;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* MaillageLaser;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UPointLightComponent* LumiereLaser;

private:

	FVector VitesseLineaire = FVector::ZeroVector;
	bool bProjectileEnnemi = false;
	int32 Degats = 1;

	AActor* ChercherCible(const FVector& Debut, const FVector& Fin) const;
	void AppliquerImpact(AActor* Cible);
};
