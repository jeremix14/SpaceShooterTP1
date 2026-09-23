#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Asteroide.generated.h"

class UStaticMeshComponent;

UCLASS()
class SPACESHOOTERTP1_API AAsteroide : public AActor{
	GENERATED_BODY()

public:

	AAsteroide();
	virtual void Tick(float DeltaTime) override;

	void ConfigurerAsteroide(const FVector& NouvelleVitesse);
	bool RecevoirTir(int32 Degats);

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* MaillageAsteroide;

	UFUNCTION()
	void GererDebutChevauchement(
		UPrimitiveComponent* ComposantChevauche,
		AActor* AutreActeur,
		UPrimitiveComponent* AutreComposant,
		int32 IndexCorps,
		bool DepuisBalayage,
		const FHitResult& ResultatBalayage
	);

private:

	// Plusieurs sphères superposées construisent une silhouette rocheuse irrégulière.
	UPROPERTY()
	TArray<UStaticMeshComponent*> MorceauxVisuels;

	// Petites formes sombres placées sur la face caméra pour simuler des cratères.
	UPROPERTY()
	TArray<UStaticMeshComponent*> CrateresVisuels;

	FVector VitesseLineaire = FVector::ZeroVector;
	float VitesseRotation = 0.0f;
	int32 PointsResistance = 1;

	void CreerExplosion();
};
