#include "SpaceShipPawn.h"
#include "Asteroide.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/ChildActorComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Ennemi.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LaserProjectile.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
namespace{
	FVector ObtenirPointContour(UStaticMeshComponent* Maillage, const FVector& Direction){
		const FVector Centre = Maillage -> GetComponentLocation();
		const FVector Cible = Centre + Direction * 100000.0f;
		FVector PointContour = Centre;
		const float Distance = Maillage -> GetClosestPointOnCollision(Cible, PointContour);
		if(Distance >= 0.0f){
			return PointContour;
		}
		const FVector DemiTaille = Maillage -> Bounds.BoxExtent;
		return Centre + FVector(0.0f, Direction.Y * DemiTaille.Y, Direction.Z * DemiTaille.Z);
	}

	bool PointDansRectangle(const FVector2D& Point, float X, float Y, float Largeur, float Hauteur){
		return Point.X >= X && Point.X <= X + Largeur && Point.Y >= Y && Point.Y <= Y + Hauteur;
	}

	void AppliquerCouleur(UStaticMeshComponent* Maillage, const FLinearColor& Couleur, UObject* Proprietaire){
		if(!Maillage || !Proprietaire){
			return;
		}
		UMaterialInterface* MateriauBase = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")
		);
		if(!MateriauBase){
			return;
		}
		UMaterialInstanceDynamic* Materiau = UMaterialInstanceDynamic::Create(MateriauBase, Proprietaire);
		if(!Materiau){
			return;
		}
		Materiau -> SetVectorParameterValue(TEXT("Color"), Couleur);
		Maillage -> SetMaterial(0, Materiau);
	}

	void JouerSon(UObject* Contexte, const TCHAR* Chemin, float Volume = 1.0f){
		if(!Contexte){
			return;
		}
		USoundBase* Son = LoadObject<USoundBase>(nullptr, Chemin);
		if(Son){
			UGameplayStatics::PlaySound2D(Contexte, Son, Volume);
		}
	}

}

// Construit le vaisseau, sa silhouette complète, sa tourelle et les composants nécessaires au contrôle.
ASpaceShipPawn::ASpaceShipPawn(){
	// Active la mise à jour du Pawn à chaque image.
	PrimaryActorTick.bCanEverTick = true;
	// Donne automatiquement le contrôle de ce Pawn au premier joueur.
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	// Le maillage d'origine demeure la racine et conserve la collision déjà réglée dans le Blueprint.
	MaillageVaisseau = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MaillageVaisseau"));
	MaillageVaisseau -> SetMobility(EComponentMobility::Movable);
	MaillageVaisseau -> SetGenerateOverlapEvents(true);
	MaillageVaisseau -> SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MaillageVaisseau -> SetCollisionObjectType(ECC_Pawn);
	MaillageVaisseau -> SetCollisionResponseToAllChannels(ECR_Ignore);
	MaillageVaisseau -> SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	RootComponent = MaillageVaisseau;
	// Cache seulement l'ancien cône visuel sans retirer sa collision.
	MaillageVaisseau -> SetVisibility(false, false);
	// Crée le composant qui instancie directement le Blueprint Star Sparrow déjà présent dans Content.
	VisuelStarSparrow = CreateDefaultSubobject<UChildActorComponent>(TEXT("VisuelStarSparrow"));
	VisuelStarSparrow -> SetupAttachment(RootComponent);
	VisuelStarSparrow -> SetRelativeLocation(FVector::ZeroVector);
	VisuelStarSparrow -> SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	VisuelStarSparrow -> SetRelativeScale3D(FVector::OneVector);
	// Le chemin provient directement de la référence copiée dans Unreal.
	static ConstructorHelpers::FClassFinder<AActor> ClasseStarSparrow(TEXT("/Game/StarSparrow/Blueprints/BP_ModularStarSparrow01"));
	if(ClasseStarSparrow.Succeeded()){
		VisuelStarSparrow -> SetChildActorClass(ClasseStarSparrow.Class);
	}
	// Charge les formes de base utilisées pour construire un petit vaisseau stylisé sans dépendance externe.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MaillageCube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MaillageSphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MaillageCone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MaillageCylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	// Crée une coque longue afin que le vaisseau possède une vraie silhouette centrale.
	CoqueVaisseau = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoqueVaisseau"));
	CoqueVaisseau -> SetupAttachment(RootComponent);
	CoqueVaisseau -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoqueVaisseau -> SetRelativeScale3D(FVector(0.24f, 0.34f, 1.05f));
	// Ajoute un nez pointu dans la direction locale +Z utilisée pour le déplacement.
	NezVaisseau = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NezVaisseau"));
	NezVaisseau -> SetupAttachment(RootComponent);
	NezVaisseau -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NezVaisseau -> SetRelativeLocation(FVector(0.0f, 0.0f, 88.0f));
	NezVaisseau -> SetRelativeScale3D(FVector(0.21f, 0.31f, 0.46f));
	// Construit deux ailes inclinées vers l'arrière pour distinguer le vaisseau d'un simple triangle.
	AileGauche = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AileGauche"));
	AileGauche -> SetupAttachment(RootComponent);
	AileGauche -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AileGauche -> SetRelativeLocation(FVector(7.0f, -56.0f, -24.0f));
	AileGauche -> SetRelativeRotation(FRotator(0.0f, 0.0f, -22.0f));
	AileGauche -> SetRelativeScale3D(FVector(0.13f, 0.84f, 0.17f));
	AileDroite = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AileDroite"));
	AileDroite -> SetupAttachment(RootComponent);
	AileDroite -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AileDroite -> SetRelativeLocation(FVector(7.0f, 56.0f, -24.0f));
	AileDroite -> SetRelativeRotation(FRotator(0.0f, 0.0f, 22.0f));
	AileDroite -> SetRelativeScale3D(FVector(0.13f, 0.84f, 0.17f));
	// Place un cockpit arrondi vers la caméra afin qu'il reste visible au-dessus de la coque.
	CockpitVaisseau = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CockpitVaisseau"));
	CockpitVaisseau -> SetupAttachment(RootComponent);
	CockpitVaisseau -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CockpitVaisseau -> SetRelativeLocation(FVector(-31.0f, 0.0f, 25.0f));
	CockpitVaisseau -> SetRelativeScale3D(FVector(0.22f, 0.25f, 0.31f));
	// Ajoute deux moteurs arrière qui donnent un repère clair à l'opposé du nez.
	MoteurGauche = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MoteurGauche"));
	MoteurGauche -> SetupAttachment(RootComponent);
	MoteurGauche -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MoteurGauche -> SetRelativeLocation(FVector(-10.0f, -30.0f, -76.0f));
	MoteurGauche -> SetRelativeScale3D(FVector(0.17f, 0.17f, 0.26f));
	MoteurDroit = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MoteurDroit"));
	MoteurDroit -> SetupAttachment(RootComponent);
	MoteurDroit -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MoteurDroit -> SetRelativeLocation(FVector(-10.0f, 30.0f, -76.0f));
	MoteurDroit -> SetRelativeScale3D(FVector(0.17f, 0.17f, 0.26f));
	// Assigne les formes chargées aux différentes pièces visuelles.
	if(MaillageCube.Succeeded()){
		CoqueVaisseau -> SetStaticMesh(MaillageCube.Object);
		AileGauche -> SetStaticMesh(MaillageCube.Object);
		AileDroite -> SetStaticMesh(MaillageCube.Object);
	}
	if(MaillageSphere.Succeeded()){
		CockpitVaisseau -> SetStaticMesh(MaillageSphere.Object);
		MoteurGauche -> SetStaticMesh(MaillageSphere.Object);
		MoteurDroit -> SetStaticMesh(MaillageSphere.Object);
	}
	if(MaillageCone.Succeeded()){
		NezVaisseau -> SetStaticMesh(MaillageCone.Object);
	}
	// Crée un pivot sans géométrie afin que le canon puisse tourner sans hériter d'une petite échelle.
	PivotTourelle = CreateDefaultSubobject<USceneComponent>(TEXT("PivotTourelle"));
	PivotTourelle -> SetupAttachment(RootComponent);
	PivotTourelle -> SetRelativeLocation(FVector(-68.0f, 0.0f, 5.0f));
	// La base visible reste compacte, mais elle ne sert plus de parent redimensionné au canon.
	Tourelle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tourelle"));
	Tourelle -> SetupAttachment(PivotTourelle);
	Tourelle -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Tourelle -> SetRelativeLocation(FVector::ZeroVector);
	Tourelle -> SetRelativeScale3D(FVector(0.27f, 0.27f, 0.27f));
	// Le canon est volontairement épais et long pour rester visible à l'échelle actuelle du jeu.
	CanonTourelle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CanonTourelle"));
	CanonTourelle -> SetupAttachment(PivotTourelle);
	CanonTourelle -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CanonTourelle -> SetRelativeLocation(FVector(0.0f, 0.0f, 43.0f));
	CanonTourelle -> SetRelativeScale3D(FVector(0.11f, 0.11f, 0.68f));
	// La bouche du canon sert de repère lumineux et de point d'apparition des projectiles.
	BoucheCanon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoucheCanon"));
	BoucheCanon -> SetupAttachment(PivotTourelle);
	BoucheCanon -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoucheCanon -> SetRelativeLocation(FVector(0.0f, 0.0f, 82.0f));
	BoucheCanon -> SetRelativeScale3D(FVector(0.14f, 0.14f, 0.14f));
	// Construit deux flammes derrière les moteurs afin que la propulsion soit visible et animée.
	FlammeMoteurGauche = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlammeMoteurGauche"));
	FlammeMoteurGauche -> SetupAttachment(RootComponent);
	FlammeMoteurGauche -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlammeMoteurGauche -> SetRelativeLocation(FVector(-6.0f, -22.0f, -88.0f));
	FlammeMoteurGauche -> SetRelativeRotation(FRotator(180.0f, 0.0f, 0.0f));
	FlammeMoteurGauche -> SetRelativeScale3D(FVector(0.09f, 0.09f, 0.22f));
	FlammeMoteurDroit = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlammeMoteurDroit"));
	FlammeMoteurDroit -> SetupAttachment(RootComponent);
	FlammeMoteurDroit -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FlammeMoteurDroit -> SetRelativeLocation(FVector(-6.0f, 22.0f, -88.0f));
	FlammeMoteurDroit -> SetRelativeRotation(FRotator(180.0f, 0.0f, 0.0f));
	FlammeMoteurDroit -> SetRelativeScale3D(FVector(0.09f, 0.09f, 0.22f));
	// Les lumières des moteurs amplifient visuellement la poussée lorsqu'on accélère.
	LumiereMoteurGauche = CreateDefaultSubobject<UPointLightComponent>(TEXT("LumiereMoteurGauche"));
	LumiereMoteurGauche -> SetupAttachment(FlammeMoteurGauche);
	LumiereMoteurGauche -> SetLightColor(FLinearColor(1.0f, 0.18f, 0.01f));
	LumiereMoteurGauche -> SetAttenuationRadius(165.0f);
	LumiereMoteurGauche -> SetIntensity(0.0f);
	LumiereMoteurDroit = CreateDefaultSubobject<UPointLightComponent>(TEXT("LumiereMoteurDroit"));
	LumiereMoteurDroit -> SetupAttachment(FlammeMoteurDroit);
	LumiereMoteurDroit -> SetLightColor(FLinearColor(1.0f, 0.18f, 0.01f));
	LumiereMoteurDroit -> SetAttenuationRadius(165.0f);
	LumiereMoteurDroit -> SetIntensity(0.0f);
	// La bouche du canon produit un flash cyan très court à chaque tir.
	LumiereCanon = CreateDefaultSubobject<UPointLightComponent>(TEXT("LumiereCanon"));
	LumiereCanon -> SetupAttachment(BoucheCanon);
	LumiereCanon -> SetLightColor(FLinearColor(0.02f, 0.85f, 1.0f));
	LumiereCanon -> SetAttenuationRadius(190.0f);
	LumiereCanon -> SetIntensity(0.0f);
	// Réutilise les formes intégrées pour éviter tout nouvel asset obligatoire.
	if(MaillageSphere.Succeeded()){
		Tourelle -> SetStaticMesh(MaillageSphere.Object);
		BoucheCanon -> SetStaticMesh(MaillageSphere.Object);
	}
	if(MaillageCylinder.Succeeded()){
		CanonTourelle -> SetStaticMesh(MaillageCylinder.Object);
	}
	if(MaillageCone.Succeeded()){
		FlammeMoteurGauche -> SetStaticMesh(MaillageCone.Object);
		FlammeMoteurDroit -> SetStaticMesh(MaillageCone.Object);
	}
	// Conserve la caméra déclarée dans la classe, mais elle reste inactive car une caméra fixe est créée au démarrage.
	CameraJeu = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraJeu"));
	CameraJeu -> SetupAttachment(RootComponent);
	CameraJeu -> SetAutoActivate(false);
}

