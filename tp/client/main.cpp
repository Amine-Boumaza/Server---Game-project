/** ------------------------------------------------------------------------------------*
 *                                  c'est le code du client                             *
 * programme �crit par le Dr. Brahimi said                                              *
 * charg� de module R�seaux de communication - 2eme ann�e licence en informatique       *
 * universit� 8 mai 1945 - Guelma (univ-guelma.dz)                                      *
 *-------------------------------------------------------------------------------------**/

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

/****************************************************************************************
 *  variabble globbale --> ne pas changer                                                *
 *-------------------------------------------------------------------------------------- */
#define LG_MESSAGE 50
#define ADD_IP_SERVER "127.0.0.1" // � changer en cas de r�seau
#define PORT 1977

#define ATTEND_ADVERSAIRE "1"     /* envoy� au premier joueur */
#define VEILLEZ_JOUER "2"         /* envoy� au premier joueur */
#define MANIPULATION_OK "3"       /* envoy� au joueur qui fait manipulation correct*/
#define MAUVAISE_MANIPULATION "4" /* envoy� au joueur qui fait mauvaise manipulation */
#define TU_A_GAGNE "5"            /* envoy� au joueur qui fait mauvaise manipulation */
#define ECHEC "6"                 /* envoy� au joueur qui a �chou� */
#define ADVERSAIRE_DECONNECTE "7" /* envoy� au joueur si son adversaire est d�connect�  */

char Gril_jeu[3][3] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
char NomJoueur[LG_MESSAGE];
char nomAdversaire[LG_MESSAGE];
SOCKET socketDialogue; // pour communiquer avec le serveur
int etat_connexion;
int etat_envoi;
int etat_reception;
int resultat_de_jeu;
/****************************************************************************************
 *   initialisation et fermeture de la bibioth�que de de communication WinSock          *
 *
 ****************************************************************************************/

void init_bib()
{
    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 0), &WSAData);
}
/*--------------------------------------------------------------------------------------*/

void fermer_bib()
{
    WSACleanup(); // termine l�utilisation
}

/****************************************************************************************
 *                cr�er un socket qui sert d'un canal logique entre                     *
 *                         le client et la couche transport                             *
 ****************************************************************************************/

int creation_de_socket(SOCKET *sockt)
{
    // Cr�e un socket de communication
    *sockt = socket(AF_INET, SOCK_STREAM, 0); /* 0 indique que l�on utilisera le
                                                 protocole par d�faut associ� � SOCK_STREAM soit TCP */
    if (*sockt == INVALID_SOCKET)
    {
        printf("Erreur de creation de Socket \n");
        return 0;
    }
    return 1;
}

/****************************************************************************************
 *                  Gestion de connexion entre le client et le Serveur                  *
 ****************************************************************************************/

int se_deconnecter_de_serveur(SOCKET sockt)
{
    int retour = closesocket(sockt);

    // en cas d'erreur
    if (retour == SOCKET_ERROR)
    {
        printf("Probleme de fermm�ture de socket \n");
        return 0;
    }
    return 1;
}
/*-*************************************************************************************/
int se_connecter_avec_serveur(SOCKET *sockt, const char add_IP[], const u_short n_port)
{ /*add_IP est l'adresse IP du serveur
et n_port est son n� de port */
    /* d�clarer la structure qui va sp�cifier les cordon�es(add ip et n� de port de serveur; */
    struct sockaddr_in cordonees_de_serveur;

    /* cr�er socket de commmunication avec le serveur  */
    if (creation_de_socket(sockt) == 0)
        return 0;

    /* pr�parer la structure de cordonn�es qui va �tre utiliser pour se connecter avec le serveur   */
    cordonees_de_serveur.sin_family = AF_INET;
    cordonees_de_serveur.sin_addr.s_addr = inet_addr(add_IP); /* pr�ciser l'adresse IPv4 de serveur */
    cordonees_de_serveur.sin_port = htons(n_port);            /* pr�ciser le num�ro de port de serveur */

    // �talissement de connection avec le processus distant (serveur)
    int retour = connect(*sockt,
                         (SOCKADDR *)&cordonees_de_serveur,
                         sizeof(cordonees_de_serveur));

    if (retour == SOCKET_ERROR)
    {
        printf("Probleme de connection avec le serveur \n");
        se_deconnecter_de_serveur(*sockt);
        return 0; // On sort en indiquant un code erreur
    }
    // est_connecte = 1 ;
    printf("Connexion avec serveur reussite ! ... \n");
    return 1;
}
/****************************************************************************************
 *                                gestion de communication                              *
 ****************************************************************************************/

