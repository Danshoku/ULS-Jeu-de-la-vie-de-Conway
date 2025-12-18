#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include <raymath.h>

// =========================================================
// --- CONSTANTES & CONFIGURATION ---
// =========================================================
#define MAX_JOUEURS 8
#define TAILLE_HISTORIQUE 200
#define LIMITE_TEMPS_DUEL 30.0f
#define VITESSE_RETRECISSEMENT_ZONE 2.5f

// Noms des modèles pour le mode Satisfaisant
const char* NOMS_SATISFAISANTS[] = { "FLEUR", "GALAXIE", "FRACTALE", "ONDES", "CRISTAL" };
#define COMPTE_SATISFAISANT 5

// =========================================================
// --- DEFINITIONS DES MOTIFS (PATTERNS) ---
// =========================================================
typedef struct { int l, h; int *cellules; } Motif;

// 1. GLIDER
int donnees_glider_se[] = { 0,1,0, 0,0,1, 1,1,1 };
Motif MOTIF_GLIDER_SE = {3, 3, donnees_glider_se};

// 2. CLOWN
int donnees_clown[] = { 1,1,1, 1,0,1, 1,0,1 };
Motif MOTIF_CLOWN = {3, 3, donnees_clown};

// 3. CANON DE GOSPER
int donnees_gosper[] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
    0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,0,0,0,0,0,0,0,0,1,0,0,0,1,0,1,1,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
Motif MOTIF_GOSPER = {36, 9, donnees_gosper};

// Fonction pour appliquer un motif avec gestion de la rotation
void appliquer_motif(int *grille, int *grilleAge, int colonnes, int lignes, int x, int y, Motif m, int valJoueur, int rotation) {
    for (int py = 0; py < m.h; py++) {
        for (int px = 0; px < m.l; px++) {
            int dx, dy;
            switch (rotation) {
                case 0: dx = px; dy = py; break;
                case 1: dx = m.h - 1 - py; dy = px; break;
                case 2: dx = m.l - 1 - px; dy = m.h - 1 - py; break;
                case 3: dx = py; dy = m.l - 1 - px; break;
                default: dx = px; dy = py;
            }
            int cibleX = x + dx;
            int cibleY = y + dy;

            if (cibleX >= 0 && cibleX < colonnes && cibleY >= 0 && cibleY < lignes) {
                if (m.cellules[py * m.l + px] >= 1) {
                    int idx = cibleY * colonnes + cibleX;
                    grille[idx] = valJoueur;
                    grilleAge[idx] = 1;
                }
            }
        }
    }
}

// Pour setup procédural (sans rotation)
void appliquer_motif_setup(int *grille, int *grilleAge, int colonnes, int lignes, int x, int y, Motif m, int valJoueur) {
    appliquer_motif(grille, grilleAge, colonnes, lignes, x, y, m, valJoueur, 0);
}

// Pour setup miroir
void appliquer_motif_miroir(int *grille, int *grilleAge, int colonnes, int lignes, int x, int y, Motif m, int valJoueur) {
    for (int py = 0; py < m.h; py++) {
        for (int px = 0; px < m.l; px++) {
            int cibleX = x + (m.l - 1 - px);
            int cibleY = y + py;
            if (cibleX >= 0 && cibleX < colonnes && cibleY >= 0 && cibleY < lignes) {
                if (m.cellules[py * m.l + px] >= 1) {
                    int idx = cibleY * colonnes + cibleX;
                    grille[idx] = valJoueur;
                    grilleAge[idx] = 1;
                }
            }
        }
    }
}

// =========================================================
// --- STRUCTURES & ETATS ---
// =========================================================

typedef enum {
    MENU, JEU, REGLES,
    DUEL_CONFIG, DUEL_JEU, DUEL_FIN,
    BR_CONFIG, BR_JEU, BR_FIN,
    SATISFAISANT_CONFIG, SATISFAISANT_JEU
} EtatApplication;

typedef struct {
    int lignes; int colonnes;
    int *grille; int *prochaineGrille; int *grilleAge;
    int *historique[TAILLE_HISTORIQUE];
    int *historiqueAge[TAILLE_HISTORIQUE];
    int teteHistorique; int compteHistorique;
    long generation;
    bool estEnPause;
    float accumulateurTemps; float vitesseMiseAJour;
    char nomsJoueurs[MAX_JOUEURS + 1][20];
    int scores[MAX_JOUEURS + 1];
    int saisieActuelle;
    int compteJoueursBR;
    float minuteurCombat;
    float rayonZone;
    int typeSatisfaisant;
} JeuDeLaVie;

Color ObtenirCouleurJoueur(int id) {
    switch (id) {
        case 1: return BLUE;
        case 2: return RED;
        case 3: return GREEN;
        case 4: return ORANGE;
        case 5: return PURPLE;
        case 6: return YELLOW;
        case 7: return PINK;
        case 8: return LIME;
        default: return DARKGRAY;
    }
}

JeuDeLaVie* initialiser_jeu(int colonnes, int lignes) {
    JeuDeLaVie *jeu = (JeuDeLaVie*)malloc(sizeof(JeuDeLaVie));
    jeu->colonnes = colonnes; jeu->lignes = lignes;
    int taille = colonnes * lignes;
    jeu->grille = (int*)calloc(taille, sizeof(int));
    jeu->prochaineGrille = (int*)calloc(taille, sizeof(int));
    jeu->grilleAge = (int*)calloc(taille, sizeof(int));
    for(int i=0; i<TAILLE_HISTORIQUE; i++) {
        jeu->historique[i] = (int*)calloc(taille, sizeof(int));
        jeu->historiqueAge[i] = (int*)calloc(taille, sizeof(int));
    }
    jeu->teteHistorique = 0; jeu->compteHistorique = 0;
    jeu->generation = 0;
    jeu->estEnPause = true;
    jeu->accumulateurTemps = 0.0f;
    jeu->vitesseMiseAJour = 0.1f;

    // --- INITIALISATION DES NOMS DES JOUEURS ---
    strcpy(jeu->nomsJoueurs[1], "Joueur 1");
    strcpy(jeu->nomsJoueurs[2], "Joueur 2");
    strcpy(jeu->nomsJoueurs[3], "Joueur 3");
    strcpy(jeu->nomsJoueurs[4], "Joueur 4");
    strcpy(jeu->nomsJoueurs[5], "Joueur 5");
    strcpy(jeu->nomsJoueurs[6], "Joueur 6");
    strcpy(jeu->nomsJoueurs[7], "Joueur 7");
    strcpy(jeu->nomsJoueurs[8], "Joueur 8");
    // -------------------------------------------

    jeu->saisieActuelle = 1;
    jeu->compteJoueursBR = 4;
    jeu->minuteurCombat = 0.0f;
    jeu->rayonZone = (float)colonnes;
    jeu->typeSatisfaisant = 0;
    return jeu;
}

void sauvegarder_etat(JeuDeLaVie *jeu) {
    int taille = jeu->colonnes * jeu->lignes;
    jeu->teteHistorique = (jeu->teteHistorique + 1) % TAILLE_HISTORIQUE;
    memcpy(jeu->historique[jeu->teteHistorique], jeu->grille, taille * sizeof(int));
    memcpy(jeu->historiqueAge[jeu->teteHistorique], jeu->grilleAge, taille * sizeof(int));
    if (jeu->compteHistorique < TAILLE_HISTORIQUE) jeu->compteHistorique++;
}

