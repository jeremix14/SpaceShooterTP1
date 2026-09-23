#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExplosionVisuelle.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class SPACESHOOTERTP1_API AExplosionVisuelle : public AActor{
	GENERATED_BODY()

public:

	AExplosionVisuelle();
	virtual void Tick(float DeltaTime) override;

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY()
	UStaticMeshComponent* MaillageExplosion;

	UPROPERTY()
	UPointLightComponent* LumiereExplosion;

	float TempsEcoule = 0.0f;
	float Duree = 0.32f;
};
