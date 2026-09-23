#include "Ennemi.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "ExplosionVisuelle.h"
#include "Kismet/GameplayStatics.h"
#include "LaserProjectile.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "SpaceShipPawn.h"
#include "UObject/ConstructorHelpers.h"
namespace{
	// Crée un matériau dynamique coloré pour une pièce visuelle de l'alien.
	void AppliquerCouleurEnnemi(UStaticMeshComponent* Maillage, const FLinearColor& Couleur, UObject* Proprietaire){
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

	void JouerSonEnnemi(UObject* Contexte){
		USoundBase* Son = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_EnemyLaser.S_EnemyLaser"));
		if(Son){
			UGameplayStatics::PlaySound2D(Contexte, Son, 0.22f);
		}
	}

}

// Construit un alien stylisé à partir de plusieurs formes Unreal tout en conservant une collision simple.
AEnnemi::AEnnemi(){
	// Active la poursuite et le tir à chaque image.
	PrimaryActorTick.bCanEverTick = true;
	// Le composant racine reste responsable de la collision mais devient invisible.
	MaillageEnnemi = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MaillageEnnemi"));
	RootComponent = MaillageEnnemi;
	MaillageEnnemi -> SetMobility(EComponentMobility::Movable);
	MaillageEnnemi -> SetGenerateOverlapEvents(true);
	MaillageEnnemi -> SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MaillageEnnemi -> SetCollisionObjectType(ECC_WorldDynamic);
	MaillageEnnemi -> SetCollisionResponseToAllChannels(ECR_Ignore);
	MaillageEnnemi -> SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	MaillageEnnemi -> SetVisibility(false, false);
	// Charge les formes simples utilisées pour construire le visuel de l'alien.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MaillageCube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MaillageSphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MaillageCylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	// Crée un gros corps arrondi violet.
	CorpsAlien = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CorpsAlien"));
	CorpsAlien -> SetupAttachment(RootComponent);
	CorpsAlien -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CorpsAlien -> SetRelativeScale3D(FVector(0.24f, 0.48f, 0.34f));
	// Place un oeil vers la caméra pour rendre l'ennemi immédiatement reconnaissable.
	OeilAlien = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OeilAlien"));
	OeilAlien -> SetupAttachment(RootComponent);
	OeilAlien -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OeilAlien -> SetRelativeLocation(FVector(-30.0f, 0.0f, 8.0f));
	OeilAlien -> SetRelativeScale3D(FVector(0.10f, 0.13f, 0.13f));
	// Ajoute deux tentacules inclinés sous le corps.
	TentaculeGauche = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TentaculeGauche"));
	TentaculeGauche -> SetupAttachment(RootComponent);
	TentaculeGauche -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TentaculeGauche -> SetRelativeLocation(FVector(4.0f, -30.0f, -38.0f));
	TentaculeGauche -> SetRelativeRotation(FRotator(0.0f, 0.0f, -18.0f));
	TentaculeGauche -> SetRelativeScale3D(FVector(0.08f, 0.10f, 0.36f));
	TentaculeDroite = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TentaculeDroite"));
	TentaculeDroite -> SetupAttachment(RootComponent);
	TentaculeDroite -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TentaculeDroite -> SetRelativeLocation(FVector(4.0f, 30.0f, -38.0f));
	TentaculeDroite -> SetRelativeRotation(FRotator(0.0f, 0.0f, 18.0f));
	TentaculeDroite -> SetRelativeScale3D(FVector(0.08f, 0.10f, 0.36f));
	// Ajoute une base métallique qui donne l'impression d'un petit drone extraterrestre.
	BaseAlien = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseAlien"));
	BaseAlien -> SetupAttachment(RootComponent);
	BaseAlien -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BaseAlien -> SetRelativeLocation(FVector(8.0f, 0.0f, -16.0f));
	BaseAlien -> SetRelativeScale3D(FVector(0.12f, 0.42f, 0.10f));
	// Le cube invisible reste présent uniquement pour fournir une collision fiable aux lasers.
	if(MaillageCube.Succeeded()){
		MaillageEnnemi -> SetStaticMesh(MaillageCube.Object);
	}
	// Assigne ensuite les formes visibles aux différentes parties de l'alien.
	if(MaillageSphere.Succeeded()){
		CorpsAlien -> SetStaticMesh(MaillageSphere.Object);
		OeilAlien -> SetStaticMesh(MaillageSphere.Object);
	}
	if(MaillageCube.Succeeded()){
		TentaculeGauche -> SetStaticMesh(MaillageCube.Object);
		TentaculeDroite -> SetStaticMesh(MaillageCube.Object);
	}
	if(MaillageCylinder.Succeeded()){
		BaseAlien -> SetStaticMesh(MaillageCylinder.Object);
	}
}

// Initialise la couleur, la cadence et la durée de vie.
void AEnnemi::BeginPlay(){
	Super::BeginPlay();
	// Utilise des couleurs contrastées afin de distinguer immédiatement les aliens du joueur et des astéroïdes.
	AppliquerCouleurEnnemi(CorpsAlien, FLinearColor(0.44f, 0.04f, 0.72f), this);
	AppliquerCouleurEnnemi(OeilAlien, FLinearColor(1.0f, 0.06f, 0.15f), this);
	AppliquerCouleurEnnemi(TentaculeGauche, FLinearColor(0.22f, 0.02f, 0.38f), this);
	AppliquerCouleurEnnemi(TentaculeDroite, FLinearColor(0.22f, 0.02f, 0.38f), this);
	AppliquerCouleurEnnemi(BaseAlien, FLinearColor(0.14f, 0.05f, 0.24f), this);
	TempsAvantTir = FMath::FRandRange(CadenceTirMin, CadenceTirMax);
	SetLifeSpan(40.0f);
}

// Poursuit le joueur et tire selon la difficulté de la vague.
void AEnnemi::Tick(float DeltaTime){
	Super::Tick(DeltaTime);
	if(!Cible || !IsValid(Cible) || !Cible -> EstPartieCommencee() || Cible -> EstPartieTerminee() || Cible -> EstChoixBonusActif()){
		return;
	}
	FVector Direction = Cible -> GetActorLocation() - GetActorLocation();
	Direction.X = 0.0f;
	Direction.Normalize();
	AddActorWorldOffset(Direction * VitesseDeplacement * DeltaTime, false);
	TempsAvantTir -= DeltaTime;
	if(TempsAvantTir <= 0.0f){
		TirerSurJoueur();
		const float FacteurCadence = FMath::Pow(0.94f, static_cast<float>(Vague - 1));
		TempsAvantTir = FMath::Max(
			0.38f,
			FMath::FRandRange(CadenceTirMin, CadenceTirMax) * FacteurCadence
		);
	}
}

// Adapte vitesse, vie et laser à la vague actuelle.
void AEnnemi::ConfigurerEnnemi(ASpaceShipPawn* NouvelleCible, float NouvelleVitesse, int32 NouvelleVague){
	Cible = NouvelleCible;
	VitesseDeplacement = NouvelleVitesse;
	Vague = FMath::Max(1, NouvelleVague);
	// Les vagues montent progressivement la résistance.
	const int32 VieVague = (Vague - 1) / 2;
	PointsVie = FMath::RandRange(2, 3) + VieVague;
	VitesseLaser = 900.0f * (1.0f + (Vague - 1) * 0.045f);
}

// Retire la vie et détruit l'ennemi lorsqu'elle atteint zéro.
bool AEnnemi::RecevoirDegats(int32 Degats){
	PointsVie -= FMath::Max(1, Degats);
	if(PointsVie <= 0){
		FActorSpawnParameters Parametres;
		GetWorld() -> SpawnActor<AExplosionVisuelle>(
			AExplosionVisuelle::StaticClass(),
			GetActorLocation(),
			FRotator::ZeroRotator,
			Parametres
		);
		Destroy();
		return true;
	}
	return false;
}

// Crée un laser ennemi dirigé vers le joueur.
void AEnnemi::TirerSurJoueur(){
	if(!Cible || !IsValid(Cible)){
		return;
	}
	FVector Direction = Cible -> GetActorLocation() - GetActorLocation();
	Direction.X = 0.0f;
	Direction.Normalize();
	const FVector PositionProjectile = GetActorLocation() + Direction * 55.0f;
	FActorSpawnParameters Parametres;
	Parametres.Owner = this;
	ALaserProjectile* Laser = GetWorld() -> SpawnActor<ALaserProjectile>(
		ALaserProjectile::StaticClass(),
		PositionProjectile,
		FRotator::ZeroRotator,
		Parametres
	);
	if(Laser){
		Laser -> ConfigurerProjectile(Direction * VitesseLaser, true, 1);
		JouerSonEnnemi(this);
	}
}
