#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

struct msg
{
    char nazwa[30];
    char wiadomosc[20];
    char ruch[20];
    int punkty;
    int czy_polaczony;
};

struct addrinfo hints;
struct addrinfo *result, *rp, *opponent;
struct msg my_msg, opponent_msg;
char ip[16];
char *nr_portu = "7815";
int sockfd;

char plansza[10];
char nowaGra = 't';
int koniec, kolejnosc_graczy, pierwszy_ruch;
char znak, znak_przeciwnika;

/* W razie wyslania sygnalu SIGINT (CTRL + C) "sprzatamy" */
void sgnhandle(int signal)
{
    freeaddrinfo(result);
    freeaddrinfo(rp);
    freeaddrinfo(opponent);
    close(sockfd);
    exit(0);
}

void czystaPlansza()
{
    strcpy(my_msg.ruch, "");
    strcpy(opponent_msg.ruch, "");
    int i;
    for (i = 1; i < 10; i++)
    {
        plansza[i] = (char)(i + 96);
    }
}

void wypiszPlansze()
{
    printf("     |     |     \n");
    printf("  %c  |  %c  |  %c \n", plansza[1], plansza[2], plansza[3]);

    printf("_____|_____|_____\n");
    printf("     |     |     \n");

    printf("  %c  |  %c  |  %c \n", plansza[4], plansza[5], plansza[6]);

    printf("_____|_____|_____\n");
    printf("     |     |     \n");

    printf("  %c  |  %c  |  %c \n", plansza[7], plansza[8], plansza[9]);

    printf("     |     |     \n\n");
}

/* Funkcja odpowiadajaca za sprawdzenie czy ktorys z graczy wygral lub czy jest remis */
int czyWygrana()
{
    int czy_wygrana = 0, czy_remis = 0;

    if (plansza[1] == plansza[2] && plansza[2] == plansza[3])
        czy_wygrana++;

    else if (plansza[4] == plansza[5] && plansza[5] == plansza[6])
        czy_wygrana++;

    else if (plansza[7] == plansza[8] && plansza[8] == plansza[9])
        czy_wygrana++;

    else if (plansza[1] == plansza[4] && plansza[4] == plansza[7])
        czy_wygrana++;

    else if (plansza[2] == plansza[5] && plansza[5] == plansza[8])
        czy_wygrana++;

    else if (plansza[3] == plansza[6] && plansza[6] == plansza[9])
        czy_wygrana++;

    else if (plansza[1] == plansza[5] && plansza[5] == plansza[9])
        czy_wygrana++;

    else if (plansza[3] == plansza[5] && plansza[5] == plansza[7])
        czy_wygrana++;

    else if (plansza[1] != 'a' && plansza[2] != 'b' && plansza[3] != 'c' &&
             plansza[4] != 'd' && plansza[5] != 'e' && plansza[6] != 'f' &&
             plansza[7] != 'g' && plansza[8] != 'h' && plansza[9] != 'i')
        czy_remis++;

    if (czy_remis == 1)
        czy_wygrana = 2;

    return czy_wygrana;
}

void punktacja()
{
    printf("Ty %i : %i %s\n", my_msg.punkty, opponent_msg.punkty, opponent_msg.nazwa);
}

/* Funkcja odpowiadajaca za wyslanie wiadomosci do przeciwnika */
void wyslijWiadomosc()
{
    ssize_t bytes;
    bytes = sendto(sockfd, &my_msg, sizeof(my_msg), 0, opponent->ai_addr, opponent->ai_addrlen);
    if (bytes < 0)
    {
        perror("Sendto");
        exit(EXIT_FAILURE);
    }
}