// Initialise l'état du jeu, les couleurs, la souris et la caméra fixe.
void ASpaceShipPawn::BeginPlay(){
	Super::BeginPlay();
	if(GEngine){
		GEngine -> Exec(GetWorld(), TEXT("DisableAllScreenMessages"));
	}
	PositionInitialeVaisseau = GetActorLocation();
	RotationInitialeVaisseau = GetActorRotation();
	QuartiersVie = NombreCoeurs * 4;
	Score = 0;
	ProchainSeuilBonus = PointsParBonus;
	bPartieCommencee = false;
	bPartieTerminee = false;
	bChoixBonusActif = false;
	bMenuCommandesActif = false;
	SetActorHiddenInGame(true);
	// Ajuste automatiquement le Blueprint Star Sparrow aux dimensions de la collision existante.
	AjusterVisuelStarSparrow();
	// Lorsque le Star Sparrow est disponible, il devient l'unique apparence du joueur.
	if(VisuelStarSparrow -> GetChildActor()){
		CoqueVaisseau -> SetVisibility(false, false);
		NezVaisseau -> SetVisibility(false, false);
		AileGauche -> SetVisibility(false, false);
		AileDroite -> SetVisibility(false, false);
		CockpitVaisseau -> SetVisibility(false, false);
		MoteurGauche -> SetVisibility(false, false);
		MoteurDroit -> SetVisibility(false, false);
		Tourelle -> SetVisibility(false, false);
		CanonTourelle -> SetVisibility(false, false);
		BoucheCanon -> SetVisibility(false, false);
		FlammeMoteurGauche -> SetVisibility(true, false);
		FlammeMoteurDroit -> SetVisibility(true, false);
		// Les flammes utilisent un orange très vif pour rester visibles même sur le fond noir.
		AppliquerCouleur(FlammeMoteurGauche, FLinearColor(1.0f, 0.075f, 0.004f), this);
		AppliquerCouleur(FlammeMoteurDroit, FLinearColor(1.0f, 0.075f, 0.004f), this);
		// Les intensités sont ensuite pilotées dans MettreAJourEffetsVaisseau.
		LumiereCanon -> SetIntensity(0.0f);
	}
	// Le menu doit également cacher explicitement l'acteur enfant.
	DefinirVisibiliteStarSparrow(false);
	APlayerController* ControleurJoueur = GetWorld() -> GetFirstPlayerController();
	if(ControleurJoueur){
		ControleurJoueur -> bShowMouseCursor = true;
		ControleurJoueur -> bEnableClickEvents = true;
		FInputModeGameAndUI ModeEntree;
		ModeEntree.SetHideCursorDuringCapture(false);
		ControleurJoueur -> SetInputMode(ModeEntree);
	}
	FActorSpawnParameters ParametresCamera;
	ACameraActor* CameraFixe = GetWorld() -> SpawnActor<ACameraActor>(
		ACameraActor::StaticClass(),
		FVector(-1500.0f, 0.0f, 0.0f),
		FRotator(0.0f, 0.0f, 0.0f),
		ParametresCamera
	);
	if(CameraFixe){
		UCameraComponent* ComposantCameraFixe = CameraFixe -> GetCameraComponent();
		ComposantCameraFixe -> ProjectionMode = ECameraProjectionMode::Orthographic;
		ComposantCameraFixe -> OrthoWidth = LargeurZoneJeu;
		ComposantCameraFixe -> bConstrainAspectRatio = false;
		if(ControleurJoueur){
			if(ControleurJoueur -> PlayerCameraManager){
				ControleurJoueur -> PlayerCameraManager -> bDefaultConstrainAspectRatio = false;
			}
			ControleurJoueur -> SetViewTarget(CameraFixe);
		}
	}
	// Lance une musique d'ambiance si l'asset audio a été importé dans /Game/Audio sous le nom S_MusiqueFond.
	JouerSon(this, TEXT("/Game/Audio/S_MusiqueFond.S_MusiqueFond"), 1.35f);
}

