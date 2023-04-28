/** ------------------------------------------------------------------------------------------*
 *                                  c'est le code du serveur                                  *
 * programme r�alis� par le docteur Brahimi said, charg� de cours r�seaux de communication    *
 * 2eme ann�e licence informatique, universit� 8 mai 1954 - Guelma (univ-guelma.dz)           *
 *-------------------------------------------------------------------------------------------**/

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

/**===========================================================================================*
 *                       variabble globbale --> � ne pas changer                              *
 *--------------------------------------------------------------------------------------------*/
#define LG_MESSAGE 256
#define PORT 1977
#define ATTEND_ADVERSAIRE "1"     /* envoy� au premier joueur */
#define VEILLEZ_JOUER "2"         /* envoy� au premier joueur */
#define MANIPULATION_OK "3"       /* envoy� au joueur qui fait manipulation correct*/
#define MAUVAISE_MANIPULATION "4" /* envoy� au joueur qui fait mauvaise manipulation */
#define TU_A_GAGNE "5"            /* envoy� au joueur qui fait mauvaise manipulation */
#define ECHEC "6"                 /* envoy� au joueur qui a �chou� */
#define ADVERSAIRE_DECONNECTE "7" /* envoy� au joueur si son adversaire est d�connect�  */

SOCKET socketEcoute;                  /* utilis� seulement pour la r�ception des demmandes de connexion  */
SOCKET socketDialogue1;               /* utilis� pour la communication avec le premier joueur */
SOCKET socketDialogue2;               /* utilis� pour la communication avec le deuxi�me joueur */
char derniere_action_jeu[LG_MESSAGE]; // nom du 1er joueur
char nomJ1[LG_MESSAGE];               // nom du 1er joueur
char nomJ2[LG_MESSAGE];               // nom du 2eme joueur
int etat_de_connexion;
int Gril_jeu[3][3];

/****************************************************************************************
 *   initialisation et fermeture de la bibioth�que de de communication WinSock          *
 ****************************************************************************************/
void init_bib()
{ /*  initialisation  --- � ne pas changer */
    WSADATA WSAData;
    WSAStartup(MAKEWORD(2, 0), &WSAData);
}
//----------------------------------------------------
void fermer_bib()
{ /*  fermmeture  */
    WSACleanup();
}
/****************************************************************************************
 * creation et configuration du socket d'�ccoute qui va permettre au serveur de recevoir *
 * les demandes de connexion des clients (joueurs)                                       *
 *****************************************************************************************/
int creer_socket_ecoute(SOCKET *sockEcoute)
{
    // Cr�e un socket de communication
    SOCKET sockt = socket(AF_INET, SOCK_STREAM, 0); /* 0 indique que l�on utilisera
                                                       le protocole par d�faut associ� �
                                                       SOCK_STREAM soit TCP */
    if (sockt == INVALID_SOCKET)
    {
        printf("Erreur de creation socket \n");
        return 0;
    }
    *sockEcoute = sockt;
    printf("socket creer avec succes \n");
    return 1;
}
/*---------------------------------------------------*
 *  lier la socket � une adresse (bind)              *
 *---------------------------------------------------*/
int socket_bind(SOCKET sockEcoute)
{
    // On pr�pare l�adresse d�attachement locale
    struct sockaddr_in pointDeRencontreLocal; /* Renseigne la structure sockaddr_in avec les informations locales du serveur   */

    pointDeRencontreLocal.sin_family = AF_INET;
    pointDeRencontreLocal.sin_addr.s_addr = htonl(INADDR_ANY); // toutes les interfaces locales disponibles

    // On choisit le num�ro de port d��coute du serveur
    pointDeRencontreLocal.sin_port = htons(PORT);
    int retour = bind(sockEcoute, (SOCKADDR *)&pointDeRencontreLocal, sizeof(pointDeRencontreLocal));
    if (retour == SOCKET_ERROR)
    {
        printf("Erreur bind socket \n");
        return 0;
    }
    printf("Socket attachee avec succes !\n");
    // On fixe la taille de la file d�attente (pour les demandes de connexion non encore trait�es)
    if (listen(sockEcoute, SOMAXCONN) == SOCKET_ERROR)
    {
        printf("Erreur listen socket \n");
        return 0;
    }
    printf("Attente la connexion des Clients (Socket placee en ecoute passive)...\n\n");
    return 1;
}
/*---------------------------------------------------*
 *  cr�er et configurer le socket d'�coute           *
 *  on utilise pour cela les 2 procedures pr�c�dentes*              *
 *---------------------------------------------------*/