/* Funkcja odpowiadajaca za wykonanie ruchu i sprawdzenie jego poprawnosci*/
void ruch()
{
tura:
    printf("[wybierz pole] ");
    scanf("%s", my_msg.ruch);

    /*Zaznaczenie pola wybranego przez gracza*/
    if (my_msg.ruch[0] == 'a' && plansza[1] == 'a')
        plansza[1] = znak;
    else if (my_msg.ruch[0] == 'b' && plansza[2] == 'b')
        plansza[2] = znak;
    else if (my_msg.ruch[0] == 'c' && plansza[3] == 'c')
        plansza[3] = znak;
    else if (my_msg.ruch[0] == 'd' && plansza[4] == 'd')
        plansza[4] = znak;
    else if (my_msg.ruch[0] == 'e' && plansza[5] == 'e')
        plansza[5] = znak;
    else if (my_msg.ruch[0] == 'f' && plansza[6] == 'f')
        plansza[6] = znak;
    else if (my_msg.ruch[0] == 'g' && plansza[7] == 'g')
        plansza[7] = znak;
    else if (my_msg.ruch[0] == 'h' && plansza[8] == 'h')
        plansza[8] = znak;
    else if (my_msg.ruch[0] == 'i' && plansza[9] == 'i')
        plansza[9] = znak;

    /*Inne opcje ruchu gracza*/
    else if (strcmp(my_msg.ruch, "<koniec>") == 0)
    {
        strcpy(my_msg.wiadomosc, "KONIEC");
        wyslijWiadomosc();
        freeaddrinfo(result);
        close(sockfd);
        exit(0);
    }
    else if (strcmp(my_msg.ruch, "<wynik>") == 0)
    {
        punktacja();
        goto tura;
    }
    else
    {
        printf("To nie jest poprawny ruch!\n");
        goto tura;
    }
}

void wpiszRuchPrzeciwnika()
{
    if (opponent_msg.ruch[0] == 'a' && plansza[1] == 'a')
        plansza[1] = znak_przeciwnika;
    else if (opponent_msg.ruch[0] == 'b' && plansza[2] == 'b')
        plansza[2] = znak_przeciwnika;
    else if (opponent_msg.ruch[0] == 'c' && plansza[3] == 'c')
        plansza[3] = znak_przeciwnika;
    else if (opponent_msg.ruch[0] == 'd' && plansza[4] == 'd')
        plansza[4] = znak_przeciwnika;
    else if (opponent_msg.ruch[0] == 'e' && plansza[5] == 'e')
        plansza[5] = znak_przeciwnika;
    else if (opponent_msg.ruch[0] == 'f' && plansza[6] == 'f')
        plansza[6] = znak_przeciwnika;
    else if (opponent_msg.ruch[0] == 'g' && plansza[7] == 'g')
        plansza[7] = znak_przeciwnika;
    else if (opponent_msg.ruch[0] == 'h' && plansza[8] == 'h')
        plansza[8] = znak_przeciwnika;
    else if (opponent_msg.ruch[0] == 'i' && plansza[9] == 'i')
        plansza[9] = znak_przeciwnika;
}

/* Funkcja odpowiadajaca za odebranie wiadomosci */
void odbierzWiadomosc()
{
    recvfrom(sockfd, &opponent_msg, sizeof(opponent_msg), 0, NULL, NULL);

    if (strcmp(opponent_msg.wiadomosc, "KONIEC") == 0)
    {
        printf("[%s (%s) zakonczyl gre, mozesz poczekac na innego gracza]\n", opponent_msg.nazwa, ip);
        koniec = 1;
    }
    else
    {
        printf("[%s (%s) wybral pole %s]\n", opponent_msg.nazwa, ip, opponent_msg.ruch);
    }
}