void annuler_etat(JeuDeLaVie *jeu) {
    if (jeu->compteHistorique > 0) {
        int taille = jeu->colonnes * jeu->lignes;
        memcpy(jeu->grille, jeu->historique[jeu->teteHistorique], taille * sizeof(int));
        memcpy(jeu->grilleAge, jeu->historiqueAge[jeu->teteHistorique], taille * sizeof(int));
        jeu->teteHistorique--;
        if (jeu->teteHistorique < 0) jeu->teteHistorique = TAILLE_HISTORIQUE - 1;
        jeu->compteHistorique--;
        if (jeu->generation > 0) jeu->generation--;
    }
}

void vider_grille(JeuDeLaVie *jeu) {
    int taille = jeu->colonnes * jeu->lignes;
    memset(jeu->grille, 0, taille * sizeof(int));
    memset(jeu->grilleAge, 0, taille * sizeof(int));
    jeu->compteHistorique = 0; jeu->generation = 0;
}

// =========================================================
// --- MISE A JOUR ---
// =========================================================

void mise_a_jour_classique(JeuDeLaVie *jeu) {
    sauvegarder_etat(jeu);
    jeu->generation++;
    int decalages[8] = {-1-jeu->colonnes, -jeu->colonnes, 1-jeu->colonnes, -1, 1, -1+jeu->colonnes, jeu->colonnes, 1+jeu->colonnes};
    int valCentre = jeu->grille[jeu->colonnes/2 + (jeu->lignes/2)*jeu->colonnes];
    int couleurApparition = (valCentre == 5) ? 5 : 1;
    if (jeu->generation < 5 && valCentre == 0) couleurApparition = 5;

    for (int y = 0; y < jeu->lignes; y++) {
        for (int x = 0; x < jeu->colonnes; x++) {
            int idx = y * jeu->colonnes + x;
            int compte = 0;
            if (x > 0 && x < jeu->colonnes - 1 && y > 0 && y < jeu->lignes - 1) {
                for (int k = 0; k < 8; k++) if (jeu->grille[idx + decalages[k]] > 0) compte++;
            }
            int maVal = jeu->grille[idx];
            if (maVal > 0) {
                if (compte == 2 || compte == 3) { jeu->prochaineGrille[idx] = maVal; jeu->grilleAge[idx]++; }
                else { jeu->prochaineGrille[idx] = 0; jeu->grilleAge[idx] = 0; }
            } else {
                if (compte == 3) {
                    jeu->prochaineGrille[idx] = (couleurApparition == 5) ? 5 : 1;
                    jeu->grilleAge[idx] = 1;
                }
                else { jeu->prochaineGrille[idx] = 0; jeu->grilleAge[idx] = 0; }
            }
        }
    }
    int *temp = jeu->grille; jeu->grille = jeu->prochaineGrille; jeu->prochaineGrille = temp;
}

void mise_a_jour_duel(JeuDeLaVie *jeu) {
    sauvegarder_etat(jeu);
    jeu->generation++;
    int decalages[8] = {-1-jeu->colonnes, -jeu->colonnes, 1-jeu->colonnes, -1, 1, -1+jeu->colonnes, jeu->colonnes, 1+jeu->colonnes};
    int taille = jeu->lignes * jeu->colonnes;
    memset(jeu->prochaineGrille, 0, taille * sizeof(int));
    jeu->scores[1] = 0; jeu->scores[2] = 0;

    for (int y = 0; y < jeu->lignes; y++) {
        for (int x = 0; x < jeu->colonnes; x++) {
            int idx = y * jeu->colonnes + x;
            int compte1 = 0; int compte2 = 0;
            if (x > 0 && x < jeu->colonnes - 1 && y > 0 && y < jeu->lignes - 1) {
                for (int k = 0; k < 8; k++) {
                    int nVal = jeu->grille[idx + decalages[k]];
                    if (nVal == 1) compte1++; else if (nVal == 2) compte2++;
                }
            }
            int total = compte1 + compte2;
            if (jeu->grille[idx] > 0) {
                if (total == 2 || total == 3) jeu->prochaineGrille[idx] = jeu->grille[idx];
                else jeu->prochaineGrille[idx] = 0;
            } else {
                if (total == 3) {
                    if (compte1 > compte2) jeu->prochaineGrille[idx] = 1; else jeu->prochaineGrille[idx] = 2;
                } else jeu->prochaineGrille[idx] = 0;
            }
            if (jeu->prochaineGrille[idx] == 1) jeu->scores[1]++;
            if (jeu->prochaineGrille[idx] == 2) jeu->scores[2]++;
        }
    }
    int *temp = jeu->grille; jeu->grille = jeu->prochaineGrille; jeu->prochaineGrille = temp;
}

int mise_a_jour_br(JeuDeLaVie *jeu, int maxJoueurs, bool utiliserZone) {
    sauvegarder_etat(jeu);
    jeu->generation++;
    int decalages[8] = {-1-jeu->colonnes, -jeu->colonnes, 1-jeu->colonnes, -1, 1, -1+jeu->colonnes, jeu->colonnes, 1+jeu->colonnes};
    float cx = jeu->colonnes / 2.0f; float cy = jeu->lignes / 2.0f;
    float rayonCarre = jeu->rayonZone * jeu->rayonZone;
    int taille = jeu->lignes * jeu->colonnes;
    memset(jeu->prochaineGrille, 0, taille * sizeof(int));
    for(int i=0; i<=MAX_JOUEURS; i++) jeu->scores[i] = 0;

    for (int y = 0; y < jeu->lignes; y++) {
        for (int x = 0; x < jeu->colonnes; x++) {
            int idx = y * jeu->colonnes + x;
            float dx = x - cx; float dy = y - cy;
            float distCarre = dx*dx + dy*dy;

            if (utiliserZone && distCarre > rayonCarre && jeu->grille[idx] > 0) {
                int moveX = (x > cx) ? -1 : (x < cx ? 1 : 0);
                int moveY = (y > cy) ? -1 : (y < cy ? 1 : 0);
                int cibleX = x + moveX; int cibleY = y + moveY;
                if (cibleX >= 0 && cibleX < jeu->colonnes && cibleY >= 0 && cibleY < jeu->lignes) {
                    int idxCible = cibleY * jeu->colonnes + cibleX;
                    if (jeu->prochaineGrille[idxCible] == 0) jeu->prochaineGrille[idxCible] = jeu->grille[idx];
                }
                continue;
            }
            int comptes[MAX_JOUEURS + 1]; for(int k=0; k<=MAX_JOUEURS; k++) comptes[k] = 0;
            if (x > 0 && x < jeu->colonnes - 1 && y > 0 && y < jeu->lignes - 1) {
                for (int k = 0; k < 8; k++) {
                    int val = jeu->grille[idx + decalages[k]];
                    if (val > 0 && val <= maxJoueurs) comptes[val]++;
                }
            }
            int total = 0; for(int p=1; p<=maxJoueurs; p++) total += comptes[p];

            if (jeu->grille[idx] > 0) {
                if (total == 2 || total == 3) { if (jeu->prochaineGrille[idx] == 0) jeu->prochaineGrille[idx] = jeu->grille[idx]; }
            } else {
                if (total == 3) {
                    int maxC = 0; int vainqueur = 0;
                    for(int p=1; p<=maxJoueurs; p++) {
                        if (comptes[p] > maxC) { maxC = comptes[p]; vainqueur = p; }
                        else if (comptes[p] == maxC && maxC > 0) { if (rand()%2 == 0) vainqueur = p; }
                    }
                    if (jeu->prochaineGrille[idx] == 0) jeu->prochaineGrille[idx] = vainqueur;
                }
            }
            if (jeu->prochaineGrille[idx] > 0) jeu->scores[jeu->prochaineGrille[idx]]++;
        }
    }
    int *temp = jeu->grille; jeu->grille = jeu->prochaineGrille; jeu->prochaineGrille = temp;
    int survivants = 0; for(int i=1; i<=maxJoueurs; i++) { if(jeu->scores[i] > 0) survivants++; }
    return survivants;
}

