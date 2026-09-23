#include "SpaceShooterHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "SpaceShipPawn.h"
namespace{
	bool PointDansRectangleHUD(const FVector2D& Point, float X, float Y, float Largeur, float Hauteur){
		return Point.X >= X && Point.X <= X + Largeur && Point.Y >= Y && Point.Y <= Y + Hauteur;
	}

}

// Choisit l'écran d'interface à dessiner selon l'état de la partie.
void ASpaceShooterHUD::DrawHUD(){
	Super::DrawHUD();
	if(!Canvas || !PlayerOwner){
		return;
	}
	// Crée paresseusement une vraie icône de coeur avec transparence lors du premier dessin.
	if(!TextureCoeur){
		InitialiserTextureCoeur();
	}
	ASpaceShipPawn* Vaisseau = Cast<ASpaceShipPawn>(PlayerOwner -> GetPawn());
	if(!Vaisseau){
		return;
	}
	const float Largeur = Canvas -> SizeX;
	const float Hauteur = Canvas -> SizeY;
	// Quelques étoiles donnent de la profondeur au noir sans masquer le monde 3D.
	DessinerFondEtoile(Largeur, Hauteur);
	if(!Vaisseau -> EstPartieCommencee()){
		if(Vaisseau -> EstMenuCommandesActif()){
			DessinerCommandes(Largeur, Hauteur);
		}
		else{
			DessinerMenu(Largeur, Hauteur);
		}
		return;
	}
	if(Vaisseau -> EstPartieTerminee()){
		DessinerFinPartie(Largeur, Hauteur);
		return;
	}
	DessinerInterfaceJeu(Largeur, Hauteur);
	if(Vaisseau -> EstChoixBonusActif()){
		DessinerChoixBonus(Largeur, Hauteur);
	}
}

// Génère en mémoire une icône de coeur 128x128 avec alpha lissé, sans fichier image externe.
void ASpaceShooterHUD::InitialiserTextureCoeur(){
	// Crée une texture transitoire qui restera référencée par le HUD pendant toute la partie.
	const int32 TailleTexture = 128;
	TextureCoeur = UTexture2D::CreateTransient(TailleTexture, TailleTexture, PF_B8G8R8A8, TEXT("TextureCoeurHUD"));
	if(!TextureCoeur || !TextureCoeur -> GetPlatformData() || TextureCoeur -> GetPlatformData() -> Mips.Num() == 0){
		TextureCoeur = nullptr;
		return;
	}
	TextureCoeur -> Filter = TF_Bilinear;
	TextureCoeur -> NeverStream = true;
	// Écrit les pixels directement dans le premier mip en utilisant l'équation implicite classique d'un coeur.
	FTexture2DMipMap& Mip = TextureCoeur -> GetPlatformData() -> Mips[0];
	FColor* Pixels = static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
	const int32 SousEchantillons = 4;
	for(int32 Y = 0; Y < TailleTexture; Y++){
		for(int32 X = 0; X < TailleTexture; X++){
			int32 Interieur = 0;
			for(int32 SY = 0; SY < SousEchantillons; SY++){
				for(int32 SX = 0; SX < SousEchantillons; SX++){
					const float PixelX = static_cast<float>(X) + (static_cast<float>(SX) + 0.5f) / SousEchantillons;
					const float PixelY = static_cast<float>(Y) + (static_cast<float>(SY) + 0.5f) / SousEchantillons;
					const float NormaliseX = -1.40f + PixelX / TailleTexture * 2.80f;
					const float NormaliseY = 1.30f - PixelY / TailleTexture * 2.60f;
					const float Somme = NormaliseX * NormaliseX + NormaliseY * NormaliseY - 1.0f;
					const float Equation = Somme * Somme * Somme - NormaliseX * NormaliseX * NormaliseY * NormaliseY * NormaliseY;
					if(Equation <= 0.0f){
						Interieur++;
					}
				}
			}
			const uint8 Alpha = static_cast<uint8>(255.0f * static_cast<float>(Interieur) / static_cast<float>(SousEchantillons * SousEchantillons));
			Pixels[Y * TailleTexture + X] = FColor(255, 255, 255, Alpha);
		}
	}
	Mip.BulkData.Unlock();
	TextureCoeur -> UpdateResource();
}

