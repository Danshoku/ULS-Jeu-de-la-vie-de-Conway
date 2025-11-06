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
// Compte chaque case autour
// ---------------------------------------------------------

int compte_voisins(int val, int t[val][val], int x, int y) {
    int cnt = 0; // compteur de voisins
    int i,j;
    for (i=x-1; i<=x+1; i++) {
        for (j=y-1; j<=y+1; j++) {
            if (i==x && j==y) {
                continue;
            }
            if (i >= 0 && i < val && j >= 0 && j < val) {
                if (t[i][j] == 1)
                    cnt++;
            }
            else{
                cnt = cnt;
            }
        }
    }
    return cnt;
}

// ---------------------------------------------------------
// Calcule des générations suivantes
// ---------------------------------------------------------

void calcul_gen(int val, int t[val][val]) {
    int x,y,i,j;
    int res = 0;
    int temp[val][val];
    for (x=0; x < val; x++) {
        for (y=0; y < val; y++) {
            if (t[x][y] == 0) {
                res = compte_voisins(val, t, x, y);
                if (res==3) {
                    temp[x][y] = 1;
                }
                else {
                    temp[x][y] = 0;
                }
            }
            else {
                res = compte_voisins(val, t, x, y);
                if (res==3 || res==2) {
                    temp[x][y] = 1;
                }
                else {
                    temp[x][y] = 0;
                }
            }
        }
    }
    affichage_matrice(val, temp);
    for (i = 0; i < val; i++) {
        for (j = 0; j < val; j++) {
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
    printf("3 - Remplir la matrice aleatoirement\n");
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
            printf("Combien de generations voulez-vous simuler ? ");
            scanf("%d", &nb_gen);
            printf("\nGeneration de base :\n");
            affichage_matrice(val, t);
            for (int g = 0; g < nb_gen; g++) {
                printf("\nGeneration %d :\n", g + 1);
                calcul_gen(val,t);
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