/* Funkcja odpowiadajaca za uzyskanie adresu IPv4 oraz polaczenie sie z przeciwnikiem*/
void polacz(char *arg)
{
    kolejnosc_graczy = 0;
    my_msg.czy_polaczony = 0;
    opponent_msg.czy_polaczony = 0;

    int g1, g2;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    g1 = getaddrinfo(NULL, nr_portu, &hints, &result);

    hints.ai_flags = 0;

    g2 = getaddrinfo(arg, nr_portu, &hints, &opponent);

    if (g1 != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(g1));
        exit(EXIT_FAILURE);
    }

    if (g2 != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(g2));
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1)
            continue;

        if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;

        close(sockfd);
    }
    inet_ntop(AF_INET, &(((struct sockaddr_in *)opponent->ai_addr)->sin_addr), ip, 16);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sgnhandle);

    /* Obsluga podawanych danych*/
    if (argc == 3)
    {
        strcpy(my_msg.nazwa, argv[2]);
    }
    else if (argc == 2)
    {
        strcpy(my_msg.nazwa, "NN");
    }
    else
    {
        printf("Podaj adres domenowy lub adres IP\n");
        printf("Opcjonalnie drugi argument bedacy nazwa uzytkownika\n");
        exit(EXIT_FAILURE);
    }

    my_msg.punkty = 0;
    opponent_msg.punkty = 0;

    /* Glowna petla gry */
    while (nowaGra != 'n')
    {
        /* Zerowanie zmiennych */
        czystaPlansza();
        koniec = 0;

        /* Polaczenie sie z przeciwnikiem*/
        polacz(argv[1]);
        printf("Rozpoczynam gre z %s. Napisz <koniec> by zakonczyc.\n", ip);

        /* Wymiana sie danymi */
        my_msg.czy_polaczony = 1;
        strcpy(my_msg.wiadomosc, "NowaGra");

        ssize_t bytes;
        bytes = sendto(sockfd, &my_msg, sizeof(my_msg), 0, opponent->ai_addr, opponent->ai_addrlen);
        if (bytes < 0)
        {
            perror("Sendto");
            exit(EXIT_FAILURE);
        }
        else if (bytes > 0 && strcmp("NowaGra", my_msg.wiadomosc) == 0)
        {
            printf("[Propozycja gry wyslana]\n");
        }
        recvfrom(sockfd, &opponent_msg, sizeof(opponent_msg), 0, NULL, NULL);

        if (opponent_msg.czy_polaczony == 1)
        {
            printf("[%s (%s) dolaczyl do gry]\n", opponent_msg.nazwa, ip);
            kolejnosc_graczy = 1;
            znak = 'O';
            znak_przeciwnika = 'X';
            pierwszy_ruch = 1;
        }
        else
        {
            znak = 'X';
            znak_przeciwnika = 'O';
        }
        my_msg.czy_polaczony = 0;
        wyslijWiadomosc();

        /* Petla w ktorej toczy sie gra */
        while (1)
        {
            if (kolejnosc_graczy == 1)
            {
                if (pierwszy_ruch == 1)
                {
                    recvfrom(sockfd, &opponent_msg, sizeof(opponent_msg), 0, NULL, NULL);
                    pierwszy_ruch = 0;
                }
                wypiszPlansze();
                if (czyWygrana() == 1)
                {
                    printf("[Przegrana! Zagraj jeszcze raz]\n");
                    break;
                }
                else if (czyWygrana() == 2)
                {
                    printf("[Remis! Zagraj jeszcze raz]\n");
                    break;
                }
            }
            else
            {
                odbierzWiadomosc();
                if (koniec == 1)
                    break;
                wpiszRuchPrzeciwnika();
                wypiszPlansze();
                if (czyWygrana() == 1)
                {
                    printf("[Przegrana! Zagraj jeszcze raz]\n");
                    break;
                }
                else if (czyWygrana() == 2)
                {
                    printf("[Remis! Zagraj jeszcze raz]\n");
                    break;
                }
            }

            ruch();
            if (czyWygrana() == 1)
            {
                printf("[Wygrana! Kolejna rozgrywka, poczekaj na swoja kolej]\n");
                my_msg.punkty++;
            }
            else if (czyWygrana() == 2)
            {
                printf("[Remis! Zagraj jeszcze raz]\n");
            }
            wyslijWiadomosc();
            if (czyWygrana() >= 1)
                break;

            if (kolejnosc_graczy == 1)
            {
                odbierzWiadomosc();
                if (koniec == 1)
                    break;
                wpiszRuchPrzeciwnika();
            }
        }

        /* Okreslamy czy chcemy zagrac jeszcze raz */
        printf("Czy chcesz zagrac jeszcza raz? [t/n] ");
        do
        {
            scanf("%c", &nowaGra);
        } while (nowaGra != 't' && nowaGra != 'n');

        /* Wyczyszczenie danych */
        freeaddrinfo(result);
        close(sockfd);
    }

    return 0;
}