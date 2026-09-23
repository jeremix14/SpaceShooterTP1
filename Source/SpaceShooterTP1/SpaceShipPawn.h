#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SpaceShipPawn.generated.h"

class UCameraComponent;
class UChildActorComponent;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class SPACESHOOTERTP1_API ASpaceShipPawn : public APawn{
	GENERATED_BODY()

public:

	ASpaceShipPawn();
	virtual void Tick(float DeltaTime) override;

	void CommencerPartie();
	void RetournerMenu();

	// Astéroïde : exactement un coeur de dégât.
	void RecevoirImpactAsteroide();

	// Projectile ennemi : dégâts exprimés en quarts de coeur.
	void RecevoirDegatsProjectile(int32 QuartiersDegats);

	// Ajoute des points et déclenche éventuellement un choix de bonus.
	void AjouterScore(int32 Points);

	// Informations utilisées par les autres acteurs et le HUD.
	int32 ObtenirQuartiersVie() const;
	int32 ObtenirQuartiersVieMax() const;
	int32 ObtenirScore() const;
	int32 ObtenirVague() const;
	int32 ObtenirChargesDash() const;
	int32 ObtenirChargesDashMax() const;
	float ObtenirRechargeDashRestante() const;
	float ObtenirTempsRechargeDash() const;
	bool EstPartieCommencee() const;
	bool EstPartieTerminee() const;
	bool EstChoixBonusActif() const;
	bool EstMenuCommandesActif() const;
	FString ObtenirNomsMembres() const;
	FString ObtenirNomBonus(int32 Index) const;
	FString ObtenirDescriptionBonus(int32 Index) const;

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* MaillageVaisseau;

	// Blueprint complet Star Sparrow utilisé uniquement comme apparence du joueur.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UChildActorComponent* VisuelStarSparrow;

	// Corps central visible du nouveau vaisseau.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* CoqueVaisseau;

	// Nez du vaisseau qui rend immédiatement sa direction lisible.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* NezVaisseau;

	// Ailes latérales du vaisseau.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* AileGauche;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* AileDroite;

	// Cockpit placé devant la coque.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* CockpitVaisseau;

	// Deux moteurs visibles à l'arrière.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* MoteurGauche;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* MoteurDroit;

	// Pivot sans géométrie : il tourne vers la souris sans réduire la taille du canon.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	USceneComponent* PivotTourelle;

	// Base visible de la tourelle.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* Tourelle;

	// Canon long attaché au pivot.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* CanonTourelle;

	// Embout lumineux placé au bout du canon afin de rendre la direction de tir évidente.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* BoucheCanon;

	// Deux flammes de moteur dont la longueur varie avec la propulsion.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* FlammeMoteurGauche;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UStaticMeshComponent* FlammeMoteurDroit;

	// Lumières dynamiques utilisées par les moteurs et le flash du canon.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UPointLightComponent* LumiereMoteurGauche;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UPointLightComponent* LumiereMoteurDroit;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UPointLightComponent* LumiereCanon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Composants")
	UCameraComponent* CameraJeu;

	// Déplacement.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deplacement")
	float VitesseMaxAvant = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deplacement")
	float VitesseMaxArriere = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deplacement")
	float AccelerationAvant = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deplacement")
	float AccelerationArriere = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deplacement")
	float FreinageBase = 1100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deplacement")
	float ResistanceNaturelle = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deplacement")
	float RotationMaxBasseVitesse = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deplacement")
	float RotationMaxHauteVitesse = 125.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deplacement")
	float ReponseRotation = 9.0f;

	// Caméra.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float LargeurZoneJeu = 1800.0f;

	// Vie : quatre unités = un coeur.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vie")
	int32 NombreCoeurs = 5;

	// Tir.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tir")
	float CadenceTir = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tir")
	float VitesseLaser = 1550.0f;