// Met à jour la conduite, la visée, le tir, le dash, les vagues et les apparitions.
void ASpaceShipPawn::Tick(float DeltaTime){
	Super::Tick(DeltaTime);
	GererInterfaceSouris();
	if(!bPartieCommencee || bPartieTerminee || bChoixBonusActif){
		return;
	}
	APlayerController* ControleurJoueur = GetWorld() -> GetFirstPlayerController();
	if(!ControleurJoueur){
		return;
	}
	// La visée reste liée à la souris, mais aucune tourelle visuelle n'est utilisée.
	const bool Avancer = ControleurJoueur -> IsInputKeyDown(EKeys::W);
	const bool Reculer = ControleurJoueur -> IsInputKeyDown(EKeys::S);
	if(Avancer && !Reculer){
		if(VitesseActuelle < 0.0f){
			const float RapportFreinage = FMath::Clamp(FMath::Abs(VitesseActuelle) / VitesseMaxArriere, 0.0f, 1.0f);
			const float FreinageActuel = FMath::Lerp(520.0f, FreinageBase, RapportFreinage);
			VitesseActuelle = FMath::Min(0.0f, VitesseActuelle + FreinageActuel * DeltaTime);
		}
		else{
			VitesseActuelle = FMath::Min(VitesseMaxAvant, VitesseActuelle + AccelerationAvant * DeltaTime);
		}
	}
	else if(Reculer && !Avancer){
		if(VitesseActuelle > 0.0f){
			const float RapportFreinage = FMath::Clamp(VitesseActuelle / VitesseMaxAvant, 0.0f, 1.0f);
			const float FreinageActuel = FMath::Lerp(520.0f, FreinageBase, RapportFreinage);
			VitesseActuelle = FMath::Max(0.0f, VitesseActuelle - FreinageActuel * DeltaTime);
		}
		else{
			VitesseActuelle = FMath::Max(-VitesseMaxArriere, VitesseActuelle - AccelerationArriere * DeltaTime);
		}
	}
	else{
		VitesseActuelle = FMath::FInterpConstantTo(VitesseActuelle, 0.0f, DeltaTime, ResistanceNaturelle);
	}
	float CommandeRotation = 0.0f;
	if(ControleurJoueur -> IsInputKeyDown(EKeys::A)){
		CommandeRotation -= 1.0f;
	}
	if(ControleurJoueur -> IsInputKeyDown(EKeys::D)){
		CommandeRotation += 1.0f;
	}
	if(VitesseActuelle < 0.0f){
		CommandeRotation *= -1.0f;
	}
	const float RapportVitesse = FMath::Clamp(FMath::Abs(VitesseActuelle) / VitesseMaxAvant, 0.0f, 1.0f);
	const float InfluenceVitesse = RapportVitesse * RapportVitesse;
	const float RotationMaxActuelle = FMath::Lerp(RotationMaxBasseVitesse, RotationMaxHauteVitesse, InfluenceVitesse);
	const float RotationCible = CommandeRotation * RotationMaxActuelle;
	VitesseRotationActuelle = FMath::FInterpTo(VitesseRotationActuelle, RotationCible, DeltaTime, ReponseRotation);
	if(!FMath::IsNearlyZero(VitesseRotationActuelle, 0.01f)){
		AddActorLocalRotation(FRotator(0.0f, 0.0f, VitesseRotationActuelle * DeltaTime));
	}
	FVector DirectionVaisseau = MaillageVaisseau -> GetUpVector();
	DirectionVaisseau.X = 0.0f;
	DirectionVaisseau.Normalize();
	if(!FMath::IsNearlyZero(VitesseActuelle)){
		AddActorWorldOffset(DirectionVaisseau * VitesseActuelle * DeltaTime, false);
	}
	// Les deux propulseurs C++ restent visibles derrière le Star Sparrow et réagissent à la poussée.
	MettreAJourEffetsVaisseau(Avancer, Reculer, DeltaTime);
	TempsRechargeTir = FMath::Max(0.0f, TempsRechargeTir - DeltaTime);
	if(ChargesDash < ChargesDashMax){
		RechargeDashRestante = FMath::Max(0.0f, RechargeDashRestante - DeltaTime);
		if(RechargeDashRestante <= 0.0f){
			ChargesDash++;
			RechargeDashRestante = ChargesDash < ChargesDashMax ? TempsRechargeDash : 0.0f;
		}
	}
	if(ControleurJoueur -> IsInputKeyDown(EKeys::LeftMouseButton) && TempsRechargeTir <= 0.0f){
		TirerLaser();
		TempsRechargeTir = CadenceTir;
	}
	if(ControleurJoueur -> WasInputKeyJustPressed(EKeys::SpaceBar) && ChargesDash > 0){
		FaireDash();
	}
	MettreAJourVague(DeltaTime);
	// Si la fin de vague vient d'ouvrir les bonus, aucune nouvelle apparition ne doit se produire.
	if(bChoixBonusActif){
		return;
	}
	TempsAvantProchainAsteroide -= DeltaTime;
	if(TempsAvantProchainAsteroide <= 0.0f){
		GenererAsteroide();
		const float FacteurVague = FMath::Pow(0.96f, static_cast<float>(VagueActuelle - 1));
		TempsAvantProchainAsteroide = FMath::Max(
			0.75f,
			FMath::FRandRange(IntervalleAsteroideMin, IntervalleAsteroideMax) * FacteurVague
		);
	}
	TempsAvantProchainEnnemi -= DeltaTime;
	if(TempsAvantProchainEnnemi <= 0.0f){
		const int32 NombreApparitions = FMath::Clamp(1 + (VagueActuelle - 1) / 4, 1, 3);
		for(int32 Index = 0; Index < NombreApparitions; Index++){
			GenererEnnemi();
		}
		const float FacteurVague = FMath::Pow(0.88f, static_cast<float>(VagueActuelle - 1));
		TempsAvantProchainEnnemi = FMath::Max(
			0.85f,
			FMath::FRandRange(IntervalleEnnemiMin, IntervalleEnnemiMax) * FacteurVague
		);
	}
	LimiterPositionDansEcran();
}