int creation_et_configuration_socket_eccoute(SOCKET *socketEcoute)
{
    /*socketEcoute est un socket utilis� seulement pour la r�ception des demmandes de connexion  */

    int creation_reussite = creer_socket_ecoute(socketEcoute);
    if (creation_reussite == 0)
        return 0; /* probl�me de cr�ation de socket d'�coute */

    int bind_correcte = socket_bind(*socketEcoute);
    if (bind_correcte == 0)
        return 0;

    return 1;
}

/****************************************************************************************
 *                  gestion de connexion et fermeture de connexion                      *
 *                              (fermeture des sockets)                                 *
 ****************************************************************************************/

void fermeture_de_socket_Ecoute(SOCKET socktEcoute)
{ // � ne pas changer
    /* On ferme la ressource avant de quitter */
    int etat_de_fermeture = closesocket(socktEcoute);
    if (etat_de_fermeture == SOCKET_ERROR)
    {
        printf("Erreur fermeture socket : %d\n", WSAGetLastError());
    }
}

/*-*************************************************************************************/

void fermeture_de_socket_dialog(SOCKET sockDialogue)
{ // � ne pas changer
    int retour = closesocket(sockDialogue);
    if (retour == SOCKET_ERROR)
    {
        printf("Probleme de fermm�ture de connexion (erreur de fermeture de socket) \n");
    }
    printf("Fermm�ture de connexion (fermeture de socket) \n");
}

/*-*************************************************************************************/

int connexion_d_un_client(SOCKET sockEcoute, SOCKET *sockDialogue)
{
    /* cette procedure peret d''attendre la connexion d'un client (joueur) (via accept)
     * elle ne termine que si le lient se connecte ou s'il erreur de communication */
    struct sockaddr_in pointDeRencontreDistant;
    int longueurAdresse = sizeof(pointDeRencontreDistant);

    /* acceppt : est un appel bloquant : on ne continue l'e�xution jusqu'� la connexion d'un client */
    SOCKET socktDialg = accept(sockEcoute, (SOCKADDR *)&pointDeRencontreDistant, &longueurAdresse);

    if (socktDialg == INVALID_SOCKET)
    {
        printf("Probleme de communication (Erreur accept socket) \n");
        return 0;
    }
    // printf("Un client (Adresse : %d - N Port :%d) s'est connecte \n" , pointDeRencontreDistant.sin_addr,pointDeRencontreDistant.sin_port);
    printf("Un client s'est connecte \n");
    *sockDialogue = socktDialg;
    return 1;
}

/****************************************************************************************
 *  gestion de communication avec un client (joueur)                                    *                       *
 ****************************************************************************************/

int recevoir_message_de_client(SOCKET sockDialogue, char *buffer)
{
    int nbr_octets_lus;                        /* nb d�octets ecrits et lus */
    char message_a_recevoir[LG_MESSAGE] = {0}; /* le message de la couche Application ! */

    // r�ception d'un message de client
    nbr_octets_lus = recv(sockDialogue, message_a_recevoir, sizeof(message_a_recevoir), 0);

    if (nbr_octets_lus > 0)
    { /* r�ception coorecte de n octets */
        printf("message recu :\"%s\"\n", message_a_recevoir);
        strcpy(buffer, message_a_recevoir);
        return 1; // retoure dans le cas d'une reception correcte d'un mess&ge de
    }
    // le cas o� le serveur ne peut pas recevoir de mmessage
    if (nbr_octets_lus == 0) /* on ne peut pas recevoir d'informmation car le socket est ferm�e par le serveur */
        printf("Connexion avec le Client s'est fermee (socket fermee)\n");
    else /* on ne peut pas recevoir d'informmation car il y a une erreur quelque part ! */
        printf("Probleme de communication avec le Client (Erreur lecture socket) \n");

    return 0;
}
/*-*************************************************************************************/
int envoyer_message_au_client(SOCKET sockDialogue, const char message_a_envoyer[])
{
    int nbr_octets_envoyes;
    nbr_octets_envoyes = send(sockDialogue, message_a_envoyer, (int)strlen(message_a_envoyer), 0); // message � TAILLE variable
    if (nbr_octets_envoyes == SOCKET_ERROR)
    {
        printf("Probleme d'envoi du message au client (Erreur envoi sur socket)\n");

        // dans les deux cas d'erreur, le serveurs vas se d�connecter
        return 0;
    }
    printf("le message \"%s\" envoye au client avec succes \n\n", message_a_envoyer);
    return 1;
}

