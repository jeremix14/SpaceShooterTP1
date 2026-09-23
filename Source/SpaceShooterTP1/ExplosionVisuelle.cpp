#include "ExplosionVisuelle.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
// Construit la sphère et la lumière de l'explosion.
AExplosionVisuelle::AExplosionVisuelle(){
	PrimaryActorTick.bCanEverTick = true;
	MaillageExplosion = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MaillageExplosion"));
	RootComponent = MaillageExplosion;
	MaillageExplosion -> SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MaillageExplosion -> SetWorldScale3D(FVector(0.12f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> MaillageSphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if(MaillageSphere.Succeeded()){
		MaillageExplosion -> SetStaticMesh(MaillageSphere.Object);
	}
	LumiereExplosion = CreateDefaultSubobject<UPointLightComponent>(TEXT("LumiereExplosion"));
	LumiereExplosion -> SetupAttachment(RootComponent);
	LumiereExplosion -> SetIntensity(12000.0f);
	LumiereExplosion -> SetAttenuationRadius(360.0f);
	LumiereExplosion -> SetLightColor(FLinearColor(1.0f, 0.20f, 0.015f));
}

// Applique la couleur, joue le son et programme la disparition.
void AExplosionVisuelle::BeginPlay(){
	Super::BeginPlay();
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(
		nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")
	);
	if(Base){
		UMaterialInstanceDynamic* Materiau = UMaterialInstanceDynamic::Create(Base, this);
		if(Materiau){
			Materiau -> SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.15f, 0.015f));
			MaillageExplosion -> SetMaterial(0, Materiau);
		}
	}
	USoundBase* Son = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_Explosion.S_Explosion"));
	if(Son){
		UGameplayStatics::PlaySound2D(this, Son, 0.78f);
	}
	SetLifeSpan(Duree);
}

// Agrandit l'explosion et diminue progressivement sa lumière.
void AExplosionVisuelle::Tick(float DeltaTime){
	Super::Tick(DeltaTime);
	TempsEcoule += DeltaTime;
	const float Progression = FMath::Clamp(TempsEcoule / Duree, 0.0f, 1.0f);
	const float Taille = FMath::Lerp(0.12f, 0.95f, Progression);
	SetActorScale3D(FVector(Taille));
	LumiereExplosion -> SetIntensity(FMath::Lerp(12000.0f, 0.0f, Progression));
}