// Dessine un fond spatial plus riche avec davantage d'étoiles et quelques nappes colorées très discrètes.
void ASpaceShooterHUD::DessinerFondEtoile(float Largeur, float Hauteur){
	// Quelques nuages de gaz très translucides cassent le noir absolu sans masquer l'action 3D.
	DrawRect(FLinearColor(0.05f, 0.10f, 0.22f, 0.08f), Largeur * 0.08f, Hauteur * 0.10f, Largeur * 0.26f, Hauteur * 0.18f);
	DrawRect(FLinearColor(0.15f, 0.04f, 0.18f, 0.07f), Largeur * 0.62f, Hauteur * 0.12f, Largeur * 0.24f, Hauteur * 0.16f);
	DrawRect(FLinearColor(0.03f, 0.16f, 0.22f, 0.06f), Largeur * 0.18f, Hauteur * 0.62f, Largeur * 0.22f, Hauteur * 0.20f);
	DrawRect(FLinearColor(0.18f, 0.08f, 0.05f, 0.05f), Largeur * 0.68f, Hauteur * 0.68f, Largeur * 0.20f, Hauteur * 0.16f);
	// Motif déterministe : aucune étoile ne saute d'une image à l'autre.
	for(int32 Index = 0; Index < 180; Index++){
		const int32 XEntier = (Index * 137 + 53) % FMath::Max(1, static_cast<int32>(Largeur));
		const int32 YEntier = (Index * 83 + 29) % FMath::Max(1, static_cast<int32>(Hauteur));
		const float Taille = Index % 13 == 0 ? 3.0f : (Index % 5 == 0 ? 1.9f : 1.1f);
		const float Intensite = Index % 9 == 0 ? 0.85f : (Index % 4 == 0 ? 0.46f : 0.25f);
		const FLinearColor Couleur = Index % 7 == 0
			? FLinearColor(0.85f, 0.92f, 1.0f, Intensite)
			: (Index % 11 == 0 ? FLinearColor(0.55f, 0.78f, 1.0f, Intensite) : FLinearColor(0.42f, 0.68f, 1.0f, Intensite));
		DrawRect(Couleur, static_cast<float>(XEntier), static_cast<float>(YEntier), Taille, Taille);
	}
}

// Dessine le menu principal avec le nom et les boutons.
void ASpaceShooterHUD::DessinerMenu(float Largeur, float Hauteur){
	ASpaceShipPawn* Vaisseau = Cast<ASpaceShipPawn>(PlayerOwner -> GetPawn());
	const float LargeurPanneau = 650.0f;
	const float HauteurPanneau = 530.0f;
	const float XPanneau = Largeur * 0.5f - LargeurPanneau * 0.5f;
	const float YPanneau = Hauteur * 0.5f - HauteurPanneau * 0.5f;
	DrawRect(FLinearColor(0.012f, 0.028f, 0.060f, 0.94f), XPanneau, YPanneau, LargeurPanneau, HauteurPanneau);
	DrawRect(FLinearColor(0.03f, 0.72f, 1.0f, 0.95f), XPanneau, YPanneau, LargeurPanneau, 4.0f);
	const FString Titre = TEXT("SPACE SHOOTER");
	const FString Membres = FString::Printf(TEXT("Jeremie Bouchard"));
	float LargeurTexte = 0.0f;
	float HauteurTexte = 0.0f;
	GetTextSize(Titre, LargeurTexte, HauteurTexte, GEngine -> GetLargeFont(), 2.0f);
	DrawText(
		Titre,
		FLinearColor(0.85f, 0.96f, 1.0f),
		Largeur * 0.5f - LargeurTexte * 0.5f,
		YPanneau + 58.0f,
		GEngine -> GetLargeFont(),
		2.0f,
		false
	);
	GetTextSize(Membres, LargeurTexte, HauteurTexte, GEngine -> GetSmallFont(), 0.95f);
	DrawText(
		Membres,
		FLinearColor(0.38f, 0.78f, 1.0f),
		Largeur * 0.5f - LargeurTexte * 0.5f,
		YPanneau + 155.0f,
		GEngine -> GetSmallFont(),
		0.95f,
		false
	);
	const float LargeurBouton = 320.0f;
	const float HauteurBouton = 62.0f;
	const float X = Largeur * 0.5f - LargeurBouton * 0.5f;
	const float YJouer = Hauteur * 0.52f;
	DessinerBouton(TEXT("JOUER"), X, YJouer, LargeurBouton, HauteurBouton);
	DessinerBouton(TEXT("COMMANDES"), X, YJouer + 78.0f, LargeurBouton, HauteurBouton);
	DessinerBouton(TEXT("QUITTER"), X, YJouer + 156.0f, LargeurBouton, HauteurBouton);
	DrawText(
		TEXT("SYSTEME PRET"),
		FLinearColor(0.18f, 1.0f, 0.42f),
		XPanneau + 26.0f,
		YPanneau + HauteurPanneau - 36.0f,
		GEngine -> GetSmallFont(),
		0.72f,
		false
	);
}