int envoyer_message_au_serveur(SOCKET sockt, char message_a_envoyer[])
{
    int ecrits = send(sockt, message_a_envoyer, (int)strlen(message_a_envoyer), 0); /* message � TAILLE variable */

    if (ecrits == SOCKET_ERROR)
    {
        printf("Probleme d'envoi du message \n");
        return 0;
    }
    printf("Envoi du Message <%s> de %d octets avec succes \n", message_a_envoyer, ecrits);
    return 1;
}

/*-**************************************************************************************/

int recevoir_message_du_serveur(SOCKET sockt, char *buffer)
{ /* Reception des donn�es du serveur */

    char message_a_recevoir[LG_MESSAGE] = {0}; /* le message de la couche Application ! */
    int lus = recv(sockt, message_a_recevoir, sizeof(message_a_recevoir), 0);
    /* attend un message de TAILLE max fixe */

    if (lus > 0)
    { /* r�ception de lus octets */
        // printf("          Reponse de serveur : \n%s \n", message_a_recevoir);
        strcpy(buffer, message_a_recevoir);
        return 1;
    }
    if (lus == 0)
    { /* la socket est ferm�e par le serveur */
        printf("Serveur deconnecte (Socket est ferm�e par le serveur) !\n");
        // est_connecte = 0 ;
        return 0;
    }
    else
    { /* une erreur ! */
        printf("Probleme de transmmission (erreur de lecture de socket) \n");
        return 0;
    }
}

/****************************************************************************************
 *                     gestion de communication avec l'utilisateur (joueur)              *
 *****************************************************************************************/

void demmnder_nom_joueur()
{
    // demmmander de l''utilisateur de taper son nom
    printf("taper votre Nom : ");
    gets(NomJoueur);
}

/*-*************************************************************************************/

void demander_placer_une_piece(int n, int *x, int *y)
{
    // demander de l'utilisateur d'exprimer son deplacement
    printf("Ou tu va placer la piece n %d ? tu va donner x et y : \n    ", n);
    scanf("%d %d", x, y);
    printf("  Alors tu la place dans (%d,%d) \n", *x, *y);
}

/*-*************************************************************************************/

void demander_deplacer_une_piece(int *xs, int *ys, int *xc, int *yc)
{
    // demander de l'utilisateur d'exprimer son deplacement
    printf("Quelle piece tu va Deplacer ? (x et y de sources et x et y de cible): \n    ");
    scanf("%d %d %d %d", xs, ys, xc, yc);
    printf("   Alors tu joue (%d,%d)-->(%d,%d) \n", *xs, *ys, *xc, *yc);
}

/*-**************************************************************************************/

void afficher_gril()
{
    system("cls"); // pour effacer l'ecran
    printf("\n             [ %c ]   [ %c ]   [ %c ]", Gril_jeu[0][0], Gril_jeu[0][1], Gril_jeu[0][2]);
    printf("\n  ");
    printf("\n             [ %c ]   [ %c ]   [ %c ]", Gril_jeu[1][0], Gril_jeu[1][1], Gril_jeu[1][2]);
    printf("\n  ");
    printf("\n             [ %c ]   [ %c ]   [ %c ] \n", Gril_jeu[2][0], Gril_jeu[2][1], Gril_jeu[2][2]);
}

/*-**************************************************************************************/

void afficher_scord(int sc)
{
    if (sc == 2) // joueur a gagn�
        printf("\n<<<< Felicitation, tu est vaiqueur >>>");
    else if (sc == 3) // �chec
        printf("\n<<<< Domage, Echec >>>");
    else if (sc == 0) // �chec
        printf("\n<<<< Probleme de communication >>>");
}

/****************************************************************************************
 *                                  Gestion de jeu                                       *
 ***************************************************************************************b**/

void extraire_place(char *message_a_recevoir, int *x, int *y)
{
    *x = 0;
    *y = 0;
    if ((strlen(message_a_recevoir) == 3) & (message_a_recevoir[1] == ' '))
    {
        // printf("============= \n");
        if (message_a_recevoir[0] == '1')
            *x = 1;
        else if (message_a_recevoir[0] == '2')
            *x = 2;
        else if (message_a_recevoir[0] == '3')
            *x = 3;

        if (*x != 0)
        {
            if (message_a_recevoir[2] == '1')
                *y = 1;
            else if (message_a_recevoir[2] == '2')
                *y = 2;
            else if (message_a_recevoir[2] == '3')
                *y = 3;
        }
    }
}

/*-**************************************************************************************/