	// Dash.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
	float DistanceDash = 330.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dash")
	float TempsRechargeDash = 2.4f;

	// Progression.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	int32 PointsParBonus = 750;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	float DureeVague = 28.0f;

	// Astéroïdes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asteroides")
	float IntervalleAsteroideMin = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asteroides")
	float IntervalleAsteroideMax = 3.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asteroides")
	float VitesseAsteroideMin = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Asteroides")
	float VitesseAsteroideMax = 340.0f;

	// Ennemis.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ennemis")
	float IntervalleEnnemiMin = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ennemis")
	float IntervalleEnnemiMax = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ennemis")
	float VitesseEnnemiMin = 105.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ennemis")
	float VitesseEnnemiMax = 175.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	FString NomsMembres = TEXT("Jeremie Bouchard");

private:

	enum class EBonusType : uint8{
		TirMultiple,
		Degats,
		Cadence,
		ChargeDash,
		RechargeDash,
		Vie,
		Vitesse,
		Rotation,
		Acceleration
	};

	float VitesseActuelle = 0.0f;
	float VitesseRotationActuelle = 0.0f;
	// Valeur lissée utilisée pour animer les flammes de propulsion.
	float IntensitePropulseurs = 0.0f;
	// Petit chronomètre utilisé pour le flash lumineux de la bouche du canon.
	float TempsFlashCanon = 0.0f;

	float TempsRechargeTir = 0.0f;
	float RechargeDashRestante = 0.0f;
	float TempsAvantProchainAsteroide = 0.0f;
	float TempsAvantProchainEnnemi = 0.0f;
	float TempsVagueRestant = 0.0f;
	float TempsAffichageVague = 0.0f;

	int32 QuartiersVie = 20;
	int32 Score = 0;
	int32 ProchainSeuilBonus = 750;
	// Nombre de bonus déjà gagnés qui attendent une fin de vague.
	int32 BonusEnAttente = 0;
	int32 VagueActuelle = 1;

	int32 NombreProjectilesParTir = 1;
	int32 DegatsLaserJoueur = 1;
	int32 ChargesDash = 1;
	int32 ChargesDashMax = 1;

	bool bPartieCommencee = false;
	bool bPartieTerminee = false;
	bool bChoixBonusActif = false;
	// Empêche la vague suivante de commencer avant le choix du bonus.
	bool bPassageVagueEnAttente = false;
	bool bMenuCommandesActif = false;

	TArray<EBonusType> BonusProposes;

	FVector PositionInitialeVaisseau = FVector::ZeroVector;
	FRotator RotationInitialeVaisseau = FRotator::ZeroRotator;

	bool ObtenirDirectionCurseur(FVector& Direction) const;
	// Ajuste automatiquement le Star Sparrow aux dimensions de l'ancienne collision du vaisseau.
	void AjusterVisuelStarSparrow();
	// Affiche ou cache explicitement le Blueprint enfant lorsque le menu ou la partie change d'état.
	void DefinirVisibiliteStarSparrow(bool bVisible);
	void MettreAJourTourelle();
	// Anime les flammes et les lumières selon les commandes et la vitesse actuelle.
	void MettreAJourEffetsVaisseau(bool Avancer, bool Reculer, float DeltaTime);
	void TirerLaser();
	void FaireDash();

	void GererInterfaceSouris();
	void PreparerChoixBonus();
	void ChoisirBonus(int32 Index);
	void AppliquerBonus(EBonusType Bonus);
	FString NomBonus(EBonusType Bonus) const;
	FString DescriptionBonus(EBonusType Bonus) const;

	void GenererAsteroide();
	void GenererEnnemi();
	void MettreAJourVague(float DeltaTime);

	void LimiterPositionDansEcran();
	void TerminerPartie();
	void NettoyerActeursJeu();

	bool ObtenirLimitesMondeEcran(float& MinimumY, float& MaximumY, float& MinimumZ, float& MaximumZ) const;
};
