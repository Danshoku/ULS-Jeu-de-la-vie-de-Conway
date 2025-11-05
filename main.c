#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// ---------------------------------------------------------
// Affiche la matrice
// ---------------------------------------------------------
void affichage_matrice(int val, int t[val][val]) {
    int i, j;
    for (i = 0; i < val; i++) {
        for (j = 0; j < val; j++) {
            printf("%d ", t[i][j]);
        }
        printf("\n"); //0 = morte, 1 = vivante)
    }
    printf("\n");
}

// ---------------------------------------------------------
// Remplit la matrice manuellement
// ---------------------------------------------------------
void remplissage_matrice(int val, int t[val][val]) {
    int i, j;
    printf("Nous utiliserons des 0 pour les cases mortes et 1 pour les cases vivantes\n");
    for (i = 0; i < val; i++) {
        for (j = 0; j < val; j++) {
            do {
                printf("Saisissez la valeur pour la case [%d][%d] : ", i, j);
                scanf("%d", &t[i][j]);
                if (t[i][j] < 0 || t[i][j] > 1)
                    printf("Veuillez saisir uniquement 0 ou 1.\n");
            } while (t[i][j] < 0 || t[i][j] > 1);
        }
    }
}

// ---------------------------------------------------------
// Remplit la matrice aléatoirement
// ---------------------------------------------------------
void remplissage_aleatoire(int val, int t[val][val]) {
    srand(time(NULL));
    for (int i = 0; i < val; i++) {
        for (int j = 0; j < val; j++) {
            t[i][j] = rand() % 2; // 0 ou 1 aléatoirement
        }
    }
}

// ---------------------------------------------------------
// Compte le nombre de voisins vivants d'une cellule
// ---------------------------------------------------------
int compte_voisins(int val, int t[val][val], int x, int y) {
    int voisins = 0;

    // On parcourt les 8 cases autour de la cellule
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (!(i == 0 && j == 0)) { // on ignore la cellule elle-même
                int nx = (x + i + val) % val;
                int ny = (y + j + val) % val;
                voisins += t[nx][ny];
            }
        }
    }

    return voisins;
}

// ---------------------------------------------------------
// Calcule la génération suivante
// ---------------------------------------------------------
void generation_suivante(int val, int t[val][val]) {
    int temp[val][val];

    for (int i = 0; i < val; i++) {
        for (int j = 0; j < val; j++) {
            int voisins = compte_voisins(val, t, i, j);

            // Règles du jeu de la vie
            if (t[i][j] == 1 && (voisins < 2 || voisins > 3))
                temp[i][j] = 0; // meurt
            else if (t[i][j] == 0 && voisins == 3)
                temp[i][j] = 1; // naissance
            else
                temp[i][j] = t[i][j]; // reste identique
        }
    }

    // Copie du tableau temp dans le tableau principal
    for (int i = 0; i < val; i++) {
        for (int j = 0; j < val; j++) {
            t[i][j] = temp[i][j];
        }
    }
}

// ---------------------------------------------------------
// Menu principal
// ---------------------------------------------------------
int menu(int val, int t[val][val]) {
    int rep;
    printf("\n=== MENU ===\n");
    printf("1 - Afficher la matrice\n");
    printf("2 - Remplir la matrice manuellement\n");
    printf("3 - Remplir la matrice aléatoirement\n");
    printf("4 - Lancer le jeu de la vie\n");
    printf("5 - Quitter\n");
    printf("Votre choix : ");
    scanf("%d", &rep);

    switch (rep) {
        case 1:
            affichage_matrice(val, t);
            return 1;
        case 2:
            remplissage_matrice(val, t);
            return 1;
        case 3:
            remplissage_aleatoire(val, t);
            return 1;
        case 4: {
            int nb_gen;
            printf("Combien de générations voulez-vous simuler ? ");
            scanf("%d", &nb_gen);
            for (int g = 0; g < nb_gen; g++) {
                printf("\nGénération %d :\n", g + 1);
                affichage_matrice(val, t);
                generation_suivante(val, t);
            }
            return 1;
        }
        case 5:
            return 0;
        default:
            printf("Choix invalide.\n");
            return 1;
    }
}

// ---------------------------------------------------------
// Programme principal
// ---------------------------------------------------------
int main(void) {
    int val;
    printf("Choisissez la taille de la matrice (ex: 5) : ");
    scanf("%d", &val);

    int t[val][val];
    for (int i = 0; i < val; i++)
        for (int j = 0; j < val; j++)
            t[i][j] = 0; // initialisation à 0

    int continuer = 1;
    while (continuer == 1) {
        continuer = menu(val, t);
    }

    printf("Fin du programme.\n");
    return 0;
}