/****************************************************************************************
 *                                      Gestion de jeu                                   *
 *****************************************************************************************/

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

int deplacement_valide(int joueur, int xs, int ys, int xc, int yc)
{
    /** A compl�ter    **/

    if (xs == 0 || ys == 0 || xc == 0 || yc == 0)
        return 0;
    else
    {
        if (Gril_jeu[xs - 1][ys - 1] != joueur)
            return 0;
        else
        {
            if (((xc == xs - 1) ^ (xc == xs + 1) ^ (xs == xc)) && ((yc == ys - 1) ^ (yc == ys + 1) ^ (yc == ys)))
            {
                if (Gril_jeu[xc - 1][yc - 1] != 0)
                    return 0;
            }
            else
                return 0;
        }
    }
    return 1;
}

int placement_valide(int x, int y)
{
    /** A compl�ter    **/
    if (x == 0 || y == 0)
        return 0;
    else
    {
        if (Gril_jeu[x - 1][y - 1] != 0)
            return 0;
    }
    return 1;
}

/*-**************************************************************************************/

int mettre_a_jeur_gril(int joueur, int xs, int ys, int xc, int yc)
{
    if (deplacement_valide(joueur, xs, ys, xc, yc) == 0)
        return 0;
    Gril_jeu[xc - 1][yc - 1] = Gril_jeu[xs - 1][ys - 1];
    Gril_jeu[xs - 1][ys - 1] = 0;
    return 1;
}

/*-**************************************************************************************/

int mettre_a_jeur_gril(int joueur, int x, int y)
{
    /** A compl�ter    **/
    if (placement_valide(x, y) == 0)
        return 0;
    Gril_jeu[x - 1][y - 1] = joueur;
    return 1;
}

/*-**************************************************************************************/

void initialiser_gril()
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            Gril_jeu[i][j] = 0;
}

/*-**************************************************************************************/

int est_dans_un_ligne(int joueur, int x, int y)
{
    int valide = 1;
    for (int i = 0; i < 3; i++)
        if (Gril_jeu[i][y] != joueur)
            valide = 0;
    if (valide == 0)
    {
        valide = 1;
        for (int i = 0; i < 3; i++)
            if (Gril_jeu[x][i] != joueur)
                valide = 0;
    }
    if (x == y && valide == 0)
    {
        valide = 1;
        for (int i = 0; i < 3; i++)
            if (Gril_jeu[i][i] != joueur)
                valide = 0;
    }

    return valide;
}

int qui_est_gagnant()
{
    /** A compl�ter
        analyser le contenu de la gril pour savoir quel joueur
        est arriv� a aligner ses trois pi�ces
    **/

    for (int x = 0; x < 3; x++)
    {
        for (int y = 0; y < 3; y++)
        {
            if (est_dans_un_ligne(Gril_jeu[x][y], x, y) == 1)
                return Gril_jeu[x][y];
        }
    }
    return 0;
}

/*-**************************************************************************************/