// =========================================================
// --- CONFIGURATION ---
// =========================================================

void configurer_duel_aleatoire(JeuDeLaVie *jeu) {
    vider_grille(jeu);
    int total = jeu->colonnes * jeu->lignes;
    for (int i = 0; i < total; i++) {
        int r = rand() % 100;
        if (r < 10) jeu->grille[i] = 1;
        else if (r < 20) jeu->grille[i] = 2;
        else jeu->grille[i] = 0;
        if (jeu->grille[i] != 0) jeu->grilleAge[i] = 1;
    }
    jeu->minuteurCombat = 0.0f; jeu->generation = 0;
}

void configurer_satisfaisant(JeuDeLaVie *jeu) {
    vider_grille(jeu);
    int cx = jeu->colonnes / 2; int cy = jeu->lignes / 2;
    float rMax = (jeu->lignes < jeu->colonnes ? jeu->lignes : jeu->colonnes) * 0.45f;

    for (int y = 0; y < jeu->lignes; y++) {
        for (int x = 0; x < jeu->colonnes; x++) {
            float dx = (float)(x - cx); float dy = (float)(y - cy);
            float dist = sqrtf(dx*dx + dy*dy); float angle = atan2f(dy, dx);
            bool remplir = false;
            bool estCristal = (jeu->typeSatisfaisant == 4);

            switch (jeu->typeSatisfaisant) {
                case 0: { float f = fabs(cosf(6.0f * angle)); if (dist <= rMax * (0.3f + 0.7f * f)) remplir = true; break; }
                case 1: { float off = dist * 0.15f; if (dist <= rMax && cosf(3.0f * angle + off) > 0.2f) remplir = true; break; }
                case 2: { int fx = x - (cx - (int)rMax); int fy = y - (cy - (int)rMax); if (fx>0 && fy>0 && fx<rMax*2 && fy<rMax*2 && (fx&fy)==0) remplir = true; break; }
                case 3: { if (dist <= rMax && sinf(dist * 0.2f) > 0.0f) remplir = true; break; }
                case 4: { if ((fabs(dx)+fabs(dy) + fmaxf(fabs(dx),fabs(dy))) * 0.5f < rMax * 0.8f) remplir = true; break; }
            }
            if (remplir) {
                int idx = y * jeu->colonnes + x;
                if ((estCristal && rand()%100 < 65) || (!estCristal && (x+y)%2 == 0)) {
                    jeu->grille[idx] = 5; jeu->grilleAge[idx] = 1;
                }
            }
        }
    }
    jeu->generation = 0; jeu->estEnPause = true;
}

void configurer_battleroyale(JeuDeLaVie *jeu) {
    vider_grille(jeu);
    int l = jeu->colonnes; int h = jeu->lignes;
    for (int i = 0; i < l * h; i++) {
        int r = rand() % 100;
        int assigne = 0;
        for (int p = 1; p <= jeu->compteJoueursBR; p++) { if (r < p * 6) { jeu->grille[i] = p; assigne = 1; break; } }
        if (assigne) jeu->grilleAge[i] = 1;
    }
    float cx = l / 2.0f; float cy = h / 2.0f; float rayon = (l < h ? l : h) * 0.35f;
    for (int p = 1; p <= jeu->compteJoueursBR; p++) {
        float angle = (2.0f * PI * (p - 1)) / jeu->compteJoueursBR;
        int sx = (int)(cx + cosf(angle) * rayon) - 18;
        int sy = (int)(cy + sinf(angle) * rayon) - 4;
        if (sx < cx) appliquer_motif_setup(jeu->grille, jeu->grilleAge, l, h, sx, sy, MOTIF_GOSPER, p);
        else appliquer_motif_miroir(jeu->grille, jeu->grilleAge, l, h, sx, sy, MOTIF_GOSPER, p);
    }
    jeu->minuteurCombat = 0.0f; jeu->generation = 0; jeu->rayonZone = (float)l * 0.75f;
}

void remplissage_aleatoire_classique(JeuDeLaVie *jeu) {
    int taille = jeu->colonnes * jeu->lignes;
    for (int i = 0; i < taille; i++) {
        jeu->grille[i] = (rand() % 100 < 20) ? 1 : 0;
        jeu->grilleAge[i] = jeu->grille[i];
    }
}

Color obtenir_couleur_cellule(int val, int age, EtatApplication etat) {
    if (val == 0) return BLACK;
    if (etat == JEU || etat == SATISFAISANT_JEU) {
        if (val == 5) {
            float f = (float)age / 40.0f; if (f > 1.0f) f = 1.0f;
            return (Color){ 140+(unsigned char)(115*f), 40+(unsigned char)(100*f), 255, 255 };
        }
        float f = (float)age / 50.0f; if (f > 1.0f) f = 1.0f;
        return (Color){ 255-(unsigned char)(25*f), 255-(unsigned char)(214*f), 255-(unsigned char)(200*f), 255 };
    }
    return ObtenirCouleurJoueur(val);
}

void dessiner_bouton_accueil(EtatApplication *etat, JeuDeLaVie *jeu) {
    Rectangle btn = { 10.0f, 10.0f, 40.0f, 40.0f };
    DrawRectangleRec(btn, LIGHTGRAY);
    DrawRectangleLinesEx(btn, 2, DARKGRAY);
    DrawTriangle((Vector2){10.0f, 30.0f}, (Vector2){50.0f, 30.0f}, (Vector2){30.0f, 10.0f}, DARKGRAY);
    DrawRectangle(18, 30, 24, 18, DARKGRAY);
    DrawRectangle(26, 38, 8, 10, LIGHTGRAY);
    if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        *etat = MENU; jeu->estEnPause = true;
    }
}

