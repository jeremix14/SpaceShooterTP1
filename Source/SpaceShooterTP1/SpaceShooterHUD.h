#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SpaceShooterHUD.generated.h"

class UTexture2D;

UCLASS()
class SPACESHOOTERTP1_API ASpaceShooterHUD : public AHUD{
	GENERATED_BODY()

public:

	virtual void DrawHUD() override;

private:

	// Texture blanche avec alpha générée une seule fois; la couleur est appliquée au dessin.
	UPROPERTY()
	UTexture2D* TextureCoeur = nullptr;

	void InitialiserTextureCoeur();
	void DessinerFondEtoile(float Largeur, float Hauteur);
	void DessinerMenu(float Largeur, float Hauteur);
	void DessinerCommandes(float Largeur, float Hauteur);
	void DessinerInterfaceJeu(float Largeur, float Hauteur);
	void DessinerChoixBonus(float Largeur, float Hauteur);
	void DessinerFinPartie(float Largeur, float Hauteur);

	void DessinerBouton(const FString& Texte, float X, float Y, float Largeur, float Hauteur);
	void DessinerCarteBonus(const FString& Titre, const FString& Description, float X, float Y, float Largeur, float Hauteur, bool bSurvole);
	// Dessine un coeur composé de quatre morceaux afin de retirer visuellement un quart à la fois.
	void DessinerCoeur(float X, float Y, float Taille, int32 QuartiersRemplis);
	// Dessine chaque charge de dash sous forme de barre qui se remplit en bleu puis devient verte.
	void DessinerBarresDash(float X, float Y, int32 Charges, int32 ChargesMax, float RechargeRestante, float TempsRecharge);
};
