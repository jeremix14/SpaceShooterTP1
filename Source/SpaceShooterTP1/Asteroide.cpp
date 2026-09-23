#include "Asteroide.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "ExplosionVisuelle.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "SpaceShipPawn.h"
#include "UObject/ConstructorHelpers.h"
namespace {
	void AppliquerCouleurAsteroide(UStaticMeshComponent* Maillage, const FLinearColor& Couleur, UObject* Proprietaire) {
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")
		);
		if (!Base || !Maillage) {
			return;
		}
		UMaterialInstanceDynamic* Materiau = UMaterialInstanceDynamic::Create(Base, Proprietaire);
		if (Materiau) {
			Materiau->SetVectorParameterValue(TEXT("Color"), Couleur);
			Maillage->SetMaterial(0, Materiau);
		}
	}
}

// Construit le maillage et configure la détection de collision.
AAsteroide::AAsteroide() {
	PrimaryActorTick.bCanEverTick = true;
	MaillageAsteroide = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MaillageAsteroide"));
	RootComponent = MaillageAsteroide;
	MaillageAsteroide->SetMobility(EComponentMobility::Movable);
	MaillageAsteroide->SetGenerateOverlapEvents(true);
	MaillageAsteroide->SetCollisionObjectType(ECC_WorldDynamic);
	MaillageAsteroide->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MaillageAsteroide->SetCollisionResponseToAllChannels(ECR_Ignore);
	MaillageAsteroide->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MaillageAsteroide->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MaillageSphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (MaillageSphere.Succeeded()) {
		MaillageAsteroide->SetStaticMesh(MaillageSphere.Object);
	}
	MaillageAsteroide->OnComponentBeginOverlap.AddDynamic(this, &AAsteroide::GererDebutChevauchement);
}

// Programme la suppression d'un astéroïde resté trop longtemps hors jeu.
void AAsteroide::BeginPlay() {
	Super::BeginPlay();
	SetLifeSpan(28.0f);
}

// Déplace et fait tourner l'astéroïde pendant le combat.
void AAsteroide::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
	ASpaceShipPawn* Vaisseau = Cast<ASpaceShipPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (Vaisseau && Vaisseau->EstChoixBonusActif()) {
		return;
	}
	AddActorWorldOffset(VitesseLineaire * DeltaTime, true);
	AddActorLocalRotation(FRotator(0.0f, 0.0f, VitesseRotation * DeltaTime));
}

// Attribue à chaque astéroïde une taille, une forme, une rotation, une couleur et une résistance différentes.
void AAsteroide::ConfigurerAsteroide(const FVector& NouvelleVitesse) {
	// Conserve la vitesse initiale déterminée au moment de l'apparition.
	VitesseLineaire = NouvelleVitesse;
	// Sélectionne une famille de taille pour obtenir des astéroïdes minuscules jusqu'à colossaux.
	const float TirageTaille = FMath::FRand();
	float Taille = 1.0f;
	if (TirageTaille < 0.16f) {
		Taille = FMath::FRandRange(0.18f, 0.45f);
	}
	else if (TirageTaille < 0.42f) {
		Taille = FMath::FRandRange(0.50f, 0.95f);
	}
	else if (TirageTaille < 0.72f) {
		Taille = FMath::FRandRange(1.00f, 1.65f);
	}
	else if (TirageTaille < 0.92f) {
		Taille = FMath::FRandRange(1.70f, 2.70f);
	}
	else {
		Taille = FMath::FRandRange(2.80f, 4.20f);
	}
	// Déforme indépendamment les axes pour éviter une collection de sphères identiques.
	const FVector Echelle(
		Taille * FMath::FRandRange(0.72f, 1.28f),
		Taille * FMath::FRandRange(0.62f, 1.38f),
		Taille * FMath::FRandRange(0.58f, 1.42f)
	);
	// Applique la forme et une orientation de départ aléatoires.
	SetActorScale3D(Echelle);
	SetActorRotation(FRotator(
		0.0f,
		0.0f,
		FMath::FRandRange(0.0f, 360.0f)
	));
	// Chaque astéroïde tourne aussi à une vitesse propre.
	VitesseRotation = FMath::FRandRange(-42.0f, 42.0f);
	// Le nombre de tirs reste aléatoire comme demandé dans le TP, mais varie maintenant de 2 à 12.
	PointsResistance = FMath::RandRange(2, 12);
	// Une variation de teinte rend les astéroïdes moins uniformes sans modifier leur logique.
	const float Teinte = FMath::FRandRange(0.62f, 1.28f);
	AppliquerCouleurAsteroide(
		MaillageAsteroide,
		FLinearColor(0.34f * Teinte, 0.17f * Teinte, 0.045f * Teinte),
		this
	);
}

// Retire les dégâts reçus et détruit l'astéroïde à zéro résistance.
bool AAsteroide::RecevoirTir(int32 Degats) {
	PointsResistance -= FMath::Max(1, Degats);
	if (PointsResistance <= 0) {
		CreerExplosion();
		Destroy();
		return true;
	}
	return false;
}

// Crée l'effet d'explosion utilisé à la destruction.
void AAsteroide::CreerExplosion() {
	FActorSpawnParameters Parametres;
	GetWorld()->SpawnActor<AExplosionVisuelle>(
		AExplosionVisuelle::StaticClass(),
		GetActorLocation(),
		FRotator::ZeroRotator,
		Parametres
	);
}

// Détecte une collision avec le joueur, retire un coeur puis détruit l'astéroïde.
void AAsteroide::GererDebutChevauchement(
	UPrimitiveComponent* ComposantChevauche,
	AActor* AutreActeur,
	UPrimitiveComponent* AutreComposant,
	int32 IndexCorps,
	bool DepuisBalayage,
	const FHitResult& ResultatBalayage
) {
	ASpaceShipPawn* Vaisseau = Cast<ASpaceShipPawn>(AutreActeur);
	if (!Vaisseau || Vaisseau->EstChoixBonusActif()) {
		return;
	}
	Vaisseau->RecevoirImpactAsteroide();
	CreerExplosion();
	Destroy();
}