int initialisation_du_jeu()
{
    /* phase d'attente de l'�tablissemment de connextion du premier joueur  */
    printf("Attendre la connexion du premier joueur ... \n\n");
    int etat_de_connexion = connexion_d_un_client(socketEcoute, &socketDialogue1);
    if (etat_de_connexion == 0)
        return 0; /* quiter l'ex�cution car il y a un probl�me de connexion */
    /* dans le cas de connexion r�ussite d'un client - 1er joueur s'est connect�
       alors on attend l'arriv� d'un message portant le nom de ce premier joueur    */
    printf("attendre le message portant le nom du 1er joueur ... \n");
    int etat_reception = recevoir_message_de_client(socketDialogue1, nomJ1);
    if (etat_reception == 0)
        return 0; /* il y a une erreur lors de la reception du message */

    /* dans le cas o� il y a une reception correcte d'un message portant le nom du 1er joueur
       alors, le serveur r�pond au client par un message demandant au 1er joueur
       d'attendre l'arriv� du 2eme joueur, ce message porte le code "1" */
    printf("envoyer un message au 1er joueur indicant qu'il est le premier... \n "); //  Tu va attendre l'arrive de ton Adversaaire ...\n
    int etat_envoi = envoyer_message_au_client(socketDialogue1, ATTEND_ADVERSAIRE);
    if (etat_envoi == 0)
        return 0; /* quitter l'ex�cution car il y a une erreur lors de l'envoi du message */

    /* phase d'attente de l'�tablissemment de connextion du deuxi�me joueur  */
    printf("attendre la connexion du 2eme joueur ... \n");
    etat_de_connexion = connexion_d_un_client(socketEcoute, &socketDialogue2);
    if (etat_de_connexion == 0)
        return 0; /* quiter l'ex�cution car il y a un probl�me de connexion */

    /* dans le cas de connexion r�ussite d'un autre client - 2er joueur s'est connect�
       alors on attent l'arriv� d'un message portant le nom de ce deuxi�me joueur -   */
    printf("attente le message portant le nom du 2eme joueur ... \n");
    etat_reception = recevoir_message_de_client(socketDialogue2, nomJ2);
    if (etat_reception == 0)
        return 0; /* il y a une erreur lors de la reception du message */
    /* dans le cas o� il y a une reception correcte du message portant le nom du joueur 2
       alors, le serveur r�pond au client par un mmessage d'accueil   */
    printf("Envoi d'un message au 2eme joueur portant le nom du 1er joueur... \n");
    etat_envoi = envoyer_message_au_client(socketDialogue2, nomJ1);
    if (etat_envoi == 0)
        return 0; /* quitter l'ex�cution car il y a une erreur lors de l'envoi du message */

    printf("Envoi d'un message au 1er joueur portant le nom du 2eme joueur... \n");
    etat_envoi = envoyer_message_au_client(socketDialogue1, nomJ2);
    if (etat_envoi == 0)
        return 0; /* quitter l'ex�cution car il y a une erreur lors de l'envoi du message */
    return 1;
}

/*-**************************************************************************************/

int placement_une_piece(SOCKET socketDialogue, int joueur)
{
    /** A compl�ter
        contr�ler le placeent correcte d'une pi�ce d'un joueur d�termmin� (le 1er ou le 2eme

        ce serveur
        - donne l'ordre � un joueur pour placer un pi�ce dans la gril
        - reception de l'action de jeu de ce joueur
        - la confirmer que cette action (de placement) du joueur est valide ou nom

        retourne :
        0 : en cas de probl�me de communication
        1 : si le joueur arrive (apres avoir anlyser son action de jeu) � bien positionner sa pi�ce
        2 : si le joueur n'arrive pas (apres avoir anlyser son action de jeu) � bien positionner sa
            pi�ce ou si il n'arrive pas � exprimmer en trois tentive une placement correcte.
    **/
    int x, y, tentive = 0;

    etat_de_connexion = envoyer_message_au_client(socketDialogue, VEILLEZ_JOUER);
    if (etat_de_connexion == 0)
        return 0;
    etat_de_connexion = recevoir_message_de_client(socketDialogue, derniere_action_jeu);
    if (etat_de_connexion == 0)
        return 0;
    extraire_place(derniere_action_jeu, &x, &y);
    int met_a_jour = mettre_a_jeur_gril(joueur, x, y);
    tentive++;
    if (met_a_jour == 1)
    {
        etat_de_connexion = envoyer_message_au_client(socketDialogue, MANIPULATION_OK);
        if (etat_de_connexion == 0)
            return 0;
    }
    else
    {
        while (met_a_jour != 1 && tentive < 3)
        {
            etat_de_connexion = envoyer_message_au_client(socketDialogue, MAUVAISE_MANIPULATION);
            if (etat_de_connexion == 0)
                return 0;
            etat_de_connexion = recevoir_message_de_client(socketDialogue, derniere_action_jeu);
            if (etat_de_connexion == 0)
                return 0;
            extraire_place(derniere_action_jeu, &x, &y);
            met_a_jour = mettre_a_jeur_gril(joueur, x, y);
            tentive++;
        }
        if (met_a_jour == 1)
        {
            etat_de_connexion = envoyer_message_au_client(socketDialogue, MANIPULATION_OK);
            if (etat_de_connexion == 0)
                return 0;
        }
        else if (tentive == 3)
        {
            etat_de_connexion = envoyer_message_au_client(socketDialogue, ECHEC);
            if (etat_de_connexion == 0)
                return 0;
            return 2;
        }
    }
    return 1;
}