void extraire_deplacement(char *message_a_recevoir, int *xs, int *ys, int *xc, int *yc)
{
    /** A compl�ter    **/
    *xs = 0;
    *ys = 0;
    *xc = 0;
    *yc = 0;
    if ((strlen(message_a_recevoir) == 7) & (message_a_recevoir[1] == ' ') & (message_a_recevoir[3] == ' ') & (message_a_recevoir[5] == ' '))
    {
        // printf("============= \n");
        if (message_a_recevoir[0] == '1')
            *xs = 1;
        else if (message_a_recevoir[0] == '2')
            *xs = 2;
        else if (message_a_recevoir[0] == '3')
            *xs = 3;

        if (*xs != 0)
        {
            if (message_a_recevoir[2] == '1')
                *ys = 1;
            else if (message_a_recevoir[2] == '2')
                *ys = 2;
            else if (message_a_recevoir[2] == '3')
                *ys = 3;
        }

        if (*ys != 0)
        {
            if (message_a_recevoir[4] == '1')
                *xc = 1;
            else if (message_a_recevoir[4] == '2')
                *xc = 2;
            else if (message_a_recevoir[4] == '3')
                *xc = 3;
        }
        if (*xc != 0)
        {
            if (message_a_recevoir[6] == '1')
                *yc = 1;
            else if (message_a_recevoir[6] == '2')
                *yc = 2;
            else if (message_a_recevoir[6] == '3')
                *yc = 3;
        }
    }
}

/*-**************************************************************************************/

void prepare_message(int x, int y, char *message)
{
    char mg[4] = "0 0";
    if (x == 1)
        mg[0] = '1';
    else if (x == 2)
        mg[0] = '2';
    else if (x == 3)
        mg[0] = '3';

    if (y == 1)
        mg[2] = '1';
    else if (y == 2)
        mg[2] = '2';
    else if (y == 3)
        mg[2] = '3';
    strcpy(message, mg);
}

/*-**************************************************************************************/

void prepare_message(int xs, int ys, int xc, int yc, char *message)
{
    /** A compl�ter    **/
    char mg[8] = "0 0 0 0";
    if (xs == 1)
        mg[0] = '1';
    else if (xs == 2)
        mg[0] = '2';
    else if (xs == 3)
        mg[0] = '3';

    if (ys == 1)
        mg[2] = '1';
    else if (ys == 2)
        mg[2] = '2';
    else if (ys == 3)
        mg[2] = '3';

    if (xc == 1)
        mg[4] = '1';
    else if (xc == 2)
        mg[4] = '2';
    else if (xc == 3)
        mg[4] = '3';

    if (yc == 1)
        mg[6] = '1';
    else if (yc == 2)
        mg[6] = '2';
    else if (yc == 3)
        mg[6] = '3';
    strcpy(message, mg);
}

/*-**************************************************************************************/

void mettre_a_jeur_gril(int xs, int ys, int xc, int yc)
{
    Gril_jeu[xc - 1][yc - 1] = Gril_jeu[xs - 1][ys - 1];
    Gril_jeu[xs - 1][ys - 1] = ' ';
}

/*-**************************************************************************************/

void mettre_a_jeur_gril(char type, int x, int y)
{

    Gril_jeu[x - 1][y - 1] = type;
}

/*-**************************************************************************************/

void initialisation_du_Gril()
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            Gril_jeu[i][j] = ' ';
}

int initialisation_du_jeu()
{
    char message_a_recevoir[LG_MESSAGE];
    /*  phase d'�tablissement de connexion */
    etat_connexion = se_connecter_avec_serveur(&socketDialogue, (char *)ADD_IP_SERVER, PORT);
    if (etat_connexion == 0)
        return 0; /* quitter l'ex�cution, il y a un probl�me de connexion �choue */
    /* envoyer un message au serveur */
    etat_envoi = envoyer_message_au_serveur(socketDialogue, NomJoueur);
    if (etat_envoi == 0)
        return 0; /* quitter l'ex�cution, il y a un probl�me lors de l'envoi du message */
    /* recevoir un message de serveur */
    etat_reception = recevoir_message_du_serveur(socketDialogue, message_a_recevoir);
    if (etat_reception == 0)
        return 0; /* quitter l'ex�cution, il y a un probleme de communication  */

    if (strcmp(message_a_recevoir, "1") == 0)
    {
        printf("Bien venu dans le jeu d'allignement de 3 pieces \n   Tu va attendre ton adverssaire  \n");
        etat_reception = recevoir_message_du_serveur(socketDialogue, message_a_recevoir);
        if (etat_reception == 0)
            return 0; /* quitter l'ex�cution, il y a un probleme de communication  */
        strcpy(nomAdversaire, message_a_recevoir);
        printf("Tu va jouer contre <%s>  \n  Veuillez attendre le tour \n", nomAdversaire);
    }
    else
        printf("Bien venu dans le jeu d'allignement de 3 pieces \n   Tu va jouerre <%s> \n Veuillez attendre le tour \n", message_a_recevoir);
    return 1;
}

/*-**************************************************************************************/