// Affiche la page séparée expliquant les contrôles.
void ASpaceShooterHUD::DessinerCommandes(float Largeur, float Hauteur){
	const float LargeurPanneau = 700.0f;
	const float HauteurPanneau = 560.0f;
	const float X = Largeur * 0.5f - LargeurPanneau * 0.5f;
	const float Y = Hauteur * 0.5f - HauteurPanneau * 0.5f;
	DrawRect(FLinearColor(0.012f, 0.028f, 0.060f, 0.96f), X, Y, LargeurPanneau, HauteurPanneau);
	DrawRect(FLinearColor(0.03f, 0.72f, 1.0f, 0.95f), X, Y, LargeurPanneau, 4.0f);
	DrawText(TEXT("COMMANDES"), FLinearColor::White, X + 225.0f, Y + 48.0f, GEngine -> GetLargeFont(), 1.55f, false);
	const float TexteX = X + 105.0f;
	float TexteY = Y + 145.0f;
	DrawText(TEXT("W     Acceleration avant"), FLinearColor(0.72f, 0.88f, 1.0f), TexteX, TexteY, GEngine -> GetSmallFont(), 1.0f, false);
	TexteY += 48.0f;
	DrawText(TEXT("S     Freinage / marche arriere"), FLinearColor(0.72f, 0.88f, 1.0f), TexteX, TexteY, GEngine -> GetSmallFont(), 1.0f, false);
	TexteY += 48.0f;
	DrawText(TEXT("A / D     Rotation du vaisseau"), FLinearColor(0.72f, 0.88f, 1.0f), TexteX, TexteY, GEngine -> GetSmallFont(), 1.0f, false);
	TexteY += 48.0f;
	DrawText(TEXT("Souris     Orientation de la tourelle"), FLinearColor(0.72f, 0.88f, 1.0f), TexteX, TexteY, GEngine -> GetSmallFont(), 1.0f, false);
	TexteY += 48.0f;
	DrawText(TEXT("Clic gauche     Tir laser"), FLinearColor(0.72f, 0.88f, 1.0f), TexteX, TexteY, GEngine -> GetSmallFont(), 1.0f, false);
	TexteY += 48.0f;
	DrawText(TEXT("Espace     Dash vers le curseur"), FLinearColor(0.72f, 0.88f, 1.0f), TexteX, TexteY, GEngine -> GetSmallFont(), 1.0f, false);
	const float LargeurBouton = 320.0f;
	const float HauteurBouton = 62.0f;
	const float XBouton = Largeur * 0.5f - LargeurBouton * 0.5f;
	const float YBouton = Hauteur * 0.78f;
	DessinerBouton(TEXT("RETOUR"), XBouton, YBouton, LargeurBouton, HauteurBouton);
}