/*-**************************************************************************************/

int placement_des_pieces()
{

    /** A compl�ter
     ici c'est la phase qui permet au serveur de controler et gerer le placement correcte
     des (trois) pi�ces des deux joueurs � l'alternat.

     le serveur utilise pour cela utilise la matrice representant la gril global. il
     - la valide l'action de chaque joueur
     - la reconnaissance de vinqueur apr�s chaque action de chaque joueur

     retourne :
       0 : en cas de probl�me de communication
       1 : si les deux joueur arrivent � bien positionner leurs 3 pi�ces
       2 : un joueur arrive � placer ces 3 pi�ces align�es (il est d�clar�
           vinqueur et son adversaire non) ou un joueur n'arrive pas � bien
           placer une pi�ce parmis les trois en trois tentive (son adversaire
           est dans ce cas d�clar� vinqueur)
     **/

    int nb_pieces_placee_J1 = 0, nb_pieces_placee_J2 = 0;

    int joueur = 1, etat_jeu;
    while (nb_pieces_placee_J1 != 3 || nb_pieces_placee_J2 != 3)
    {
        printf("En attente du tour de joueur %d. \n", joueur);
        if (joueur == 1)
            etat_jeu = placement_une_piece(socketDialogue1, joueur);
        else
            etat_jeu = placement_une_piece(socketDialogue2, joueur);
        if (etat_jeu == 0)
        {
            if (joueur == 1)
                etat_de_connexion = envoyer_message_au_client(socketDialogue2, ADVERSAIRE_DECONNECTE);
            else if (joueur == 2)
                etat_de_connexion = envoyer_message_au_client(socketDialogue1, ADVERSAIRE_DECONNECTE);
            return 0;
        }
        if (etat_jeu == 2)
        {
            if (joueur == 1)
                etat_de_connexion = envoyer_message_au_client(socketDialogue1, TU_A_GAGNE);
            else
                etat_de_connexion = envoyer_message_au_client(socketDialogue2, TU_A_GAGNE);
            if (etat_de_connexion == 0)
                return 0;

            return 2;
        }

        if (joueur == 1)
        {
            nb_pieces_placee_J1++;
            etat_de_connexion = envoyer_message_au_client(socketDialogue2, derniere_action_jeu);
            if (etat_de_connexion == 0)
            {
                etat_de_connexion = envoyer_message_au_client(socketDialogue1, ADVERSAIRE_DECONNECTE);
                return 0;
            }
            joueur = 2;
            if (nb_pieces_placee_J1 == 3)
            {
                if (qui_est_gagnant() == 1)
                {
                    etat_de_connexion = envoyer_message_au_client(socketDialogue1, TU_A_GAGNE);
                    if (etat_de_connexion == 0)
                        return 0;
                    etat_de_connexion = envoyer_message_au_client(socketDialogue2, ECHEC);
                    if (etat_de_connexion == 0)
                        return 0;
                    return 2;
                }
            }
        }
        else
        {
            nb_pieces_placee_J2++;
            joueur = 1;
            etat_de_connexion = envoyer_message_au_client(socketDialogue1, derniere_action_jeu);
            if (etat_de_connexion == 0)
            {
                etat_de_connexion = envoyer_message_au_client(socketDialogue2, ADVERSAIRE_DECONNECTE);
                return 0;
            }
            if (nb_pieces_placee_J2 == 3)
            {
                if (qui_est_gagnant() == 2)
                {
                    etat_de_connexion = envoyer_message_au_client(socketDialogue2, TU_A_GAGNE);
                    if (etat_de_connexion == 0)
                        return 0;
                    etat_de_connexion = envoyer_message_au_client(socketDialogue1, ECHEC);
                    if (etat_de_connexion == 0)
                        return 0;
                    return 2;
                }
            }
        }
    }
    return 1;
}

