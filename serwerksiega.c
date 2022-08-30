/*Wersja projektu B*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <unistd.h>
#include <sys/sem.h>
#include <string.h>
#include <math.h>

#define MY_MSG_SIZE 80

key_t key;
int shmid;
struct my_data
{
    int pid;                    // 4b przechowuje pid serwera
    int ile_jest_wpisow;        // 4b
    int maks_wpisow;            // 4b maksymalna ilosc wpisow podana przez uzytkownika
    char txt[MY_MSG_SIZE];      // 80b tekst wpisywany przez uzytkownika klienta do pamieci wspoldzielonej
    char nazwa_uzytkownika[36]; // 36b nazwa uzytkownika podana przez uzytkownika klienta
} * shared_data;                // razem 128b

union semun
{
    int val;
    struct semid_ds *buf;
    ushort *array;
} arg;

int semid;
struct sembuf sb;

void sgnhandle(int signal)
{
    printf("\n[Serwer]: dostalem SIGINT => koncze i sprzatam...");
    printf(" (odlaczenie: %s, usuniecie pamieci: %s, usuniecie semafora: %s)\n",
           (shmdt(shared_data) == 0) ? "OK" : "blad shmdt",
           (shmctl(shmid, IPC_RMID, 0) == 0) ? "OK" : "blad shmctl",
           (semctl(semid, 0, IPC_RMID, 0) == 0) ? "OK" : "blad semctl");

    exit(0);
}

void sgnhandle2(int signal)
{
    if (shared_data[0].ile_jest_wpisow == 0)
        printf("\nKsiega skarg i wnioskow jest jeszcze pusta\n");
    else
    {
        printf("\n_______Ksiega skarg i wnioskow:_______\n");
        int i;
        for (i = 0; i < shared_data[0].ile_jest_wpisow; i++)
            printf("[%s]: %s\n", shared_data[i].nazwa_uzytkownika, shared_data[i].txt);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Nieprawidlowa liczba argumentow: ./serwer [nazwa_pliku] [rozmiar_ksiegi]\n");
        exit(0);
    }

    signal(SIGINT, sgnhandle);
    signal(SIGTSTP, sgnhandle2);

    int m_wpisow = atoi(argv[2]); // zmienna pomocnicza - przechowuje maksymalna ilosc wpisow podana przez uzytkownika

    /*--------------------------------------------------------------------------------------*/
    printf("[Serwer]: tworze klucz na podstawie pliku %s...", argv[1]);
    if ((key = ftok(argv[1], 1)) == -1)
    {
        perror("ftok");
        exit(1);
    }
    printf(" OK (klucz: %d)\n", key);

    /*-------------------inicjalizacja semafora-----------------------*/
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = 0;

    /*tworze semafor*/
    if ((semid = semget(key, 1, 0666 | IPC_CREAT)) == -1)
    {
        perror("semget");
        exit(1);
    }

    /*przypisuje wartosc dla semafora*/
    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1)
    {
        perror("semctl");
        exit(1);
    }
    /*---------------------------------------------------------------*/

    struct shmid_ds buf;

    printf("[Serwer]: tworze segment pamieci wspolnej dla ksiegi na %d wpisow po %ldb...\n", m_wpisow, sizeof(struct my_data));
    if ((shmid = shmget(key, sizeof(struct my_data) * m_wpisow, 0600 | IPC_CREAT | IPC_EXCL)) == -1)
    {
        printf(" blad shmget!\n");
        exit(1);
    }

    if ((shmctl(shmid, IPC_STAT, &buf)) == -1)
    {
        perror("shmctl");
        exit(1);
    }
    printf("OK (id: %d, rozmiar: %zub)\n", shmid, buf.shm_segsz);

    printf("[Serwer]: dolaczam pamiec wspolna...");
    shared_data = (struct my_data *)shmat(shmid, (void *)0, 0);

    if (shared_data == (struct my_data *)-1)
    {
        printf(" blad shmat!\n");
        exit(1);
    }
    printf(" OK (adres: %lX)\n", (long int)shared_data);

    /*przypisanie maksymalnej ilosci wpisow podanej przez uzytkownika do pola w strukturze pamieci wspoldzielonej*/
    shared_data[0].maks_wpisow = m_wpisow;

    /*przypisanie pidu serwera do pola w strukturze pamieci wspoldzielonej*/
    shared_data[0].pid = getpid();

    printf("[Serwer]: nacisnij Ctrl^Z by wyswietlic stan ksiegi\n");
    printf("[Serwer]: nacisnij Ctrl^C by zakonczyc program\n");
    shared_data[0].ile_jest_wpisow = 0; // pamiec wspoldzielona jest jeszcze pusta

    while (1)
    {
        /*blokuje semafor*/
        sb.sem_op = -1;
        if (semop(semid, &sb, 1) == -1)
        {
            perror("semop");
            exit(1);
        }

        /*odblokowuje semafor*/
        sb.sem_op = 1;
        if (semop(semid, &sb, 1) == -1)
        {
            perror("semop");
            exit(1);
        }

        sleep(1);
    }

    return 0;
}