// Affiche le score sans panneau, les coeurs en bas à gauche et les charges de dash sous forme de barres.
void ASpaceShooterHUD::DessinerInterfaceJeu(float Largeur, float Hauteur){
	// Récupère les valeurs actuelles directement sur le Pawn.
	ASpaceShipPawn* Vaisseau = Cast<ASpaceShipPawn>(PlayerOwner -> GetPawn());
	const FString Score = FString::Printf(TEXT("SCORE  %d"), Vaisseau -> ObtenirScore());
	const FString Vague = FString::Printf(TEXT("VAGUE  %d"), Vaisseau -> ObtenirVague());
	// Le score flotte directement dans le coin supérieur gauche sans rectangle opaque autour.
	DrawText(Score, FLinearColor(0.86f, 0.95f, 1.0f), 24.0f, 22.0f, GEngine -> GetSmallFont(), 1.22f, false);
	// Le numéro de vague reste seul dans le coin supérieur droit et n'utilise plus de zéro inutile.
	float LargeurVague = 0.0f;
	float HauteurVague = 0.0f;
	GetTextSize(Vague, LargeurVague, HauteurVague, GEngine -> GetSmallFont(), 1.05f);
	DrawText(Vague, FLinearColor(0.28f, 0.80f, 1.0f), Largeur - LargeurVague - 26.0f, 24.0f, GEngine -> GetSmallFont(), 1.05f, false);
	// Chaque coeur est dessiné séparément et peut perdre visuellement un quart sans afficher de fraction.
	const int32 QuartiersVie = Vaisseau -> ObtenirQuartiersVie();
	const int32 CoeursMax = FMath::Max(1, Vaisseau -> ObtenirQuartiersVieMax() / 4);
	const float TailleCoeur = 64.0f;
	const float EspaceCoeur = 8.0f;
	const float PositionCoeursX = 22.0f;
	const float PositionCoeursY = Hauteur - TailleCoeur - 22.0f;
	for(int32 Index = 0; Index < CoeursMax; Index++){
		const int32 QuartiersDuCoeur = FMath::Clamp(QuartiersVie - Index * 4, 0, 4);
		DessinerCoeur(PositionCoeursX + Index * (TailleCoeur + EspaceCoeur), PositionCoeursY, TailleCoeur, QuartiersDuCoeur);
	}
	// Les barres de dash commencent juste à droite de la rangée de coeurs.
	const float PositionDashX = PositionCoeursX + CoeursMax * (TailleCoeur + EspaceCoeur) + 22.0f;
	const float PositionDashY = PositionCoeursY + 5.0f;
	DessinerBarresDash(PositionDashX, PositionDashY, Vaisseau -> ObtenirChargesDash(), Vaisseau -> ObtenirChargesDashMax(), Vaisseau -> ObtenirRechargeDashRestante(), Vaisseau -> ObtenirTempsRechargeDash());
}