/*-**************************************************************************************/

int deplacement_une_piece(SOCKET socketDialogue, int joueur)
{
    /** A compl�ter
      ici c'est la phase qui permet au serveur de controler et gerer le deplacement correcte
      d'une (seule) pi�ce des deux joueurs � l'alternat.

      le serveur utilise pour cela la matrice representant la gril global. il
      - donne l'ordre � un joueur pour deplacer un pi�ce dans la gril
      - reception de l'action de deplacement d'un joueur d�termin� et la valide
      - reconnait le joueur qui n'arrive pas faire un d�placement correcte dans 3 tentatives
      - la confirmer que cette action (de deplacement) du joueur est valide ou nom

      retourne :
        0 : en cas de probl�me de communication
        1 : si le joueur arrive (apres avoir anlyser son action de jeu) � bien deplacer sa pi�ce
        2 : si le joueur n'arrive pas (apres avoir anlyser son action de jeu) � bien deplacer sa
            pi�ce ou si il n'arrive pas � exprimmer en trois tentive un deplacement correcte.
      **/
    int xs, ys, xc, yc, tentive = 0;

    etat_de_connexion = envoyer_message_au_client(socketDialogue, VEILLEZ_JOUER);
    if (etat_de_connexion == 0)
        return 0;
    etat_de_connexion = recevoir_message_de_client(socketDialogue, derniere_action_jeu);
    if (etat_de_connexion == 0)
        return 0;
    extraire_deplacement(derniere_action_jeu, &xs, &ys, &xc, &yc);
    int met_a_jour = mettre_a_jeur_gril(joueur, xs, ys, xc, yc);
    tentive++;
    if (met_a_jour == 1)
    {
        etat_de_connexion = envoyer_message_au_client(socketDialogue, MANIPULATION_OK);
        if (etat_de_connexion == 0)
            return 0;
    }
    else
    {
        while (met_a_jour != 1 && tentive < 3)
        {
            etat_de_connexion = envoyer_message_au_client(socketDialogue, MAUVAISE_MANIPULATION);
            if (etat_de_connexion == 0)
                return 0;
            etat_de_connexion = recevoir_message_de_client(socketDialogue, derniere_action_jeu);
            if (etat_de_connexion == 0)
                return 0;
            extraire_deplacement(derniere_action_jeu, &xs, &ys, &xc, &yc);
            met_a_jour = mettre_a_jeur_gril(joueur, xs, ys, xc, yc);
            tentive++;
        }
        if (met_a_jour == 1)
        {
            etat_de_connexion = envoyer_message_au_client(socketDialogue, MANIPULATION_OK);
            if (etat_de_connexion == 0)
                return 0;
        }
        else if (tentive == 3)
        {
            etat_de_connexion = envoyer_message_au_client(socketDialogue, ECHEC);
            if (etat_de_connexion == 0)
                return 0;
            return 2;
        }
    }
    return 1;
}

/*-**************************************************************************************/