// Ajuste automatiquement le Blueprint Star Sparrow à la largeur et à la hauteur de l'ancienne collision.
void ASpaceShipPawn::AjusterVisuelStarSparrow(){
	// Le ChildActor peut être absent si l'asset n'a pas été installé ou si son chemin a changé.
	AActor* VaisseauVisuel = VisuelStarSparrow ? VisuelStarSparrow -> GetChildActor() : nullptr;
	if(!VaisseauVisuel){
		return;
	}
	// Le Star Sparrow sert uniquement de visuel : toutes ses collisions internes sont désactivées pour conserver notre gameplay.
	VaisseauVisuel -> SetActorEnableCollision(false);
	TArray<UPrimitiveComponent*> ComposantsVisuels;
	VaisseauVisuel -> GetComponents<UPrimitiveComponent>(ComposantsVisuels);
	for(UPrimitiveComponent* Composant : ComposantsVisuels){
		if(Composant){
			Composant -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Composant -> SetGenerateOverlapEvents(false);
		}
	}
	// Commence avec la rotation qui transforme l'avant Unreal +X du vaisseau en avant du jeu +Z.
	VisuelStarSparrow -> SetRelativeLocation(FVector::ZeroVector);
	VisuelStarSparrow -> SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	VisuelStarSparrow -> SetRelativeScale3D(FVector::OneVector);
	// Compare les dimensions visibles du Blueprint aux dimensions Y/Z de l'ancien cône de collision.
	const FBox BoiteInitiale = VaisseauVisuel -> GetComponentsBoundingBox(true);
	const FVector TailleInitiale = BoiteInitiale.GetSize();
	const FVector DemiTailleCible = MaillageVaisseau -> Bounds.BoxExtent;
	const float LargeurCible = FMath::Max(1.0f, DemiTailleCible.Y * 2.0f);
	const float HauteurCible = FMath::Max(1.0f, DemiTailleCible.Z * 2.0f);
	const float FacteurLargeur = LargeurCible / FMath::Max(1.0f, TailleInitiale.Y);
	const float FacteurHauteur = HauteurCible / FMath::Max(1.0f, TailleInitiale.Z);
	// Garde le vaisseau légèrement à l'intérieur de l'ancienne collision plutôt que de dépasser sur les bords.
	// L'ancien ajustement remplissait à peine la vieille collision et rendait le modèle ridicule à l'écran.
	// On conserve le calcul automatique mais on vise une silhouette environ 2.8 fois plus grande.
	const float FacteurAffichage = 2.35f;
	const float EchelleFinale = FMath::Clamp(FMath::Min(FacteurLargeur, FacteurHauteur) * FacteurAffichage, 0.001f, 100.0f);
	VisuelStarSparrow -> SetRelativeScale3D(FVector(EchelleFinale));
	// Recalcule les bounds après la mise à l'échelle puis recentre le modèle sur l'ancien vaisseau.
	const FBox BoiteFinale = VaisseauVisuel -> GetComponentsBoundingBox(true);
	const FVector CorrectionMonde = MaillageVaisseau -> Bounds.Origin - BoiteFinale.GetCenter();
	VisuelStarSparrow -> AddWorldOffset(CorrectionMonde, false, nullptr, ETeleportType::TeleportPhysics);
}

// Affiche ou cache le Blueprint enfant en même temps que le Pawn principal.
void ASpaceShipPawn::DefinirVisibiliteStarSparrow(bool bVisible){
	// Le ChildActor peut ne pas exister si son asset n'a pas été chargé.
	if(!VisuelStarSparrow){
		return;
	}
	AActor* VaisseauVisuel = VisuelStarSparrow -> GetChildActor();
	if(VaisseauVisuel){
		VaisseauVisuel -> SetActorHiddenInGame(!bVisible);
	}
}

bool ASpaceShipPawn::ObtenirDirectionCurseur(FVector& Direction) const{
	APlayerController* ControleurJoueur = GetWorld() -> GetFirstPlayerController();
	if(!ControleurJoueur){
		return false;
	}

	FVector OrigineCurseur;
	FVector DirectionRayon;
	if(!ControleurJoueur -> DeprojectMousePositionToWorld(OrigineCurseur, DirectionRayon)){
		return false;
	}

	if(FMath::IsNearlyZero(DirectionRayon.X)){
		return false;
	}

	const float PositionX = GetActorLocation().X;
	const float DistancePlan = (PositionX - OrigineCurseur.X) / DirectionRayon.X;
	const FVector PositionCurseurMonde = OrigineCurseur + DirectionRayon * DistancePlan;
	Direction = PositionCurseurMonde - GetActorLocation();
	Direction.X = 0.0f;
	if(Direction.IsNearlyZero()){
		return false;
	}

	Direction.Normalize();
	return true;
}

// Oriente directement l'axe +Z du canon vers la souris, sans inversion de signe.
void ASpaceShipPawn::MettreAJourTourelle(){
	// Récupère la direction monde exacte vers le curseur.
	FVector Direction;
	if(!ObtenirDirectionCurseur(Direction)){
		return;
	}
	// MakeFromZX construit une rotation dont l'axe Z correspond exactement à la direction de tir.
	const FRotator RotationTourelle = FRotationMatrix::MakeFromZX(Direction, FVector(-1.0f, 0.0f, 0.0f)).Rotator();
	PivotTourelle -> SetWorldRotation(RotationTourelle);
}

// Anime les propulseurs comme deux flammes courtes et nerveuses plutôt que comme des faisceaux de lampe de poche.
void ASpaceShipPawn::MettreAJourEffetsVaisseau(bool Avancer, bool Reculer, float DeltaTime){
	// La commande W donne la pleine poussée, S une poussée arrière plus faible et l'inertie conserve seulement une petite flamme.
	const float RapportVitesse = FMath::Clamp(FMath::Abs(VitesseActuelle) / FMath::Max(1.0f, VitesseMaxAvant), 0.0f, 1.0f);
	const float CiblePropulsion = Avancer ? 1.0f : (Reculer ? 0.55f : RapportVitesse * 0.15f);
	IntensitePropulseurs = FMath::FInterpTo(IntensitePropulseurs, CiblePropulsion, DeltaTime, 12.0f);
	// Deux fréquences superposées créent un scintillement irrégulier sans déplacer brutalement les flammes.
	const float Temps = GetWorld() ? GetWorld() -> GetTimeSeconds() : 0.0f;
	const float PulsationRapide = FMath::Sin(Temps * 41.0f) * 0.08f;
	const float PulsationLente = FMath::Sin(Temps * 17.0f + 1.7f) * 0.05f;
	const float Scintillement = 1.0f + PulsationRapide + PulsationLente;
	// Une flamme de fusée est volontairement plus large et moins longue qu'un faisceau lumineux.
	const float LongueurFlamme = 0.24f + IntensitePropulseurs * 1.28f * Scintillement;
	const float LargeurFlamme = 0.10f + IntensitePropulseurs * 0.085f * Scintillement;
	FlammeMoteurGauche -> SetRelativeScale3D(FVector(LargeurFlamme, LargeurFlamme, LongueurFlamme));
	FlammeMoteurDroit -> SetRelativeScale3D(FVector(LargeurFlamme, LargeurFlamme, LongueurFlamme));
	// Les cônes restent collés aux tuyères et ne reculent que légèrement quand leur longueur augmente.
	const float PositionFlammeZ = -88.0f - IntensitePropulseurs * 42.0f;
	FlammeMoteurGauche -> SetRelativeLocation(FVector(-6.0f, -22.0f, PositionFlammeZ));
	FlammeMoteurDroit -> SetRelativeLocation(FVector(-6.0f, 22.0f, PositionFlammeZ));
	// Une lumière très intense mais de faible rayon produit un halo moteur au lieu d'éclairer toute la scène.
	const float IntensiteLumiere = IntensitePropulseurs * 23000.0f * FMath::Clamp(Scintillement, 0.85f, 1.15f);
	LumiereMoteurGauche -> SetIntensity(IntensiteLumiere);
	LumiereMoteurDroit -> SetIntensity(IntensiteLumiere);
	// Le vieux flash de canon reste désactivé puisque la tourelle visuelle a été abandonnée.
	LumiereCanon -> SetIntensity(0.0f);
}