// Affiche trois cartes d'amélioration entre deux vagues.
void ASpaceShooterHUD::DessinerChoixBonus(float Largeur, float Hauteur){
	ASpaceShipPawn* Vaisseau = Cast<ASpaceShipPawn>(PlayerOwner -> GetPawn());
	DrawRect(FLinearColor(0.002f, 0.006f, 0.018f, 0.84f), 0.0f, 0.0f, Largeur, Hauteur);
	const FString Titre = TEXT("AMELIORATION DISPONIBLE");
	float LargeurTexte = 0.0f;
	float HauteurTexte = 0.0f;
	GetTextSize(Titre, LargeurTexte, HauteurTexte, GEngine -> GetLargeFont(), 1.55f);
	DrawText(
		Titre,
		FLinearColor(0.28f, 0.88f, 1.0f),
		Largeur * 0.5f - LargeurTexte * 0.5f,
		Hauteur * 0.20f,
		GEngine -> GetLargeFont(),
		1.55f,
		false
	);
	float SourisX = -1.0f;
	float SourisY = -1.0f;
	PlayerOwner -> GetMousePosition(SourisX, SourisY);
	const FVector2D Souris(SourisX, SourisY);
	const float LargeurCarte = 300.0f;
	const float HauteurCarte = 230.0f;
	const float Espace = 26.0f;
	const float LargeurTotale = LargeurCarte * 3.0f + Espace * 2.0f;
	const float XDepart = Largeur * 0.5f - LargeurTotale * 0.5f;
	const float Y = Hauteur * 0.5f - HauteurCarte * 0.5f;
	for(int32 Index = 0; Index < 3; Index++){
		const float XCarte = XDepart + Index * (LargeurCarte + Espace);
		const bool bSurvole = PointDansRectangleHUD(Souris, XCarte, Y, LargeurCarte, HauteurCarte);
		DessinerCarteBonus(
			Vaisseau -> ObtenirNomBonus(Index),
			Vaisseau -> ObtenirDescriptionBonus(Index),
			XCarte,
			Y,
			LargeurCarte,
			HauteurCarte,
			bSurvole
		);
	}
}

// Affiche un écran de fin compact avec le score, la vague atteinte et deux boutons entièrement contenus dans le panneau.
void ASpaceShooterHUD::DessinerFinPartie(float Largeur, float Hauteur){
	ASpaceShipPawn* Vaisseau = Cast<ASpaceShipPawn>(PlayerOwner -> GetPawn());
	// Assombrit légèrement la scène sans la remplacer par un grand aplat opaque.
	DrawRect(FLinearColor(0.002f, 0.006f, 0.014f, 0.72f), 0.0f, 0.0f, Largeur, Hauteur);
	const float LargeurPanneau = 560.0f;
	const float HauteurPanneau = 440.0f;
	const float X = Largeur * 0.5f - LargeurPanneau * 0.5f;
	const float Y = Hauteur * 0.5f - HauteurPanneau * 0.5f;
	// Panneau sombre bleuté avec une fine ligne rouge rappelant la mort du joueur.
	DrawRect(FLinearColor(0.020f, 0.032f, 0.060f, 0.96f), X, Y, LargeurPanneau, HauteurPanneau);
	DrawRect(FLinearColor(0.95f, 0.18f, 0.28f, 0.96f), X, Y, LargeurPanneau, 4.0f);
	const FString Titre = TEXT("FIN DE PARTIE");
	const FString ScoreFinal = FString::Printf(TEXT("SCORE FINAL  %d"), Vaisseau -> ObtenirScore());
	const FString VagueFinale = FString::Printf(TEXT("VAGUE ATTEINTE  %d"), Vaisseau -> ObtenirVague());
	float LargeurTexte = 0.0f;
	float HauteurTexte = 0.0f;
	// Centre le titre et les informations principales.
	GetTextSize(Titre, LargeurTexte, HauteurTexte, GEngine -> GetLargeFont(), 1.65f);
	DrawText(Titre, FLinearColor(1.0f, 0.30f, 0.36f), Largeur * 0.5f - LargeurTexte * 0.5f, Y + 48.0f, GEngine -> GetLargeFont(), 1.65f, false);
	DrawRect(FLinearColor(0.16f, 0.56f, 0.90f, 0.26f), X + 44.0f, Y + 112.0f, LargeurPanneau - 88.0f, 2.0f);
	GetTextSize(ScoreFinal, LargeurTexte, HauteurTexte, GEngine -> GetSmallFont(), 1.10f);
	DrawText(ScoreFinal, FLinearColor(0.92f, 0.96f, 1.0f), Largeur * 0.5f - LargeurTexte * 0.5f, Y + 148.0f, GEngine -> GetSmallFont(), 1.10f, false);
	GetTextSize(VagueFinale, LargeurTexte, HauteurTexte, GEngine -> GetSmallFont(), 0.92f);
	DrawText(VagueFinale, FLinearColor(0.48f, 0.78f, 1.0f), Largeur * 0.5f - LargeurTexte * 0.5f, Y + 188.0f, GEngine -> GetSmallFont(), 0.92f, false);
	// Les deux boutons restent à l'intérieur du panneau sur toutes les résolutions usuelles.
	const float LargeurBouton = 300.0f;
	const float HauteurBouton = 58.0f;
	const float XBouton = Largeur * 0.5f - LargeurBouton * 0.5f;
	const float YRejouer = Y + 260.0f;
	const float YMenu = YRejouer + 76.0f;
	DessinerBouton(TEXT("REJOUER"), XBouton, YRejouer, LargeurBouton, HauteurBouton);
	DessinerBouton(TEXT("RETOUR AU MENU"), XBouton, YMenu, LargeurBouton, HauteurBouton);
}