int deplacement_des_pieces()
{
    /** A compl�ter
     ici c'est la phase qui permet au serveur de controler et gerer le deplacement correcte
     des pi�ces des deux joueurs � l'alternat.

     le serveur utilise pour cela utilise la matrice representant la gril global. il
     - la valide l'action de chaque joueur
     - la reconnaissance de vinqueur apr�s chaque action de chaque joueur

     retourne :
       0 : en cas de probl�me de communication
       1 : cas d'un vinqueur - si un des deux joueurs arrive � bien aligner ses 3 pi�ces(il est d�clar�
           vinqueur et son adversaire non) ou un joueur n'arrive pas � bien
           deplacer une pi�ce en trois tentive (son adversaire
           est dans ce cas d�clar� vinqueur)
     **/
    int etat_jeu, joueur = 1;
    int est_gagnat = 0;

    while (est_gagnat == 0)
    {
        if (joueur == 1)
            etat_jeu = deplacement_une_piece(socketDialogue1, joueur);
        else
            etat_jeu = deplacement_une_piece(socketDialogue2, joueur);
        if (etat_jeu == 0)
        {
            if (joueur == 1)
                etat_de_connexion = envoyer_message_au_client(socketDialogue2, ADVERSAIRE_DECONNECTE);
            else if (joueur == 2)
                etat_de_connexion = envoyer_message_au_client(socketDialogue1, ADVERSAIRE_DECONNECTE);
            return 0;
        }
        if (etat_jeu == 2)
        {
            if (joueur == 1)
                etat_de_connexion = envoyer_message_au_client(socketDialogue1, TU_A_GAGNE);
            else
                etat_de_connexion = envoyer_message_au_client(socketDialogue2, TU_A_GAGNE);
            if (etat_de_connexion == 0)
                return 0;

            return 1;
        }

        if (joueur == 1)
        {
            etat_de_connexion = envoyer_message_au_client(socketDialogue2, derniere_action_jeu);
            if (etat_de_connexion == 0)
            {
                etat_de_connexion = envoyer_message_au_client(socketDialogue1, ADVERSAIRE_DECONNECTE);
                return 0;
            }
            joueur = 2;
        }
        else
        {
            etat_de_connexion = envoyer_message_au_client(socketDialogue1, derniere_action_jeu);
            if (etat_de_connexion == 0)
            {
                etat_de_connexion = envoyer_message_au_client(socketDialogue2, ADVERSAIRE_DECONNECTE);
                return 0;
            }
            joueur = 1;
        }

        est_gagnat = qui_est_gagnant();
    }
    if (est_gagnat == 1)
    {
        etat_de_connexion = envoyer_message_au_client(socketDialogue1, TU_A_GAGNE);
        if (etat_de_connexion == 0)
            return 0;
        etat_de_connexion = envoyer_message_au_client(socketDialogue2, ECHEC);
        if (etat_de_connexion == 0)
            return 0;
    }
    else
    {
        etat_de_connexion = envoyer_message_au_client(socketDialogue2, TU_A_GAGNE);
        if (etat_de_connexion == 0)
            return 0;
        etat_de_connexion = envoyer_message_au_client(socketDialogue1, ECHEC);
        if (etat_de_connexion == 0)
            return 0;
    }
    return 1;
}

/*-**************************************************************************************/

int gerer_un_jeu()
{
    if (initialisation_du_jeu() == 0)
        return 0;
    initialiser_gril();

    /* phase de placement des pi�ces */
    int termminer = placement_des_pieces();
    if (termminer != 1)
        return 0;

    /* phase de d�placement des pi�ces */
    deplacement_des_pieces();
    /* se d�connecter  */
    fermeture_de_socket_dialog(socketDialogue1);
    fermeture_de_socket_dialog(socketDialogue2);
    return 1;
}

/**************************************************************************************/

int main()
{
    /* initilaiser et lancer l�utilisation de la iliotheque de communication  */
    init_bib();
    /* attacher le serveur avec la couche de transport   */
    int creation_reussite = creation_et_configuration_socket_eccoute(&socketEcoute);
    if (creation_reussite == 0)
        return 0; /* quiter l'ex�cution car il y a un probl�me de cr�ation de socket d'�coute */
    while (true)
        gerer_un_jeu();
    /* termine l�utilisation de la iliotheque de communication  */
    fermeture_de_socket_Ecoute(socketEcoute);
    fermer_bib();
    getchar();
}

/**************************************************************************************/