// Crée les projectiles du joueur selon les bonus de tir actifs.
void ASpaceShipPawn::TirerLaser(){
	FVector Direction;
	if(!ObtenirDirectionCurseur(Direction)){
		return;
	}
	// Vecteur perpendiculaire dans le plan Y/Z pour espacer les tirs multiples.
	const FVector Perpendiculaire(0.0f, -Direction.Z, Direction.Y);
	for(int32 Index = 0; Index < NombreProjectilesParTir; Index++){
		float Decalage = 0.0f;
		if(NombreProjectilesParTir == 2){
			Decalage = Index == 0 ? -18.0f : 18.0f;
		}
		else if(NombreProjectilesParTir >= 3){
			Decalage = (Index - 1) * 22.0f;
		}
		// Sans tourelle, le laser part simplement légèrement devant le centre du vaisseau vers le curseur.
		FVector PositionProjectile = GetActorLocation() + Direction * 72.0f + Perpendiculaire * Decalage;
		// Tous les projectiles restent exactement sur le plan Y/Z où se trouvent les cibles.
		PositionProjectile.X = GetActorLocation().X;
		FActorSpawnParameters Parametres;
		Parametres.Owner = this;
		Parametres.Instigator = this;
		ALaserProjectile* Laser = GetWorld() -> SpawnActor<ALaserProjectile>(
			ALaserProjectile::StaticClass(),
			PositionProjectile,
			FRotator::ZeroRotator,
			Parametres
		);
		if(Laser){
			Laser -> ConfigurerProjectile(Direction * VitesseLaser, false, DegatsLaserJoueur);
		}
	}
	// Joue seulement le son du tir; aucun effet de tourelle n'est nécessaire.
	JouerSon(this, TEXT("/Game/Audio/S_Laser.S_Laser"), 0.28f);
}

// Déplace rapidement le vaisseau vers le curseur et consomme une charge.
void ASpaceShipPawn::FaireDash(){
	FVector Direction;
	if(!ObtenirDirectionCurseur(Direction) || ChargesDash <= 0){
		return;
	}
	AddActorWorldOffset(Direction * DistanceDash, false);
	LimiterPositionDansEcran();
	ChargesDash--;
	if(ChargesDash < ChargesDashMax && RechargeDashRestante <= 0.0f){
		RechargeDashRestante = TempsRechargeDash;
	}
	JouerSon(this, TEXT("/Game/Audio/S_Dash.S_Dash"), 0.85f);
}

// Retire exactement un coeur lors d'une collision avec un astéroïde.
void ASpaceShipPawn::RecevoirImpactAsteroide(){
	QuartiersVie = FMath::Max(0, QuartiersVie - 4);
	if(QuartiersVie <= 0){
		TerminerPartie();
	}
}

// Retire les quarts de coeur infligés par les tirs ennemis.
void ASpaceShipPawn::RecevoirDegatsProjectile(int32 QuartiersDegats){
	QuartiersVie = FMath::Max(0, QuartiersVie - FMath::Max(0, QuartiersDegats));
	if(QuartiersVie <= 0){
		TerminerPartie();
	}
}

// Ajoute les points et mémorise les récompenses qui seront proposées seulement à la fin d'une vague.
void ASpaceShipPawn::AjouterScore(int32 Points){
	// Ignore les points lorsqu'aucune partie active ne doit progresser.
	if(!bPartieCommencee || bPartieTerminee){
		return;
	}
	// Ajoute les points gagnés à la destruction d'une cible.
	Score += FMath::Max(0, Points);
	// Chaque seuil franchi gagne un bonus, mais aucun écran ne coupe le combat en cours.
	while(Score >= ProchainSeuilBonus){
		BonusEnAttente++;
		ProchainSeuilBonus += PointsParBonus;
	}
}

// Choisit trois améliorations différentes à présenter entre deux vagues.
void ASpaceShipPawn::PreparerChoixBonus(){
	bChoixBonusActif = true;
	BonusProposes.Reset();
	TArray<EBonusType> Disponibles = {
		EBonusType::TirMultiple,
		EBonusType::Degats,
		EBonusType::Cadence,
		EBonusType::ChargeDash,
		EBonusType::RechargeDash,
		EBonusType::Vie,
		EBonusType::Vitesse,
		EBonusType::Rotation,
		EBonusType::Acceleration
	};
	// Mélange simple et choix de trois bonus différents.
	for(int32 Index = Disponibles.Num() - 1; Index > 0; Index--){
		const int32 Autre = FMath::RandRange(0, Index);
		Disponibles.Swap(Index, Autre);
	}
	for(int32 Index = 0; Index < 3 && Index < Disponibles.Num(); Index++){
		BonusProposes.Add(Disponibles[Index]);
	}
}

// Applique la carte choisie puis autorise le démarrage de la vague suivante.
void ASpaceShipPawn::ChoisirBonus(int32 Index){
	// Refuse un clic qui ne correspond pas à une des trois cartes valides.
	if(!bChoixBonusActif || !BonusProposes.IsValidIndex(Index)){
		return;
	}
	// Applique l'amélioration sélectionnée et ferme l'écran de récompense.
	AppliquerBonus(BonusProposes[Index]);
	bChoixBonusActif = false;
	BonusProposes.Reset();
	BonusEnAttente = FMath::Max(0, BonusEnAttente - 1);
	// La vague suivante commence seulement après le choix.
	if(bPassageVagueEnAttente){
		bPassageVagueEnAttente = false;
		VagueActuelle++;
		TempsVagueRestant = DureeVague;
		TempsAffichageVague = 2.5f;
	}
	// Confirme le choix avec le son de bonus.
	JouerSon(this, TEXT("/Game/Audio/S_Bonus.S_Bonus"), 0.90f);
}

// Modifie les statistiques du joueur selon le bonus choisi.
void ASpaceShipPawn::AppliquerBonus(EBonusType Bonus){
	switch(Bonus){
		case EBonusType::TirMultiple:
			NombreProjectilesParTir = FMath::Min(3, NombreProjectilesParTir + 1);
			break;
		case EBonusType::Degats:
			DegatsLaserJoueur++;
			break;
		case EBonusType::Cadence:
			CadenceTir = FMath::Max(0.055f, CadenceTir * 0.82f);
			break;
		case EBonusType::ChargeDash:
			ChargesDashMax = FMath::Min(4, ChargesDashMax + 1);
			ChargesDash = FMath::Min(ChargesDashMax, ChargesDash + 1);
			break;
		case EBonusType::RechargeDash:
			TempsRechargeDash = FMath::Max(0.75f, TempsRechargeDash * 0.82f);
			break;
		case EBonusType::Vie:
			NombreCoeurs++;
			QuartiersVie += 4;
			break;
		case EBonusType::Vitesse:
			VitesseMaxAvant *= 1.12f;
			VitesseMaxArriere *= 1.08f;
			break;
		case EBonusType::Rotation:
			RotationMaxBasseVitesse *= 1.12f;
			RotationMaxHauteVitesse *= 1.12f;
			ReponseRotation *= 1.08f;
			break;
		case EBonusType::Acceleration:
			AccelerationAvant *= 1.15f;
			AccelerationArriere *= 1.12f;
			break;
	}
}