// ---------------------------------------------------------
// MAIN
// ---------------------------------------------------------
int main(void) {
    srand(time(NULL));
    InitWindow(0, 0, "Jeu de la Vie - Ultimate");
    int moniteur = GetCurrentMonitor();
    int largeurEcran = GetMonitorWidth(moniteur);
    int hauteurEcran = GetMonitorHeight(moniteur);
    ToggleFullscreen();
    SetTargetFPS(60);

    int LARGEUR_GRILLE = 200; int HAUTEUR_GRILLE = 150; int TAILLE_CELLULE = 10;
    JeuDeLaVie *jeu = initialiser_jeu(LARGEUR_GRILLE, HAUTEUR_GRILLE);

    Camera2D camera = { 0 };
    camera.zoom = 0.8f;
    camera.offset = (Vector2){ (float)largeurEcran/2.0f, (float)hauteurEcran/2.0f };
    camera.target = (Vector2){ (float)(LARGEUR_GRILLE*TAILLE_CELLULE)/2.0f, (float)(HAUTEUR_GRILLE*TAILLE_CELLULE)/2.0f };

    Texture2D fondDuel = LoadTexture("image_2.png");
    Texture2D fondBR = LoadTexture("fortnite.png");

    EtatApplication etat = MENU;
    float defilementFond = 0.0f;
    int motifSelectionne = 0;
    int rotationMotif = 0;
    const char* nomOutil = "Crayon";

    while (!WindowShouldClose()) {
        if (IsWindowResized()) {
            largeurEcran = GetScreenWidth();
            hauteurEcran = GetScreenHeight();
            camera.offset = (Vector2){ (float)largeurEcran/2.0f, (float)hauteurEcran/2.0f };
        }
        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        if (etat == MENU) {
            defilementFond += 0.5f;
        }
        else if (etat == DUEL_CONFIG || etat == BR_CONFIG) {
            if (etat == BR_CONFIG) {
                 if (IsKeyPressed(KEY_LEFT)) { jeu->compteJoueursBR--; if (jeu->compteJoueursBR < 2) jeu->compteJoueursBR = 2; }
                 if (IsKeyPressed(KEY_RIGHT)) { jeu->compteJoueursBR++; if (jeu->compteJoueursBR > MAX_JOUEURS) jeu->compteJoueursBR = MAX_JOUEURS; }
            }
            int maxP = (etat == DUEL_CONFIG) ? 2 : jeu->compteJoueursBR;
            int touche = GetCharPressed();
            while (touche > 0) {
                if (touche >= 32 && touche <= 125) {
                    int len = strlen(jeu->nomsJoueurs[jeu->saisieActuelle]);
                    if (len < 19) { jeu->nomsJoueurs[jeu->saisieActuelle][len] = (char)touche; jeu->nomsJoueurs[jeu->saisieActuelle][len+1] = '\0'; }
                }
                touche = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                int len = strlen(jeu->nomsJoueurs[jeu->saisieActuelle]); if (len > 0) jeu->nomsJoueurs[jeu->saisieActuelle][len-1] = '\0';
            }
            if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ENTER)) {
                jeu->saisieActuelle++; if (jeu->saisieActuelle > maxP) jeu->saisieActuelle = 1;
            }
            if (jeu->saisieActuelle > maxP) jeu->saisieActuelle = maxP;
        }
        else if (etat == DUEL_JEU || etat == BR_JEU || etat == SATISFAISANT_JEU) {
            float molette = GetMouseWheelMove();
            if (molette != 0) {
                Vector2 pos = GetScreenToWorld2D(GetMousePosition(), camera);
                camera.offset = GetMousePosition(); camera.target = pos;
                camera.zoom += (molette * 0.125f); if (camera.zoom < 0.1f) camera.zoom = 0.1f;
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
                Vector2 delta = Vector2Scale(GetMouseDelta(), -1.0f/camera.zoom);
                camera.target = Vector2Add(camera.target, delta);
            }
             if (IsKeyPressed(KEY_SPACE)) jeu->estEnPause = !jeu->estEnPause;
             if (jeu->estEnPause) {
                 Rectangle btn = { (float)largeurEcran/2.0f - 150.0f, (float)hauteurEcran/2.0f - 30.0f, 300.0f, 60.0f };
                 if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) jeu->estEnPause = false;
             }
             if (!jeu->estEnPause) {
                 float dt = GetFrameTime();
                 jeu->minuteurCombat += dt;
                 if (etat == BR_JEU) {
                     jeu->rayonZone -= (VITESSE_RETRECISSEMENT_ZONE * dt);
                     if (jeu->rayonZone < 0) jeu->rayonZone = 0;
                 }
                 if (etat == DUEL_JEU && jeu->minuteurCombat >= LIMITE_TEMPS_DUEL) etat = DUEL_FIN;
                 float cible = (etat == SATISFAISANT_JEU) ? 0.08f : 0.05f;
                 jeu->accumulateurTemps += dt;
                 if (jeu->accumulateurTemps >= cible) {
                    if (etat == DUEL_JEU) mise_a_jour_duel(jeu);
                    else if (etat == SATISFAISANT_JEU) mise_a_jour_classique(jeu);
                    else {
                        int survivants = mise_a_jour_br(jeu, jeu->compteJoueursBR, (etat == BR_JEU));
                        if (etat == BR_JEU && survivants <= 1 && jeu->minuteurCombat > 2.0f) etat = BR_FIN;
                    }
                    jeu->accumulateurTemps = 0.0f;
                 }
             }
        }
        else if (etat == DUEL_FIN || etat == BR_FIN) {
             if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) etat = MENU;
        }
        else if (etat == JEU) {
            float molette = GetMouseWheelMove();
            if (molette != 0) {
                Vector2 pos = GetScreenToWorld2D(GetMousePosition(), camera);
                camera.offset = GetMousePosition(); camera.target = pos;
                camera.zoom += (molette * 0.125f); if (camera.zoom < 0.1f) camera.zoom = 0.1f;
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
                Vector2 delta = Vector2Scale(GetMouseDelta(), -1.0f/camera.zoom);
                camera.target = Vector2Add(camera.target, delta);
            }
            if (IsKeyPressed(KEY_SPACE)) jeu->estEnPause = !jeu->estEnPause;
            if (IsKeyPressed(KEY_ESCAPE)) etat = MENU;

            if (IsKeyPressed(KEY_C)) { motifSelectionne = 0; nomOutil = "Crayon"; }
            if (IsKeyPressed(KEY_I)) { motifSelectionne = 1; nomOutil = "Glider"; }
            if (IsKeyPressed(KEY_O)) { motifSelectionne = 2; nomOutil = "Clown"; }
            if (IsKeyPressed(KEY_P)) { motifSelectionne = 3; nomOutil = "Canon Gosper"; }
            if (IsKeyPressed(KEY_R)) { rotationMotif = (rotationMotif + 1) % 4; }

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && motifSelectionne == 0)) {
                Vector2 mw = GetScreenToWorld2D(GetMousePosition(), camera);
                int gx = mw.x/TAILLE_CELLULE; int gy = mw.y/TAILLE_CELLULE;
                if(gx>=0 && gx<jeu->colonnes && gy>=0 && gy<jeu->lignes) {
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                         if(motifSelectionne==1) appliquer_motif(jeu->grille, jeu->grilleAge, jeu->colonnes, jeu->lignes, gx, gy, MOTIF_GLIDER_SE, 1, rotationMotif);
                         else if(motifSelectionne==2) appliquer_motif(jeu->grille, jeu->grilleAge, jeu->colonnes, jeu->lignes, gx, gy, MOTIF_CLOWN, 1, rotationMotif);
                         else if(motifSelectionne==3) appliquer_motif(jeu->grille, jeu->grilleAge, jeu->colonnes, jeu->lignes, gx, gy, MOTIF_GOSPER, 1, rotationMotif);
                         else { int idx=gy*jeu->colonnes+gx; jeu->grille[idx]=!jeu->grille[idx]; jeu->grilleAge[idx]=jeu->grille[idx]?1:0; }
                    } else if(motifSelectionne==0) { int idx=gy*jeu->colonnes+gx; jeu->grille[idx]=1; jeu->grilleAge[idx]=1; }
                }
            }
            if (jeu->estEnPause && (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT))) annuler_etat(jeu);
            if (jeu->estEnPause && (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT))) mise_a_jour_classique(jeu);
            if (!jeu->estEnPause) {
                jeu->accumulateurTemps+=GetFrameTime();
                if(jeu->accumulateurTemps>=jeu->vitesseMiseAJour){ mise_a_jour_classique(jeu); jeu->accumulateurTemps=0.0f;}
            }
        }

        // --- DESSIN ---
        BeginDrawing();
        ClearBackground(GetColor(0x181818FF));

        if (etat == DUEL_JEU || etat == BR_JEU || etat == MENU) {
             for(int i=0; i<largeurEcran/40+1; i++) DrawLine(i*40+(int)defilementFond%40, 0, i*40+(int)defilementFond%40, hauteurEcran, Fade(WHITE, 0.05f));
        }

        if (etat == SATISFAISANT_CONFIG || etat == SATISFAISANT_JEU) {
            DrawRectangleGradientV(0, 0, largeurEcran, hauteurEcran, (Color){10, 10, 30, 255}, (Color){40, 10, 60, 255});
            float temps = (float)GetTime();
            for(int i = 0; i < 15; i++) {
                float decalage = (float)i * 123.0f;
                float vitesse = 0.3f + (float)(i % 3) * 0.1f;
                float x = (float)largeurEcran/2.0f + sinf(temps * vitesse + decalage) * (float)(largeurEcran * 0.4f);
                float y = (float)hauteurEcran/2.0f + cosf(temps * vitesse * 0.8f + decalage) * (float)(hauteurEcran * 0.4f);
                float rayon = 100.0f + sinf(temps + decalage) * 50.0f;
                float teinte = fmodf(temps * 10.0f + (float)i * 20.0f, 360.0f);
                Color c = ColorFromHSV(teinte, 0.6f, 0.8f); c.a = 30;
                DrawCircleGradient((int)x, (int)y, rayon, c, Fade(c, 0.0f));
            }
        }

        if (etat == MENU) {
            int cx = largeurEcran/2; int cy = hauteurEcran/2;

            // Fond stylé pour le menu
            DrawRectangleGradientV(0, 0, largeurEcran, hauteurEcran, (Color){15, 20, 30, 255}, (Color){50, 60, 80, 255});
            for(int i=0; i<largeurEcran/40+1; i++) {
                DrawLine(i*40+(int)defilementFond%40, 0, i*40+(int)defilementFond%40, hauteurEcran, Fade(SKYBLUE, 0.1f));
            }
            float time = (float)GetTime();
            for(int i=0; i<20; i++) {
                float x = fmodf(i * 100 + time * 50, largeurEcran);
                float y = fmodf(i * 200 + sinf(time + i)*50, hauteurEcran);
                DrawRectangle((int)x, (int)y, 4, 4, Fade(SKYBLUE, 0.5f + 0.5f*sinf(time*2 + i)));
            }

            // --- TITRE ---
            float pulse = sinf(time * 2.0f);
            int titleSize = 60 + (int)(pulse * 5.0f);
            DrawText("LE JEU DE LA VIE", cx - MeasureText("LE JEU DE LA VIE", titleSize)/2 + 4, cy - 250 + 4, titleSize, BLACK);
            DrawText("LE JEU DE LA VIE", cx - MeasureText("LE JEU DE LA VIE", titleSize)/2, cy - 250, titleSize, SKYBLUE);

            // --- CREDITS ANIMES (Bas de page) ---
            float credTime = (float)GetTime();
            float floatY = sinf(credTime * 2.0f) * 5.0f;
            int credY = hauteurEcran - 50 + (int)floatY;
            const char* txtCred = "Par Coelho Batiste et Nicole Corentin";
            int txtW = MeasureText(txtCred, 20);

            // Texte Crédits
            DrawText(txtCred, cx - txtW/2 + 2, credY + 2, 20, BLACK);
            DrawText(txtCred, cx - txtW/2, credY, 20, WHITE);
            // ------------------------------------

            Rectangle rD = { (float)cx - 175.0f, (float)cy - 140.0f, 350.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), rD) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                jeu->saisieActuelle = 1; etat = DUEL_CONFIG;
            }
            DrawRectangleRec(rD, CheckCollisionPointRec(GetMousePosition(), rD)?RED:DARKGRAY);
            const char* t = "MODE DUEL";
            DrawText(t, (int)(rD.x + (rD.width - MeasureText(t, 20))/2), (int)rD.y+15, 20, WHITE);

            Rectangle rB = { (float)cx - 175.0f, (float)cy - 80.0f, 350.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), rB) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                jeu->saisieActuelle = 1; etat = BR_CONFIG; jeu->compteJoueursBR = 4;
            }
            DrawRectangleRec(rB, CheckCollisionPointRec(GetMousePosition(), rB)?ORANGE:DARKGRAY);
            DrawText("BATTLE ROYALE", (int)rB.x+100, (int)rB.y+15, 20, WHITE);

            Rectangle rOP = { (float)cx - 175.0f, (float)cy - 20.0f, 350.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), rOP) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                etat = SATISFAISANT_CONFIG;
            }
            DrawRectangleRec(rOP, CheckCollisionPointRec(GetMousePosition(), rOP)?PURPLE:DARKGRAY);
            DrawText("MODE DESSIN", (int)rOP.x + 110, (int)rOP.y+15, 20, WHITE);

            Rectangle rRegles = { (float)cx - 175.0f, (float)cy + 40.0f, 350.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), rRegles) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                etat = REGLES;
            }
            DrawRectangleRec(rRegles, CheckCollisionPointRec(GetMousePosition(), rRegles)?SKYBLUE:DARKGRAY);
            DrawText("REGLES DU JEU", (int)rRegles.x + 105, (int)rRegles.y+15, 20, CheckCollisionPointRec(GetMousePosition(), rRegles)?BLACK:WHITE);

            Rectangle r1 = { (float)cx - 175.0f, (float)cy + 100.0f, 350.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), r1) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { vider_grille(jeu); etat = JEU; }
            DrawRectangleRec(r1, CheckCollisionPointRec(GetMousePosition(), r1)?RAYWHITE:DARKGRAY);
            DrawText("MODE SOLO (EDITEUR)", (int)r1.x+60, (int)r1.y+15, 20, CheckCollisionPointRec(GetMousePosition(), r1)?BLACK:WHITE);

            Rectangle r2 = { (float)cx - 175.0f, (float)cy + 160.0f, 350.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), r2) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { remplissage_aleatoire_classique(jeu); etat = JEU; }
            DrawRectangleRec(r2, CheckCollisionPointRec(GetMousePosition(), r2)?RAYWHITE:DARKGRAY);
            DrawText("MODE SOLO (ALEATOIRE)", (int)r2.x+50, (int)r2.y+15, 20, CheckCollisionPointRec(GetMousePosition(), r2)?BLACK:WHITE);

            Rectangle rQ = { (float)cx - 175.0f, (float)cy + 220.0f, 350.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), rQ) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) CloseWindow();
            DrawRectangleRec(rQ, CheckCollisionPointRec(GetMousePosition(), rQ)?RED:DARKGRAY);
            DrawText("QUITTER", (int)rQ.x+130, (int)rQ.y+15, 20, WHITE);
        }
        else if (etat == REGLES) {
            DrawRectangleGradientV(0, 0, largeurEcran, hauteurEcran, (Color){20, 30, 40, 255}, (Color){10, 15, 20, 255});
            float temps = (float)GetTime();
            for(int i = 0; i < 20; i++) {
                float vitesse = 0.05f + (float)(i % 5) * 0.01f;
                float decalage = (float)i * 50.0f;
                float x = (float)largeurEcran/2.0f + sinf(temps * vitesse + decalage) * (float)largeurEcran * 0.6f;
                float y = (float)hauteurEcran/2.0f + cosf(temps * vitesse * 1.2f + decalage) * (float)hauteurEcran * 0.6f;
                float rayon = 50.0f + sinf(temps * 0.5f + decalage) * 30.0f;
                Color couleurBulle = (i%2==0) ? SKYBLUE : LIME;
                couleurBulle.a = 20 + (int)(sinf(temps + decalage)*10);
                DrawCircleV((Vector2){x, y}, rayon, couleurBulle);
            }

            dessiner_bouton_accueil(&etat, jeu);
            int cx = largeurEcran/2; int cy = hauteurEcran/2;
            DrawText("LES 4 REGLES DE CONWAY", cx - MeasureText("LES 4 REGLES DE CONWAY", 40)/2, 50, 40, GOLD);

            int boxH = 120;
            int totalHeight = 4 * (boxH + 20);
            int startY = (hauteurEcran - totalHeight) / 2 + 40;

            DrawRectangle(cx - 300, startY, 600, boxH, Fade(DARKGRAY, 0.5f));
            DrawText("1. SOUS-POPULATION", cx - 280, startY + 20, 20, ORANGE);
            DrawText("Une cellule vivante avec moins de 2 voisines meurt.", cx - 280, startY + 50, 18, WHITE);
            float fade1 = (sinf(temps * 3.0f) + 1.0f) * 0.5f;
            DrawRectangle(cx + 200, startY + 40, 20, 20, Fade(WHITE, fade1));
            DrawRectangle(cx + 200 + 22, startY + 40, 20, 20, WHITE);
            DrawText(fade1 > 0.5f ? "VIVANT" : "MORT", cx + 200, startY + 70, 10, fade1 > 0.5f ? GREEN : RED);

            int y2 = startY + boxH + 20;
            DrawRectangle(cx - 300, y2, 600, boxH, Fade(DARKGRAY, 0.5f));
            DrawText("2. SURVIE", cx - 280, y2 + 20, 20, GREEN);
            DrawText("Une cellule avec 2 ou 3 voisines reste vivante.", cx - 280, y2 + 50, 18, WHITE);
            DrawRectangle(cx + 200, y2 + 40, 20, 20, WHITE);
            DrawRectangle(cx + 200 + 22, y2 + 40, 20, 20, WHITE);
            DrawRectangle(cx + 200 - 22, y2 + 40, 20, 20, WHITE);
            DrawText("STABLE", cx + 200, y2 + 70, 10, GREEN);

            int y3 = y2 + boxH + 20;
            DrawRectangle(cx - 300, y3, 600, boxH, Fade(DARKGRAY, 0.5f));
            DrawText("3. SURPOPULATION", cx - 280, y3 + 20, 20, RED);
            DrawText("Une cellule avec plus de 3 voisines meurt.", cx - 280, y3 + 50, 18, WHITE);
            float fade3 = (sinf(temps * 3.0f) + 1.0f) * 0.5f;
            DrawRectangle(cx + 200, y3 + 40, 20, 20, fade3 > 0.5f ? RED : BLACK);
            DrawRectangle(cx + 200 + 22, y3 + 40, 20, 20, WHITE);
            DrawRectangle(cx + 200 - 22, y3 + 40, 20, 20, WHITE);
            DrawRectangle(cx + 200, y3 + 40 - 22, 20, 20, WHITE);
            DrawRectangle(cx + 200, y3 + 40 + 22, 20, 20, WHITE);

            int y4 = y3 + boxH + 20;
            DrawRectangle(cx - 300, y4, 600, boxH, Fade(DARKGRAY, 0.5f));
            DrawText("4. NAISSANCE", cx - 280, y4 + 20, 20, SKYBLUE);
            DrawText("Une cellule morte avec exactement 3 voisines nait.", cx - 280, y4 + 50, 18, WHITE);
            float fade4 = (sinf(temps * 3.0f - 1.5f) + 1.0f) * 0.5f;
            DrawRectangleLines(cx + 200, y4 + 40, 20, 20, GRAY);
            if (fade4 > 0.3f) DrawRectangle(cx + 200, y4 + 40, 20, 20, Fade(SKYBLUE, fade4));
            DrawRectangle(cx + 200 + 22, y4 + 40, 20, 20, WHITE);
            DrawRectangle(cx + 200 - 22, y4 + 40, 20, 20, WHITE);
            DrawRectangle(cx + 200, y4 + 40 - 22, 20, 20, WHITE);
        }
        else if (etat == SATISFAISANT_CONFIG) {
            dessiner_bouton_accueil(&etat, jeu);
            int cx = largeurEcran/2; int cy = hauteurEcran/2;
            DrawText("CHOISISSEZ VOTRE MOTIF", cx - MeasureText("CHOISISSEZ VOTRE MOTIF", 30)/2, cy - 150, 30, WHITE);
            Rectangle rG = { (float)cx - 200.0f, (float)cy - 30.0f, 50.0f, 60.0f };
            Rectangle rD = { (float)cx + 150.0f, (float)cy - 30.0f, 50.0f, 60.0f };
            if (CheckCollisionPointRec(GetMousePosition(), rG) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                jeu->typeSatisfaisant--; if (jeu->typeSatisfaisant < 0) jeu->typeSatisfaisant = COMPTE_SATISFAISANT - 1;
            }
            if (CheckCollisionPointRec(GetMousePosition(), rD) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                jeu->typeSatisfaisant++; if (jeu->typeSatisfaisant >= COMPTE_SATISFAISANT) jeu->typeSatisfaisant = 0;
            }
            DrawRectangleRec(rG, CheckCollisionPointRec(GetMousePosition(), rG)?LIGHTGRAY:GRAY);
            DrawText("<", (int)rG.x + 15, (int)rG.y + 15, 30, WHITE);
            DrawRectangleRec(rD, CheckCollisionPointRec(GetMousePosition(), rD)?LIGHTGRAY:GRAY);
            DrawText(">", (int)rD.x + 15, (int)rD.y + 15, 30, WHITE);
            const char* nom = NOMS_SATISFAISANTS[jeu->typeSatisfaisant];
            DrawText(nom, cx - MeasureText(nom, 40)/2, cy - 20, 40, PURPLE);
            Rectangle btn = { (float)cx - 100.0f, (float)cy + 80.0f, 200.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), btn) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                configurer_satisfaisant(jeu); etat = SATISFAISANT_JEU;
            }
            DrawRectangleRec(btn, GREEN); DrawText("LANCER", (int)btn.x + 60, (int)btn.y + 15, 20, BLACK);
        }
        else if (etat == DUEL_CONFIG || etat == BR_CONFIG) {
            int cx = largeurEcran/2; int cy = hauteurEcran/2;
            int maxP = (etat == DUEL_CONFIG) ? 2 : jeu->compteJoueursBR;
            if (etat == DUEL_CONFIG) {
                DrawTexturePro(fondDuel, (Rectangle){0, 0, (float)fondDuel.width, (float)fondDuel.height}, (Rectangle){0, 0, (float)largeurEcran, (float)hauteurEcran}, (Vector2){0, 0}, 0.0f, WHITE);
                DrawRectangle(0, 0, largeurEcran, hauteurEcran, Fade(BLACK, 0.3f));
            } else {
                DrawTexturePro(fondBR, (Rectangle){0, 0, (float)fondBR.width, (float)fondBR.height}, (Rectangle){0, 0, (float)largeurEcran, (float)hauteurEcran}, (Vector2){0, 0}, 0.0f, WHITE);
                DrawRectangle(0, 0, largeurEcran, hauteurEcran, Fade(BLACK, 0.3f));
            }
            dessiner_bouton_accueil(&etat, jeu);
            const char* titre = (etat==DUEL_CONFIG) ? "DUEL DE CELLULES" : "BATTLE ROYALE";
            int tx = cx - MeasureText(titre, 50)/2;
            int ty = cy - 250;
            DrawText(titre, tx + 3, ty + 3, 50, BLACK);
            DrawText(titre, tx, ty, 50, (etat==DUEL_CONFIG) ? RAYWHITE : ORANGE);

            if (etat == BR_CONFIG) {
                DrawText("Nombre de Joueurs :", cx - 150, cy - 180, 20, WHITE);
                DrawText("<", cx + 50, cy - 180, 20, (jeu->compteJoueursBR > 2) ? YELLOW : DARKGRAY);
                DrawText(TextFormat("%d", jeu->compteJoueursBR), cx + 70, cy - 180, 20, YELLOW);
                DrawText(">", cx + 90, cy - 180, 20, (jeu->compteJoueursBR < MAX_JOUEURS) ? YELLOW : DARKGRAY);
                int startY = cy - 120;
                DrawRectangle(cx - 250, startY - 10, 500, (maxP * 40) + 20, Fade(BLACK, 0.6f));
                for(int i=1; i<=maxP; i++) {
                    int yPos = startY + ((i-1)*40);
                    Color pColor = ObtenirCouleurJoueur(i);
                    DrawText(TextFormat("J%d:", i), cx - 220, yPos + 5, 20, pColor);
                    Rectangle rBox = { (float)cx - 150.0f, (float)yPos, 350.0f, 30.0f };
                    DrawRectangleRec(rBox, Fade(LIGHTGRAY, 0.8f));
                    if (jeu->saisieActuelle == i) DrawRectangleLinesEx(rBox, 2, pColor);
                    DrawText(jeu->nomsJoueurs[i], (int)rBox.x + 5, (int)rBox.y + 5, 20, BLACK);
                    if (jeu->saisieActuelle == i) DrawText("_", (int)rBox.x + 5 + MeasureText(jeu->nomsJoueurs[i], 20), (int)rBox.y + 5, 20, pColor);
                }
            } else {
                int panelY = cy - 50;
                DrawRectangle(cx - 400, panelY, 300, 100, Fade(BLUE, 0.2f));
                DrawRectangleLines(cx - 400, panelY, 300, 100, BLUE);
                DrawText("JOUEUR 1", cx - 400 + 10, panelY - 30, 20, BLUE);
                Rectangle rBox1 = { (float)cx - 380.0f, (float)panelY + 35.0f, 260.0f, 30.0f };
                DrawRectangleRec(rBox1, Fade(LIGHTGRAY, 0.9f));
                if (jeu->saisieActuelle == 1) DrawRectangleLinesEx(rBox1, 2, BLUE);
                DrawText(jeu->nomsJoueurs[1], (int)rBox1.x + 5, (int)rBox1.y + 5, 20, BLACK);
                if (jeu->saisieActuelle == 1) DrawText("_", (int)rBox1.x + 5 + MeasureText(jeu->nomsJoueurs[1], 20), (int)rBox1.y + 5, 20, BLUE);

                DrawText("VS", cx - MeasureText("VS", 60)/2, panelY + 20, 60, WHITE);

                DrawRectangle(cx + 100, panelY, 300, 100, Fade(RED, 0.2f));
                DrawRectangleLines(cx + 100, panelY, 300, 100, RED);
                DrawText("JOUEUR 2", cx + 100 + 10, panelY - 30, 20, RED);
                Rectangle rBox2 = { (float)cx + 120.0f, (float)panelY + 35.0f, 260.0f, 30.0f };
                DrawRectangleRec(rBox2, Fade(LIGHTGRAY, 0.9f));
                if (jeu->saisieActuelle == 2) DrawRectangleLinesEx(rBox2, 2, RED);
                DrawText(jeu->nomsJoueurs[2], (int)rBox2.x + 5, (int)rBox2.y + 5, 20, BLACK);
                if (jeu->saisieActuelle == 2) DrawText("_", (int)rBox2.x + 5 + MeasureText(jeu->nomsJoueurs[2], 20), (int)rBox2.y + 5, 20, RED);
            }
            Rectangle btnVal = { (float)cx - 100.0f, (float)cy + 200.0f, 200.0f, 50.0f };
            if (CheckCollisionPointRec(GetMousePosition(), btnVal) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (etat == DUEL_CONFIG) { configurer_duel_aleatoire(jeu); etat = DUEL_JEU; }
                else if (etat == BR_CONFIG) { configurer_battleroyale(jeu); etat = BR_JEU; }
                jeu->estEnPause = true;
            }
            DrawRectangleRec(btnVal, GREEN); DrawText("LANCER", (int)btnVal.x + 60, (int)btnVal.y + 15, 20, BLACK);
        }
        else if (etat == DUEL_JEU || etat == BR_JEU || etat == SATISFAISANT_JEU) {
            BeginMode2D(camera);
            if (etat == BR_JEU) {
                float rayonReel = jeu->rayonZone * TAILLE_CELLULE;
                Vector2 centreReel = { (float)(LARGEUR_GRILLE*TAILLE_CELLULE)/2.0f, (float)(HAUTEUR_GRILLE*TAILLE_CELLULE)/2.0f };
                DrawCircleLines((int)centreReel.x, (int)centreReel.y, rayonReel, Fade(RED, 0.5f));
            }
            for (int y=0; y<jeu->lignes; y++) {
                for (int x=0; x<jeu->colonnes; x++) {
                    int idx = y*jeu->colonnes+x;
                    if (jeu->grille[idx] > 0) DrawRectangle(x*TAILLE_CELLULE, y*TAILLE_CELLULE, TAILLE_CELLULE-1, TAILLE_CELLULE-1, obtenir_couleur_cellule(jeu->grille[idx], jeu->grilleAge[idx], etat));
                }
            }
            EndMode2D();

            DrawRectangle(0, 0, largeurEcran, 50, Fade(BLACK, 0.8f));
            DrawText(TextFormat("Gen: %ld", jeu->generation), 70, 15, 20, WHITE);
            DrawText(TextFormat("Zoom: %.2f", camera.zoom), 200, 15, 20, GREEN);
            const char* txtEtat = jeu->estEnPause ? "PAUSE" : "EN COURS";
            Color colEtat = jeu->estEnPause ? RED : GREEN;
            DrawText(txtEtat, largeurEcran/2 - MeasureText(txtEtat, 20)/2, 15, 20, colEtat);

            dessiner_bouton_accueil(&etat, jeu);

            if (etat == DUEL_JEU) {
                float tempsRestant = LIMITE_TEMPS_DUEL - jeu->minuteurCombat;
                if (tempsRestant < 0) tempsRestant = 0;
                DrawText(TextFormat("TEMPS: %.1f", tempsRestant), largeurEcran/2 - 50, 60, 30, WHITE);
            } else if (etat == BR_JEU) {
                int survivants = 0; int maxP = jeu->compteJoueursBR;
                for(int i=1; i<=maxP; i++) if(jeu->scores[i] > 0) survivants++;
                DrawText(TextFormat("SURVIVANTS: %d", survivants), largeurEcran/2 - 80, 60, 30, WHITE);
            } else if (etat == SATISFAISANT_JEU) {
                DrawText(NOMS_SATISFAISANTS[jeu->typeSatisfaisant], largeurEcran/2 - MeasureText(NOMS_SATISFAISANTS[jeu->typeSatisfaisant], 30)/2, 60, 30, PURPLE);
            }

            int totalScore = 0;
            int maxP = (etat == DUEL_JEU) ? 2 : jeu->compteJoueursBR;
            for(int i=1; i<=maxP; i++) totalScore += jeu->scores[i];

            if (totalScore > 0 && etat != SATISFAISANT_JEU) {
                int barreX = 50; int barreY = hauteurEcran - 150; int barreLargeur = largeurEcran - 100; int barreHauteur = 30; int courantX = barreX;
                for(int i=1; i<=maxP; i++) {
                    float pct = (float)jeu->scores[i] / (float)totalScore;
                    int w = (int)(pct * barreLargeur);
                    if (w > 0) {
                        DrawRectangle(courantX, barreY, w, barreHauteur, ObtenirCouleurJoueur(i));
                        if (w > 40) DrawText(TextFormat("%.1f%%", pct * 100.0f), courantX + 5, barreY + 5, 20, BLACK);
                        courantX += w;
                    }
                }
                DrawRectangleLines(barreX, barreY, barreLargeur, barreHauteur, WHITE);
            }
        }
        else if (etat == DUEL_FIN || etat == BR_FIN) {
             dessiner_bouton_accueil(&etat, jeu);
             int cx = largeurEcran/2; int cy = hauteurEcran/2;
             int vainqueur = 0; int maxScore = -1;
             int maxP = (etat == DUEL_FIN) ? 2 : jeu->compteJoueursBR;
             for(int i=1; i<=maxP; i++) { if (jeu->scores[i] > maxScore) { maxScore = jeu->scores[i]; vainqueur = i; } }
             DrawText("FIN DE LA PARTIE", cx - 150, cy - 100, 40, GRAY);
             DrawText("VAINQUEUR :", cx - 100, cy - 40, 30, WHITE);
             DrawText(jeu->nomsJoueurs[vainqueur], cx - MeasureText(jeu->nomsJoueurs[vainqueur], 60)/2, cy + 20, 60, ObtenirCouleurJoueur(vainqueur));
             DrawText(TextFormat("SCORE: %d", maxScore), cx - 100, cy + 80, 30, GOLD);
             DrawText("Appuyez sur MAISON ou ENTREE", cx - 180, cy + 140, 20, GRAY);
        }
        else if (etat == JEU) {
            BeginMode2D(camera);
            for (int y=0; y<jeu->lignes; y++) {
                for (int x=0; x<jeu->colonnes; x++) {
                    int idx = y*jeu->colonnes+x;
                    if (jeu->grille[idx]==1) {
                        DrawRectangle(x*TAILLE_CELLULE, y*TAILLE_CELLULE, TAILLE_CELLULE-1, TAILLE_CELLULE-1, obtenir_couleur_cellule(1, jeu->grilleAge[idx], etat));
                    } else if(camera.zoom>0.5f) {
                        DrawRectangleLines(x*TAILLE_CELLULE, y*TAILLE_CELLULE, TAILLE_CELLULE, TAILLE_CELLULE, GetColor(0x333333FF));
                    }
                }
            }
            DrawRectangleLines(0, 0, jeu->colonnes*TAILLE_CELLULE, jeu->lignes*TAILLE_CELLULE, BLUE);
            EndMode2D();

            DrawRectangle(0, 0, largeurEcran, 50, Fade(BLACK, 0.8f));
            DrawText(TextFormat("Gen: %ld", jeu->generation), 70, 15, 20, WHITE);
            DrawText(TextFormat("Zoom: %.2f", camera.zoom), 200, 15, 20, GREEN);
            const char* txtEtat = jeu->estEnPause ? "PAUSE" : "EN COURS";
            Color colEtat = jeu->estEnPause ? RED : GREEN;
            DrawText(txtEtat, largeurEcran/2 - MeasureText(txtEtat, 20)/2, 15, 20, colEtat);

            DrawRectangle(0, hauteurEcran - 100, 320, 100, Fade(BLACK, 0.7f));
            DrawText("OUTIL ACTUEL :", 20, hauteurEcran - 80, 20, WHITE);
            Color couleurOutil = YELLOW;
            if (motifSelectionne == 1) couleurOutil = ORANGE;
            if (motifSelectionne == 2) couleurOutil = PINK;
            if (motifSelectionne == 3) couleurOutil = RED;
            DrawText(nomOutil, 180, hauteurEcran - 80, 20, couleurOutil);
            const char* txtRot = "0";
            if (rotationMotif == 1) txtRot = "90";
            if (rotationMotif == 2) txtRot = "180";
            if (rotationMotif == 3) txtRot = "270";
            DrawText(TextFormat("Rotation: %s deg", txtRot), 20, hauteurEcran - 45, 20, GOLD);

            int touchesX = largeurEcran - 360;
            DrawRectangle(touchesX - 20, hauteurEcran - 80, 400, 80, Fade(BLACK, 0.7f));
            DrawText("CONTROLES :", touchesX, hauteurEcran - 70, 10, GRAY);
            DrawText("[I/O/P] -> Outils  [R] -> Rotation", touchesX, hauteurEcran - 55, 20, WHITE);
            DrawText("[<] Reculer  [>] Avancer", touchesX, hauteurEcran - 30, 20, GREEN);

            dessiner_bouton_accueil(&etat, jeu);
        }
        EndDrawing();
    }

    UnloadTexture(fondDuel);
    UnloadTexture(fondBR);

    free(jeu->grille); free(jeu->prochaineGrille); free(jeu->grilleAge);
    for(int i=0; i<TAILLE_HISTORIQUE; i++) { free(jeu->historique[i]); free(jeu->historiqueAge[i]); }
    free(jeu); CloseWindow(); return 0;
}