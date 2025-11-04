
#include <stdio.h>
#include <stdlib.h>

void affichage_matrice(int val, int t[val][val]) {
    int j,i;
    for (i=0;i<val;i++) {
        printf("\n");
        for (j=0;j<val;j++) {
            printf("%i ",t[i][j]);
        }
    }
    printf("\n\n");
}

void remplissage_matrice(int val, int t[val][val]) {
    int j,i;
    printf("Nous utiliserons des 0 pour les cases vides et 1 pour les cases pleines\n");
    for (i=0;i<val;i++) {
        for (j=0;j<val;j++) {
            do {
                printf("Saissez votre valeur pour la case [%i][%i]\n",i,j);
                scanf("%i",&t[i][j]);
                if (t[i][j]<0||t[i][j]>1) {
                    printf("Veuillez saisir que des valeurs de 0 ou 1\n");
                }
            }while (t[i][j]<0||t[i][j]>1);
        }
    }
}


// Menu principal
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

            return 1;
        case 4: {
            int nb_gen;
            printf("Combien de générations voulez-vous simuler ? ");
            scanf("%d", &nb_gen);
            for (int g = 0; g < nb_gen; g++) {
                printf("\nGénération %d :\n", g + 1);
                affichage_matrice(val, t);

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


// Programme principal
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