FString ASpaceShipPawn::NomBonus(EBonusType Bonus) const{
	switch(Bonus){
		case EBonusType::TirMultiple: return TEXT("TIR MULTIPLE");
		case EBonusType::Degats: return TEXT("LASER RENFORCE");
		case EBonusType::Cadence: return TEXT("CADENCE +");
		case EBonusType::ChargeDash: return TEXT("CHARGE DE DASH +");
		case EBonusType::RechargeDash: return TEXT("RECHARGE DASH +");
		case EBonusType::Vie: return TEXT("COEUR +");
		case EBonusType::Vitesse: return TEXT("PROPULSION +");
		case EBonusType::Rotation: return TEXT("MANOEUVRABILITE +");
		case EBonusType::Acceleration: return TEXT("ACCELERATION +");
	}

	return TEXT("BONUS");
}

FString ASpaceShipPawn::DescriptionBonus(EBonusType Bonus) const{
	switch(Bonus){
		case EBonusType::TirMultiple: return TEXT("Ajoute un projectile a chaque tir.");
		case EBonusType::Degats: return TEXT("Chaque laser inflige davantage de degats.");
		case EBonusType::Cadence: return TEXT("Reduit le temps entre deux tirs.");
		case EBonusType::ChargeDash: return TEXT("Ajoute une charge de dash.");
		case EBonusType::RechargeDash: return TEXT("Recharge les dash plus rapidement.");
		case EBonusType::Vie: return TEXT("Ajoute un coeur maximum et le soigne.");
		case EBonusType::Vitesse: return TEXT("Augmente la vitesse maximale.");
		case EBonusType::Rotation: return TEXT("Ameliore la rotation a toutes les vitesses.");
		case EBonusType::Acceleration: return TEXT("Augmente l'acceleration avant et arriere.");
	}

	return TEXT("");
}

FString ASpaceShipPawn::ObtenirNomBonus(int32 Index) const{
	return BonusProposes.IsValidIndex(Index) ? NomBonus(BonusProposes[Index]) : TEXT("");
}

FString ASpaceShipPawn::ObtenirDescriptionBonus(int32 Index) const{
	return BonusProposes.IsValidIndex(Index) ? DescriptionBonus(BonusProposes[Index]) : TEXT("");
}

// Termine une vague, propose éventuellement un bonus, puis déclenche la vague suivante.
void ASpaceShipPawn::MettreAJourVague(float DeltaTime){
	// Décompte le temps restant dans la vague actuelle.
	TempsVagueRestant -= DeltaTime;
	TempsAffichageVague = FMath::Max(0.0f, TempsAffichageVague - DeltaTime);
	// Aucune transition n'est nécessaire tant que la vague continue.
	if(TempsVagueRestant > 0.0f){
		return;
	}
	// Un bonus gagné pendant la vague est proposé uniquement maintenant, entre deux vagues.
	if(BonusEnAttente > 0){
		bPassageVagueEnAttente = true;
		PreparerChoixBonus();
		return;
	}
	// Sans bonus en attente, la prochaine vague démarre immédiatement.
	VagueActuelle++;
	TempsVagueRestant = DureeVague;
	TempsAffichageVague = 2.5f;
}

// Réinitialise les statistiques temporaires et lance une nouvelle partie.
void ASpaceShipPawn::CommencerPartie(){
	NettoyerActeursJeu();
	bPartieCommencee = true;
	bPartieTerminee = false;
	bChoixBonusActif = false;
	bMenuCommandesActif = false;
	QuartiersVie = NombreCoeurs * 4;
	Score = 0;
	ProchainSeuilBonus = PointsParBonus;
	BonusEnAttente = 0;
	bPassageVagueEnAttente = false;
	VagueActuelle = 1;
	TempsVagueRestant = DureeVague;
	TempsAffichageVague = 2.5f;
	VitesseActuelle = 0.0f;
	VitesseRotationActuelle = 0.0f;
	TempsRechargeTir = 0.0f;
	NombreProjectilesParTir = 1;
	DegatsLaserJoueur = 1;
	ChargesDashMax = 1;
	ChargesDash = 1;
	RechargeDashRestante = 0.0f;
	TempsAvantProchainAsteroide = FMath::FRandRange(0.6f, 1.2f);
	TempsAvantProchainEnnemi = FMath::FRandRange(3.0f, 4.5f);
	SetActorLocation(PositionInitialeVaisseau);
	SetActorRotation(RotationInitialeVaisseau);
	SetActorHiddenInGame(false);
	DefinirVisibiliteStarSparrow(true);
}

// Nettoie la partie active et replace le jeu dans l'état du menu principal.
void ASpaceShipPawn::RetournerMenu(){
	NettoyerActeursJeu();
	bPartieCommencee = false;
	bPartieTerminee = false;
	bChoixBonusActif = false;
	bPassageVagueEnAttente = false;
	bMenuCommandesActif = false;
	VitesseActuelle = 0.0f;
	VitesseRotationActuelle = 0.0f;
	SetActorLocation(PositionInitialeVaisseau);
	SetActorRotation(RotationInitialeVaisseau);
	SetActorHiddenInGame(true);
	DefinirVisibiliteStarSparrow(false);
}

// Arrête le gameplay lorsque le joueur n'a plus de vie.
void ASpaceShipPawn::TerminerPartie(){
	bPartieTerminee = true;
	bChoixBonusActif = false;
	bPassageVagueEnAttente = false;
	VitesseActuelle = 0.0f;
	VitesseRotationActuelle = 0.0f;
	NettoyerActeursJeu();
	SetActorHiddenInGame(true);
	DefinirVisibiliteStarSparrow(false);
}