int placement_des_pieces()
{

    char message_a_recevoir[LG_MESSAGE], message_a_envoyer[LG_MESSAGE];
    int x, y;
    int nb_pieces_placee = 0, nb_pieces_placee_adv = 0;

    while (nb_pieces_placee != 3 || nb_pieces_placee_adv != 3)
    {

        etat_reception = recevoir_message_du_serveur(socketDialogue, message_a_recevoir);
        if (etat_reception == 0)
            return 0;
        if (strcmp(message_a_recevoir, VEILLEZ_JOUER) == 0)
        {

            demander_placer_une_piece(nb_pieces_placee + 1, &x, &y);
            prepare_message(x, y, message_a_envoyer);
            etat_envoi = envoyer_message_au_serveur(socketDialogue, message_a_envoyer);
            if (etat_envoi == 0)
                return 0;
        }
        else if (strcmp(message_a_recevoir, MANIPULATION_OK) == 0)
        {
            printf("Votre piece est bien placee..Attendez votre adversaire..\n");
            nb_pieces_placee++;
            mettre_a_jeur_gril('X', x, y);
            afficher_gril();
        }
        else if (strcmp(message_a_recevoir, MAUVAISE_MANIPULATION) == 0)
        {
            printf("MAUVAISE MANIPULATION.. Redonner les cordonnees\n");
            demander_placer_une_piece(nb_pieces_placee + 1, &x, &y);
            prepare_message(x, y, message_a_envoyer);
            etat_envoi = envoyer_message_au_serveur(socketDialogue, message_a_envoyer);
            if (etat_envoi == 0)
                return 0;
        }
        else if (strcmp(message_a_recevoir, ECHEC) == 0)
        {
            printf("ECHEC..\n");
            return 3;
        }
        else if (strcmp(message_a_recevoir, TU_A_GAGNE) == 0)
            return 2;
        else if (strcmp(message_a_recevoir, ADVERSAIRE_DECONNECTE) == 0)
            return 4;
        else
        {
            extraire_place(message_a_recevoir, &x, &y);
            mettre_a_jeur_gril('O', x, y);
            afficher_gril();
            nb_pieces_placee_adv++;
        }
    }

    return 1;
}

/*-**************************************************************************************/

int deplacement_des_pieces()
{
    char message_a_recevoir[LG_MESSAGE], message_a_envoyer[LG_MESSAGE];
    int xs, ys, xc, yc;
    while (true)
    {
        etat_reception = recevoir_message_du_serveur(socketDialogue, message_a_recevoir);
        if (etat_reception == 0)
            return 0;
        if (strcmp(message_a_recevoir, VEILLEZ_JOUER) == 0)
        {
            printf("C\'est votre tour..\n");
            demander_deplacer_une_piece(&xs, &ys, &xc, &yc);
            prepare_message(xs, ys, xc, yc, message_a_envoyer);
            etat_envoi = envoyer_message_au_serveur(socketDialogue, message_a_envoyer);
            if (etat_envoi == 0)
                return 0;
        }
        else if (strcmp(message_a_recevoir, MANIPULATION_OK) == 0)
        {
            printf("Votre piece est bien deplacee..Attendez votre adversaire..\n");
            mettre_a_jeur_gril(xs, ys, xc, yc);
            afficher_gril();
        }
        else if (strcmp(message_a_recevoir, MAUVAISE_MANIPULATION) == 0)
        {
            printf("MAUVAISE DEPLACEMENT.. Redonner les cordonnees\n");
            demander_deplacer_une_piece(&xs, &ys, &xc, &yc);
            prepare_message(xs, ys, xc, yc, message_a_envoyer);
            etat_envoi = envoyer_message_au_serveur(socketDialogue, message_a_envoyer);
            if (etat_envoi == 0)
                return 0;
        }
        else if (strcmp(message_a_recevoir, ECHEC) == 0)
        {
            printf("ECHEC..\n");
            return 3;
        }
        else if (strcmp(message_a_recevoir, TU_A_GAGNE) == 0)
            return 2;
        else if (strcmp(message_a_recevoir, ADVERSAIRE_DECONNECTE) == 0)
        {

            return 4;
        }
        else
        {
            extraire_deplacement(message_a_recevoir, &xs, &ys, &xc, &yc);
            mettre_a_jeur_gril(xs, ys, xc, yc);
            afficher_gril();
        }
    }
}

/****************************************************************************************/

int main()
{

    init_bib(); // initialiser la biblioth�que "WinSock"

    initialisation_du_Gril();
    afficher_gril();
    demmnder_nom_joueur();
    initialisation_du_jeu();
    int resultat = placement_des_pieces();
    if (resultat == 4)
        printf("\nADVERSAIRE DECONNECTE\n");
    if (resultat != 1)
        afficher_scord(resultat);
    else
    {
        resultat = deplacement_des_pieces();
        if (resultat == 4)
            printf("\nADVERSAIRE DECONNECTE\n");
        afficher_scord(resultat);
    }

    fermer_bib();
    return 1;
}
/****************************************************************************************/