// Dessine un bouton avec un état de survol.
void ASpaceShooterHUD::DessinerBouton(const FString& Texte, float X, float Y, float Largeur, float Hauteur){
	float SourisX = -1.0f;
	float SourisY = -1.0f;
	if(PlayerOwner){
		PlayerOwner -> GetMousePosition(SourisX, SourisY);
	}
	const bool bSurvole = PointDansRectangleHUD(FVector2D(SourisX, SourisY), X, Y, Largeur, Hauteur);
	const FLinearColor Fond = bSurvole
		? FLinearColor(0.06f, 0.40f, 0.68f, 0.98f)
		: FLinearColor(0.025f, 0.085f, 0.16f, 0.98f);
	const FLinearColor Accent = bSurvole
		? FLinearColor(0.28f, 0.92f, 1.0f, 1.0f)
		: FLinearColor(0.04f, 0.62f, 0.95f, 0.92f);
	DrawRect(Fond, X, Y, Largeur, Hauteur);
	DrawRect(Accent, X, Y, 4.0f, Hauteur);
	DrawRect(Accent, X, Y, Largeur, 2.0f);
	float LargeurTexte = 0.0f;
	float HauteurTexte = 0.0f;
	GetTextSize(Texte, LargeurTexte, HauteurTexte, GEngine -> GetSmallFont(), 1.02f);
	DrawText(
		Texte,
		bSurvole ? FLinearColor::White : FLinearColor(0.82f, 0.90f, 1.0f),
		X + Largeur * 0.5f - LargeurTexte * 0.5f,
		Y + Hauteur * 0.5f - HauteurTexte * 0.5f,
		GEngine -> GetSmallFont(),
		1.02f,
		false
	);
}

// Dessine une carte de bonus et sa description.
void ASpaceShooterHUD::DessinerCarteBonus(const FString& Titre, const FString& Description, float X, float Y, float Largeur, float Hauteur, bool bSurvole){
	const FLinearColor Fond = bSurvole
		? FLinearColor(0.045f, 0.20f, 0.31f, 0.98f)
		: FLinearColor(0.018f, 0.055f, 0.095f, 0.98f);
	const FLinearColor Accent = bSurvole
		? FLinearColor(0.28f, 0.94f, 1.0f, 1.0f)
		: FLinearColor(0.06f, 0.55f, 0.86f, 0.92f);
	DrawRect(Fond, X, Y, Largeur, Hauteur);
	DrawRect(Accent, X, Y, Largeur, 4.0f);
	DrawText(
		Titre,
		FLinearColor::White,
		X + 22.0f,
		Y + 34.0f,
		GEngine -> GetSmallFont(),
		1.0f,
		false
	);
	DrawText(
		Description,
		FLinearColor(0.68f, 0.78f, 0.90f),
		X + 22.0f,
		Y + 104.0f,
		GEngine -> GetSmallFont(),
		0.72f,
		false
	);
	DrawText(
		TEXT("CLIQUER POUR CHOISIR"),
		Accent,
		X + 22.0f,
		Y + Hauteur - 42.0f,
		GEngine -> GetSmallFont(),
		0.68f,
		false
	);
}