// Gère les clics dans les menus, le choix de bonus et l'écran de fin.
void ASpaceShipPawn::GererInterfaceSouris(){
	APlayerController* ControleurJoueur = GetWorld() -> GetFirstPlayerController();
	if(!ControleurJoueur || !ControleurJoueur -> WasInputKeyJustPressed(EKeys::LeftMouseButton)){
		return;
	}
	float SourisX = 0.0f;
	float SourisY = 0.0f;
	if(!ControleurJoueur -> GetMousePosition(SourisX, SourisY)){
		return;
	}
	int32 Largeur = 0;
	int32 Hauteur = 0;
	ControleurJoueur -> GetViewportSize(Largeur, Hauteur);
	const FVector2D PositionSouris(SourisX, SourisY);
	const float LargeurBouton = 320.0f;
	const float HauteurBouton = 62.0f;
	const float X = Largeur * 0.5f - LargeurBouton * 0.5f;
	// Choix de bonus en priorité.
	if(bChoixBonusActif){
		const float LargeurCarte = 300.0f;
		const float HauteurCarte = 230.0f;
		const float Espace = 26.0f;
		const float LargeurTotale = LargeurCarte * 3.0f + Espace * 2.0f;
		const float XDepart = Largeur * 0.5f - LargeurTotale * 0.5f;
		const float Y = Hauteur * 0.5f - HauteurCarte * 0.5f;
		for(int32 Index = 0; Index < 3; Index++){
			const float XCarte = XDepart + Index * (LargeurCarte + Espace);
			if(PointDansRectangle(PositionSouris, XCarte, Y, LargeurCarte, HauteurCarte)){
				ChoisirBonus(Index);
				return;
			}
		}
		return;
	}
	// Menu principal et sous-menu commandes.
	if(!bPartieCommencee){
		if(bMenuCommandesActif){
			const float YRetour = Hauteur * 0.78f;
			if(PointDansRectangle(PositionSouris, X, YRetour, LargeurBouton, HauteurBouton)){
				bMenuCommandesActif = false;
			}
			return;
		}
		const float YJouer = Hauteur * 0.52f;
		const float YCommandes = YJouer + 78.0f;
		const float YQuitter = YCommandes + 78.0f;
		if(PointDansRectangle(PositionSouris, X, YJouer, LargeurBouton, HauteurBouton)){
			CommencerPartie();
			return;
		}
		if(PointDansRectangle(PositionSouris, X, YCommandes, LargeurBouton, HauteurBouton)){
			bMenuCommandesActif = true;
			return;
		}
		if(PointDansRectangle(PositionSouris, X, YQuitter, LargeurBouton, HauteurBouton)){
			UKismetSystemLibrary::QuitGame(GetWorld(), ControleurJoueur, EQuitPreference::Quit, false);
			return;
		}
	}
	if(bPartieTerminee){
		// Reproduit exactement les dimensions et positions utilisées par le HUD de fin.
		const float LargeurPanneau = 560.0f;
		const float HauteurPanneau = 440.0f;
		const float YPanneau = Hauteur * 0.5f - HauteurPanneau * 0.5f;
		const float LargeurBoutonFin = 300.0f;
		const float HauteurBoutonFin = 58.0f;
		const float XBoutonFin = Largeur * 0.5f - LargeurBoutonFin * 0.5f;
		const float YRejouer = YPanneau + 260.0f;
		const float YMenu = YRejouer + 76.0f;
		if(PointDansRectangle(PositionSouris, XBoutonFin, YRejouer, LargeurBoutonFin, HauteurBoutonFin)){
			CommencerPartie();
			return;
		}
		if(PointDansRectangle(PositionSouris, XBoutonFin, YMenu, LargeurBoutonFin, HauteurBoutonFin)){
			RetournerMenu();
		}
	}
}

int32 ASpaceShipPawn::ObtenirQuartiersVie() const{
	return QuartiersVie;
}

int32 ASpaceShipPawn::ObtenirQuartiersVieMax() const{
	return NombreCoeurs * 4;
}

int32 ASpaceShipPawn::ObtenirScore() const{
	return Score;
}

int32 ASpaceShipPawn::ObtenirVague() const{
	return VagueActuelle;
}

int32 ASpaceShipPawn::ObtenirChargesDash() const{
	return ChargesDash;
}

int32 ASpaceShipPawn::ObtenirChargesDashMax() const{
	return ChargesDashMax;
}

float ASpaceShipPawn::ObtenirRechargeDashRestante() const{
	return RechargeDashRestante;
}

float ASpaceShipPawn::ObtenirTempsRechargeDash() const{
	return TempsRechargeDash;
}

bool ASpaceShipPawn::EstPartieCommencee() const{
	return bPartieCommencee;
}

bool ASpaceShipPawn::EstPartieTerminee() const{
	return bPartieTerminee;
}

bool ASpaceShipPawn::EstChoixBonusActif() const{
	return bChoixBonusActif;
}

bool ASpaceShipPawn::EstMenuCommandesActif() const{
	return bMenuCommandesActif;
}

FString ASpaceShipPawn::ObtenirNomsMembres() const{
	return NomsMembres;
}

// Détruit les astéroïdes, ennemis et projectiles encore présents.
void ASpaceShipPawn::NettoyerActeursJeu(){
	TArray<AActor*> Acteurs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAsteroide::StaticClass(), Acteurs);
	for(AActor* Acteur : Acteurs){
		Acteur -> Destroy();
	}
	Acteurs.Reset();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnnemi::StaticClass(), Acteurs);
	for(AActor* Acteur : Acteurs){
		Acteur -> Destroy();
	}
	Acteurs.Reset();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALaserProjectile::StaticClass(), Acteurs);
	for(AActor* Acteur : Acteurs){
		Acteur -> Destroy();
	}
}

bool ASpaceShipPawn::ObtenirLimitesMondeEcran(float& MinimumY, float& MaximumY, float& MinimumZ, float& MaximumZ) const{
	APlayerController* ControleurJoueur = GetWorld() -> GetFirstPlayerController();
	if(!ControleurJoueur){
		return false;
	}

	int32 LargeurFenetre = 0;
	int32 HauteurFenetre = 0;
	ControleurJoueur -> GetViewportSize(LargeurFenetre, HauteurFenetre);
	if(LargeurFenetre <= 0 || HauteurFenetre <= 0){
		return false;
	}

	FVector OrigineHautGauche;
	FVector DirectionHautGauche;
	FVector OrigineBasDroite;
	FVector DirectionBasDroite;
	const bool HautGaucheValide = ControleurJoueur -> DeprojectScreenPositionToWorld(
		0.0f,
		0.0f,
		OrigineHautGauche,
		DirectionHautGauche
	);
	const bool BasDroiteValide = ControleurJoueur -> DeprojectScreenPositionToWorld(
		static_cast<float>(LargeurFenetre),
		static_cast<float>(HauteurFenetre),
		OrigineBasDroite,
		DirectionBasDroite
	);
	if(!HautGaucheValide || !BasDroiteValide){
		return false;
	}

	const float PositionX = GetActorLocation().X;
	if(FMath::IsNearlyZero(DirectionHautGauche.X) || FMath::IsNearlyZero(DirectionBasDroite.X)){
		return false;
	}

	const float DistanceHautGauche = (PositionX - OrigineHautGauche.X) / DirectionHautGauche.X;
	const float DistanceBasDroite = (PositionX - OrigineBasDroite.X) / DirectionBasDroite.X;
	const FVector CoinHautGauche = OrigineHautGauche + DirectionHautGauche * DistanceHautGauche;
	const FVector CoinBasDroite = OrigineBasDroite + DirectionBasDroite * DistanceBasDroite;
	MinimumY = FMath::Min(CoinHautGauche.Y, CoinBasDroite.Y);
	MaximumY = FMath::Max(CoinHautGauche.Y, CoinBasDroite.Y);
	MinimumZ = FMath::Min(CoinHautGauche.Z, CoinBasDroite.Z);
	MaximumZ = FMath::Max(CoinHautGauche.Z, CoinBasDroite.Z);
	return true;
}

