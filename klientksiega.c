/*Wersja projektu B*/
#define _WITH_GETLINE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>

#define MY_MSG_SIZE 80

key_t key;
int shmid;
struct my_data
{
    int pid;
    int ile_jest_wpisow;
    int maks_wpisow;
    char txt[MY_MSG_SIZE];
    char nazwa_uzytkownika[36];
} * shared_data;

char *buf = NULL;
size_t bufsize = MY_MSG_SIZE;

union semun
{
    int val;
    struct semid_ds *buf;
    ushort *array;
} arg;

int semid;
struct sembuf sb;

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Nieprawidlowa liczba argumentow: ./klient [nazwa_pliku] [nazwa_uzytkownika]\n");
        exit(0);
    }

    /*tworze klucz dla pamieci wspoldzielonej i semafora*/
    if ((key = ftok(argv[1], 1)) == -1)
    {
        perror("ftok");
        exit(1);
    }

    /*-------------------inicjalizacja semafora-----------------------*/
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = 0;

    /*pobieram semafor od serwera*/
    if ((semid = semget(key, 1, 0)) == -1)
    {
        perror("semget");
        exit(1);
    }
    /*----------------------------------------------------------------*/

    /*otwieram segment pamieci wspoldzielonej*/
    if ((shmid = shmget(key, 0, 0)) == -1)
    {
        printf(" blad shmget\n");
        exit(1);
    }

    /*dolaczam pamiec wspolna*/
    shared_data = (struct my_data *)shmat(shmid, (void *)0, 0);

    if (shared_data == (struct my_data *)-1)
    {
        printf(" blad shmat!\n");
        exit(1);
    }

    printf("Klient ksiegi skarg i wnioskow wita!\n");

    /*blokuje semafor*/
    if (semop(semid, &sb, 1) == -1)
    {
        perror("semop");
        exit(1);
    }

    /*ilosc pozostalego miejsca w ksiedze*/
    if (shared_data[0].ile_jest_wpisow >= 0 && shared_data[0].ile_jest_wpisow < shared_data[0].maks_wpisow)
        printf("[Wolnych %d wpisow (na %d)]\n", shared_data[0].maks_wpisow - shared_data[0].ile_jest_wpisow, shared_data[0].maks_wpisow);
    else
    {
        printf("Brak wolnego miejsca w ksiedze. Koncze prace.\n");
        kill(shared_data[0].pid, SIGTSTP); // wyslanie sygnalu CTRL + Z do serwera
        kill(shared_data[0].pid, SIGINT);  // wyslanie sygnalu CTRL + C do serwera

        exit(0);
    }

    printf("Napisz co ci doskwiera:\n");
    getline(&buf, &bufsize, stdin);

    /*wpisuje do pamieci wspoldzielonej*/
    buf[strlen(buf) - 1] = '\0'; // usuwam koniec linii

    strcpy(shared_data[shared_data[0].ile_jest_wpisow].txt, buf);

    strcpy(shared_data[shared_data[0].ile_jest_wpisow].nazwa_uzytkownika, argv[2]);

    shared_data[0].ile_jest_wpisow++;

    printf("Dziekuje za dokonanie wpisu do ksiegi\n");

    /*odblokowuje semafor*/
    sb.sem_op = 1;
    if (semop(semid, &sb, 1) == -1)
    {
        perror("semop");
        exit(1);
    }

    /*odlaczam pamiec wspolna*/
    shmdt(shared_data);

    return 0;
}