// Dessine un vrai coeur lisse et retire exactement un quart de son remplissage à chaque petit dégât.
void ASpaceShooterHUD::DessinerCoeur(float X, float Y, float Taille, int32 QuartiersRemplis){
	// Sans texture disponible, l'interface peut simplement ignorer cette icône au lieu de planter.
	if(!TextureCoeur){
		return;
	}
	const float FractionVie = FMath::Clamp(static_cast<float>(QuartiersRemplis) / 4.0f, 0.0f, 1.0f);
	// Dessine d'abord la silhouette complète presque transparente pour montrer le coeur maximal.
	DrawTexture(TextureCoeur, X, Y, Taille, Taille, 0.0f, 0.0f, 1.0f, 1.0f, FLinearColor(0.28f, 0.015f, 0.035f, 0.28f), BLEND_Translucent, 1.0f, false);
	// Recouvre seulement la fraction encore vivante en rouge vif; le quart perdu devient donc réellement transparent/sombre.
	if(FractionVie > 0.0f){
		DrawTexture(TextureCoeur, X, Y, Taille * FractionVie, Taille, 0.0f, 0.0f, FractionVie, 1.0f, FLinearColor(1.0f, 0.045f, 0.085f, 1.0f), BLEND_Translucent, 1.0f, false);
	}
}

// Dessine chaque charge de dash comme une barre individuelle dont la progression devient verte lorsqu'elle est prête.
void ASpaceShooterHUD::DessinerBarresDash(float X, float Y, int32 Charges, int32 ChargesMax, float RechargeRestante, float TempsRecharge){
	// Le libellé reste discret et aucune zone opaque n'entoure les barres.
	DrawText(TEXT("DASH"), FLinearColor(0.54f, 0.82f, 1.0f), X, Y - 2.0f, GEngine -> GetSmallFont(), 0.92f, false);
	const float BarreY = Y + 30.0f;
	const float LargeurBarre = 92.0f;
	const float HauteurBarre = 18.0f;
	const float EspaceBarre = 12.0f;
	const float ProgressionRecharge = TempsRecharge > 0.0f ? FMath::Clamp(1.0f - RechargeRestante / TempsRecharge, 0.0f, 1.0f) : 1.0f;
	for(int32 Index = 0; Index < ChargesMax; Index++){
		const float XCourant = X + Index * (LargeurBarre + EspaceBarre);
		// Un fin contour bleu montre les charges possibles sans créer de gros rectangle de fond.
		DrawRect(FLinearColor(0.10f, 0.42f, 0.68f, 0.52f), XCourant, BarreY, LargeurBarre, 2.0f);
		DrawRect(FLinearColor(0.10f, 0.42f, 0.68f, 0.52f), XCourant, BarreY + HauteurBarre - 2.0f, LargeurBarre, 2.0f);
		DrawRect(FLinearColor(0.10f, 0.42f, 0.68f, 0.52f), XCourant, BarreY, 2.0f, HauteurBarre);
		DrawRect(FLinearColor(0.10f, 0.42f, 0.68f, 0.52f), XCourant + LargeurBarre - 2.0f, BarreY, 2.0f, HauteurBarre);
		// Une charge complète devient verte et indique immédiatement qu'elle peut être utilisée.
		if(Index < Charges){
			DrawRect(FLinearColor(0.10f, 1.0f, 0.34f, 0.95f), XCourant + 4.0f, BarreY + 4.0f, LargeurBarre - 8.0f, HauteurBarre - 8.0f);
		}
		// La prochaine charge disponible progresse visuellement en bleu pendant son temps de recharge.
		else if(Index == Charges && Charges < ChargesMax){
			DrawRect(FLinearColor(0.04f, 0.56f, 1.0f, 0.95f), XCourant + 4.0f, BarreY + 4.0f, (LargeurBarre - 8.0f) * ProgressionRecharge, HauteurBarre - 8.0f);
		}
	}
}