// Crée un astéroïde sur un bord aléatoire avec une trajectoire aléatoire.
void ASpaceShipPawn::GenererAsteroide(){
	float MinimumY = 0.0f;
	float MaximumY = 0.0f;
	float MinimumZ = 0.0f;
	float MaximumZ = 0.0f;
	if(!ObtenirLimitesMondeEcran(MinimumY, MaximumY, MinimumZ, MaximumZ)){
		return;
	}
	const int32 Bord = FMath::RandRange(0, 3);
	const float PositionX = GetActorLocation().X;
	const float MargeApparition = 190.0f;
	FVector PositionApparitionAsteroide(PositionX, 0.0f, 0.0f);
	if(Bord == 0){
		PositionApparitionAsteroide.Y = MinimumY - MargeApparition;
		PositionApparitionAsteroide.Z = FMath::FRandRange(MinimumZ, MaximumZ);
	}
	else if(Bord == 1){
		PositionApparitionAsteroide.Y = MaximumY + MargeApparition;
		PositionApparitionAsteroide.Z = FMath::FRandRange(MinimumZ, MaximumZ);
	}
	else if(Bord == 2){
		PositionApparitionAsteroide.Y = FMath::FRandRange(MinimumY, MaximumY);
		PositionApparitionAsteroide.Z = MinimumZ - MargeApparition;
	}
	else{
		PositionApparitionAsteroide.Y = FMath::FRandRange(MinimumY, MaximumY);
		PositionApparitionAsteroide.Z = MaximumZ + MargeApparition;
	}
	const FVector PositionCible(
		PositionX,
		FMath::FRandRange(MinimumY * 0.82f, MaximumY * 0.82f),
		FMath::FRandRange(MinimumZ * 0.82f, MaximumZ * 0.82f)
	);
	FVector Direction = PositionCible - PositionApparitionAsteroide;
	Direction.X = 0.0f;
	Direction.Normalize();
	const float FacteurVague = 1.0f + (VagueActuelle - 1) * 0.025f;
	const FVector VitesseAsteroide = Direction * FMath::FRandRange(VitesseAsteroideMin, VitesseAsteroideMax) * FacteurVague;
	FActorSpawnParameters Parametres;
	AAsteroide* NouvelAsteroide = GetWorld() -> SpawnActor<AAsteroide>(
		AAsteroide::StaticClass(),
		PositionApparitionAsteroide,
		FRotator::ZeroRotator,
		Parametres
	);
	if(NouvelAsteroide){
		NouvelAsteroide -> ConfigurerAsteroide(VitesseAsteroide);
	}
}

// Crée un ennemi et adapte sa difficulté à la vague actuelle.
void ASpaceShipPawn::GenererEnnemi(){
	float MinimumY = 0.0f;
	float MaximumY = 0.0f;
	float MinimumZ = 0.0f;
	float MaximumZ = 0.0f;
	if(!ObtenirLimitesMondeEcran(MinimumY, MaximumY, MinimumZ, MaximumZ)){
		return;
	}
	const int32 Bord = FMath::RandRange(0, 3);
	const float PositionX = GetActorLocation().X;
	const float MargeApparition = 90.0f;
	FVector PositionApparitionEnnemi(PositionX, 0.0f, 0.0f);
	if(Bord == 0){
		PositionApparitionEnnemi.Y = MinimumY - MargeApparition;
		PositionApparitionEnnemi.Z = FMath::FRandRange(MinimumZ, MaximumZ);
	}
	else if(Bord == 1){
		PositionApparitionEnnemi.Y = MaximumY + MargeApparition;
		PositionApparitionEnnemi.Z = FMath::FRandRange(MinimumZ, MaximumZ);
	}
	else if(Bord == 2){
		PositionApparitionEnnemi.Y = FMath::FRandRange(MinimumY, MaximumY);
		PositionApparitionEnnemi.Z = MinimumZ - MargeApparition;
	}
	else{
		PositionApparitionEnnemi.Y = FMath::FRandRange(MinimumY, MaximumY);
		PositionApparitionEnnemi.Z = MaximumZ + MargeApparition;
	}
	const float MultiplicateurVitesse = 1.0f + (VagueActuelle - 1) * 0.07f;
	const float Vitesse = FMath::FRandRange(VitesseEnnemiMin, VitesseEnnemiMax) * MultiplicateurVitesse;
	FActorSpawnParameters Parametres;
	AEnnemi* NouvelEnnemi = GetWorld() -> SpawnActor<AEnnemi>(
		AEnnemi::StaticClass(),
		PositionApparitionEnnemi,
		FRotator::ZeroRotator,
		Parametres
	);
	if(NouvelEnnemi){
		NouvelEnnemi -> ConfigurerEnnemi(this, Vitesse, VagueActuelle);
	}
}

// Empêche le contour du vaisseau de sortir du viewport.
void ASpaceShipPawn::LimiterPositionDansEcran(){
	APlayerController* ControleurJoueur = GetWorld() -> GetFirstPlayerController();
	if(!ControleurJoueur){
		return;
	}
	int32 LargeurFenetre = 0;
	int32 HauteurFenetre = 0;
	ControleurJoueur -> GetViewportSize(LargeurFenetre, HauteurFenetre);
	if(LargeurFenetre <= 0 || HauteurFenetre <= 0){
		return;
	}
	FVector OrigineHautGauche;
	FVector DirectionHautGauche;
	FVector OrigineBasDroite;
	FVector DirectionBasDroite;
	const bool HautGaucheValide = ControleurJoueur -> DeprojectScreenPositionToWorld(
		0.0f,
		0.0f,
		OrigineHautGauche,
		DirectionHautGauche
	);
	const bool BasDroiteValide = ControleurJoueur -> DeprojectScreenPositionToWorld(
		static_cast<float>(LargeurFenetre),
		static_cast<float>(HauteurFenetre),
		OrigineBasDroite,
		DirectionBasDroite
	);
	if(!HautGaucheValide || !BasDroiteValide){
		return;
	}
	const float PositionX = GetActorLocation().X;
	if(FMath::IsNearlyZero(DirectionHautGauche.X) || FMath::IsNearlyZero(DirectionBasDroite.X)){
		return;
	}
	const float DistanceHautGauche = (PositionX - OrigineHautGauche.X) / DirectionHautGauche.X;
	const float DistanceBasDroite = (PositionX - OrigineBasDroite.X) / DirectionBasDroite.X;
	const FVector CoinHautGauche = OrigineHautGauche + DirectionHautGauche * DistanceHautGauche;
	const FVector CoinBasDroite = OrigineBasDroite + DirectionBasDroite * DistanceBasDroite;
	const float MinimumY = FMath::Min(CoinHautGauche.Y, CoinBasDroite.Y);
	const float MaximumY = FMath::Max(CoinHautGauche.Y, CoinBasDroite.Y);
	const float MinimumZ = FMath::Min(CoinHautGauche.Z, CoinBasDroite.Z);
	const float MaximumZ = FMath::Max(CoinHautGauche.Z, CoinBasDroite.Z);
	const FVector CentreVaisseau = MaillageVaisseau -> GetComponentLocation();
	const FVector PointGauche = ObtenirPointContour(MaillageVaisseau, FVector(0.0f, -1.0f, 0.0f));
	const FVector PointDroite = ObtenirPointContour(MaillageVaisseau, FVector(0.0f, 1.0f, 0.0f));
	const FVector PointBas = ObtenirPointContour(MaillageVaisseau, FVector(0.0f, 0.0f, -1.0f));
	const FVector PointHaut = ObtenirPointContour(MaillageVaisseau, FVector(0.0f, 0.0f, 1.0f));
	const float DistanceGauche = CentreVaisseau.Y - PointGauche.Y;
	const float DistanceDroite = PointDroite.Y - CentreVaisseau.Y;
	const float DistanceBas = CentreVaisseau.Z - PointBas.Z;
	const float DistanceHaut = PointHaut.Z - CentreVaisseau.Z;
	FVector Position = GetActorLocation();
	Position.Y = FMath::Clamp(Position.Y, MinimumY + DistanceGauche, MaximumY - DistanceDroite);
	Position.Z = FMath::Clamp(Position.Z, MinimumZ + DistanceBas, MaximumZ - DistanceHaut);
	SetActorLocation(Position);
}
