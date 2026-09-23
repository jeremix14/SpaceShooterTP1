#include "LaserProjectile.h"
#include "Asteroide.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Ennemi.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "SpaceShipPawn.h"
#include "UObject/ConstructorHelpers.h"

namespace{
	void AppliquerCouleurLaser(UStaticMeshComponent* Maillage, const FLinearColor& Couleur, UObject* Proprietaire){
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")
		);
		if(!Base || !Maillage){
			return;
		}
		UMaterialInstanceDynamic* Materiau = UMaterialInstanceDynamic::Create(Base, Proprietaire);
		if(Materiau){
			Materiau -> SetVectorParameterValue(TEXT("Color"), Couleur);
			Maillage -> SetMaterial(0, Materiau);
		}
	}
}

// Construit le laser, sa collision et son halo lumineux.
ALaserProjectile::ALaserProjectile(){
	PrimaryActorTick.bCanEverTick = true;
	CollisionLaser = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionLaser"));
	CollisionLaser -> InitSphereRadius(24.0f);
	CollisionLaser -> SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionLaser -> SetCollisionObjectType(ECC_WorldDynamic);
	CollisionLaser -> SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionLaser -> SetGenerateOverlapEvents(false);
	RootComponent = CollisionLaser;
	MaillageLaser = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MaillageLaser"));
	MaillageLaser -> SetupAttachment(RootComponent);
	MaillageLaser -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Le cube est long sur son axe X local.
	// Direction.Rotation() aligne précisément cet axe X avec la trajectoire.
	MaillageLaser -> SetRelativeScale3D(FVector(0.58f, 0.055f, 0.055f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MaillageCube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if(MaillageCube.Succeeded()){
		MaillageLaser -> SetStaticMesh(MaillageCube.Object);
	}
	LumiereLaser = CreateDefaultSubobject<UPointLightComponent>(TEXT("LumiereLaser"));
	LumiereLaser -> SetupAttachment(RootComponent);
	LumiereLaser -> SetIntensity(9000.0f);
	LumiereLaser -> SetAttenuationRadius(260.0f);
}

// Programme la disparition automatique d'un tir manqué.
void ALaserProjectile::BeginPlay(){
	Super::BeginPlay();
	SetLifeSpan(3.0f);
}

// Balaye le trajet du projectile pour ne pas traverser les cibles rapides.
void ALaserProjectile::Tick(float DeltaTime){
	Super::Tick(DeltaTime);
	ASpaceShipPawn* Vaisseau = Cast<ASpaceShipPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if(Vaisseau && Vaisseau -> EstChoixBonusActif()){
		return;
	}
	const FVector Debut = GetActorLocation();
	const FVector Fin = Debut + VitesseLineaire * DeltaTime;
	AActor* Cible = ChercherCible(Debut, Fin);
	if(Cible){
		AppliquerImpact(Cible);
		return;
	}
	SetActorLocation(Fin, false);
}

// Définit vitesse, orientation, camp, couleur et dégâts du projectile.
void ALaserProjectile::ConfigurerProjectile(const FVector& NouvelleVitesse, bool EstProjectileEnnemi, int32 NouveauxDegats){
	VitesseLineaire = NouvelleVitesse;
	bProjectileEnnemi = EstProjectileEnnemi;
	Degats = FMath::Max(1, NouveauxDegats);
	const FVector Direction = NouvelleVitesse.GetSafeNormal();
	if(!Direction.IsNearlyZero()){
		// La longueur du laser est maintenant l'axe X local, donc aucune correction de 90 degrés.
		SetActorRotation(Direction.Rotation());
	}
	if(bProjectileEnnemi){
		LumiereLaser -> SetLightColor(FLinearColor(1.0f, 0.03f, 0.01f));
		AppliquerCouleurLaser(MaillageLaser, FLinearColor(1.0f, 0.025f, 0.01f), this);
	}
	else{
		LumiereLaser -> SetLightColor(FLinearColor(0.02f, 0.80f, 1.0f));
		AppliquerCouleurLaser(MaillageLaser, FLinearColor(0.02f, 0.72f, 1.0f), this);
	}
}

AActor* ALaserProjectile::ChercherCible(const FVector& Debut, const FVector& Fin) const{
	FCollisionObjectQueryParams Objets;
	Objets.AddObjectTypesToQuery(ECC_WorldDynamic);
	Objets.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams Parametres;
	Parametres.AddIgnoredActor(this);
	if(GetOwner()){
		Parametres.AddIgnoredActor(GetOwner());
	}
	TArray<FHitResult> Impacts;
	GetWorld() -> SweepMultiByObjectType(
		Impacts,
		Debut,
		Fin,
		FQuat::Identity,
		Objets,
		FCollisionShape::MakeSphere(24.0f),
		Parametres
	);
	AActor* MeilleureCible = nullptr;
	float MeilleureDistance = TNumericLimits<float>::Max();
	for(const FHitResult& Impact : Impacts){
		AActor* Acteur = Impact.GetActor();
		if(!Acteur || Acteur == GetOwner()){
			continue;
		}
		bool bCibleValide = false;
		if(Cast<AAsteroide>(Acteur)){
			bCibleValide = true;
		}
		else if(bProjectileEnnemi && Cast<ASpaceShipPawn>(Acteur)){
			bCibleValide = true;
		}
		else if(!bProjectileEnnemi && Cast<AEnnemi>(Acteur)){
			bCibleValide = true;
		}
		if(bCibleValide && Impact.Distance < MeilleureDistance){
			MeilleureDistance = Impact.Distance;
			MeilleureCible = Acteur;
		}
	}
	return MeilleureCible;
}

// Transmet les dégâts à la cible touchée et attribue le score.
void ALaserProjectile::AppliquerImpact(AActor* Cible){
	if(!Cible){
		return;
	}
	if(AAsteroide* Asteroide = Cast<AAsteroide>(Cible)){
		const bool bDetruit = Asteroide -> RecevoirTir(Degats);
		if(bDetruit && !bProjectileEnnemi){
			if(ASpaceShipPawn* Vaisseau = Cast<ASpaceShipPawn>(GetOwner())){
				Vaisseau -> AjouterScore(100);
			}
		}
		Destroy();
		return;
	}
	if(!bProjectileEnnemi){
		if(AEnnemi* Ennemi = Cast<AEnnemi>(Cible)){
			const bool bDetruit = Ennemi -> RecevoirDegats(Degats);
			if(bDetruit){
				if(ASpaceShipPawn* Vaisseau = Cast<ASpaceShipPawn>(GetOwner())){
					Vaisseau -> AjouterScore(250);
				}
			}
			Destroy();
			return;
		}
	}
	if(bProjectileEnnemi){
		if(ASpaceShipPawn* Vaisseau = Cast<ASpaceShipPawn>(Cible)){
			Vaisseau -> RecevoirDegatsProjectile(1);
			Destroy();
		}
	}
}
