#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

/*funkcja sprawdzajaca i zwracajaca uprawnienia do pliku*/
char *permissions(const char *file)
{
    struct stat sb;
    lstat(file, &sb);

    char *perms = (char *)malloc(sizeof(char) * 10);

    /*dla wlasciciela*/
    if ((sb.st_mode & S_IRWXU) == S_IRWXU)
        strcat(perms, "rwx");
    else
    {
        if ((sb.st_mode & S_IRUSR) == S_IRUSR)
            strcat(perms, "r");
        else
            strcat(perms, "-");

        if ((sb.st_mode & S_IWUSR) == S_IWUSR)
            strcat(perms, "w");
        else
            strcat(perms, "-");

        if ((sb.st_mode & S_IXUSR) == S_IXUSR)
            strcat(perms, "x");
        else
            strcat(perms, "-");
    }

    /*dla grupy*/
    if ((sb.st_mode & S_IRWXG) == S_IRWXG)
        strcat(perms, "rwx");
    else
    {
        if ((sb.st_mode & S_IRGRP) == S_IRGRP)
            strcat(perms, "r");
        else
            strcat(perms, "-");

        if ((sb.st_mode & S_IWGRP) == S_IWGRP)
            strcat(perms, "w");
        else
            strcat(perms, "-");

        if ((sb.st_mode & S_IXGRP) == S_IXGRP)
            strcat(perms, "x");
        else
            strcat(perms, "-");
    }

    /*dla innych*/
    if ((sb.st_mode & S_IRWXO) == S_IRWXO)
        strcat(perms, "rwx");
    else
    {
        if ((sb.st_mode & S_IROTH) == S_IROTH)
            strcat(perms, "r");
        else
            strcat(perms, "-");
        ;

        if ((sb.st_mode & S_IWOTH) == S_IWOTH)
            strcat(perms, "w");
        else
            strcat(perms, "-");

        if ((sb.st_mode & S_IXOTH) == S_IXOTH)
            strcat(perms, "x");
        else
            strcat(perms, "-");
    }
    return perms;
}

/*funkcja sprawdzajaca i zwracajaca typ pliku wykorzystana w tryb2*/
char *filetype_check2(const char *filename, int *default_file, int *symbolic_link)
{
    struct stat st;
    lstat(filename, &st);

    char *result = (char *)malloc(sizeof(char) * 20);

    switch (st.st_mode & S_IFMT)
    {
    case S_IFDIR:
        strcat(result, "katalog");
        break;
    case S_IFREG:
    {
        *default_file = 1;
        strcat(result, "zwykly plik");
        break;
    }
    case S_IFLNK:
    {
        *symbolic_link = 1;
        strcat(result, "link symboliczny");
        break;
    }
    }
    return result;
}

/*funkcja sprawdzajaca i zwracajaca typ pliku wykorzystana w tryb1*/
char filetype_check1(const char *filename, int *symbolic_link)
{
    struct stat st;
    lstat(filename, &st);

    char result;

    switch (st.st_mode & S_IFMT)
    {
    case S_IFDIR:
        result = 'd';
        break;
    case S_IFREG:
    {
        result = '-';
        break;
    }
    case S_IFLNK:
    {
        *symbolic_link = 1;
        result = 'l';
        break;
    }
    }
    return result;
}

void tryb2(const char *par)
{
    /*potrzebna struktura*/
    struct stat st;
    lstat(par, &st);

    /*zmienne pomocnicze tzw "flagi"*/
    int default_file = 0;
    int symbolic_link = 0;

    printf("Informacje o %s:\n", par);
    printf("%-13s %s\n", "Typ pliku:", filetype_check2(par, &default_file, &symbolic_link));

    printf("%-13s %s/%s\n", "Sciezka:", realpath(".", NULL), par);

    char *linkbuf = malloc(st.st_size + 1); /*pomocnicza zmienna do linku symbolicznego*/
    readlink(par, linkbuf, st.st_size + 1); /*funcja przypisujaca do linkbuf na co wskazuje (do czego sie odnosi) link symboliczny*/
    if (symbolic_link == 1)
        printf("%-13s %s/%s\n", "Wskazuje na:", realpath(".", NULL), linkbuf);

    printf("%-13s %ld", "Rozmiar: ", st.st_size);
    if (st.st_size <= 4 && st.st_size > 0)
        printf(" bajty\n");
    else
        printf(" bajtow\n");

    printf("%-13s %s\n", "Uprawnienia:", permissions(par));

    printf("%-25s %s", "Ostatnio uzywany:", ctime(&st.st_atime));
    printf("%-25s %s", "Ostatnio modyfikowany:", ctime(&st.st_mtime));
    printf("%-25s %s", "Ostatnio zmieniany stan:", ctime(&st.st_ctime));

    /*wypisanie pierwszych dwoch lini z pliku na wyjscie standardowe*/
    if (default_file == 1)
    {
        printf("Poczatek zawartosci:\n");

        char buf[256];

        FILE *file;
        file = fopen(par, "r");

        for (int k = 0; k < 2; k++)
        {
            fgets(buf, 256, file);
            fprintf(stdout, "%s", buf);
        }
        fclose(file);
    }
}

struct results
{
    char filetype;
    char *perms;
    int nlink;
    char *username;
    char *groupname;
    int filesize;
    int year;
    char *mon;
    int day;
    char *hour;
    char *min;
    int symbolic_links;
    char *filename;
};

/*funkcja sortujaca po nazwie pliku do qsort*/
int str_sort(const void *a, const void *b)
{
    struct results *p = (struct results *)a;
    struct results *q = (struct results *)b;

    return strcmp(p->filename, q->filename);
}

/*funkcje do obliczania maksymalnych szerekosci w odpowiednich kolumnach*/
int maks_strlen_of_filesize(struct results arr[], int i)
{
    char strparameter[i][50];
    snprintf(strparameter[0], 50, "%d", arr[0].filesize); //zamiana int na char*

    int maks = strlen(strparameter[0]);
    for (int j = 0; j < i; j++) //szukanie maksimum (w tym przypadku szerekosci kolumny)
    {
        snprintf(strparameter[j], 50, "%d", arr[j].filesize);

        if (maks < strlen(strparameter[j]))
            maks = strlen(strparameter[j]);
    }
    return maks;
}

int maks_strlen_of_nlink(struct results arr[], int i)
{
    char strparameter[i][50];
    snprintf(strparameter[0], 50, "%d", arr[0].nlink); //zamiana int na char*

    int maks = strlen(strparameter[0]);
    for (int j = 0; j < i; j++) //szukanie maksimum (w tym przypadku szerekosci kolumny)
    {
        snprintf(strparameter[j], 50, "%d", arr[j].nlink);

        if (maks < strlen(strparameter[j]))
            maks = strlen(strparameter[j]);
    }
    return maks;
}

int maks_strlen_of_username(struct results arr[], int i)
{
    int maks = strlen(arr[0].username);
    for (int j = 0; j < i; j++) //szukanie maksimum (w tym przypadku szerekosci kolumny)
    {
        if (maks < strlen(arr[j].username))
            maks = strlen(arr[j].username);
    }
    return maks;
}

int maks_strlen_of_groupname(struct results arr[], int i)
{
    int maks = strlen(arr[0].groupname);
    for (int j = 0; j < i; j++) //szukanie maksimum (w tym przypadku szerekosci kolumny)
    {
        if (maks < strlen(arr[j].groupname))
            maks = strlen(arr[j].groupname);
    }
    return maks;
}

void tryb1(const char *par)
{
    struct results arr[1000];      /*tablica typu struct przechowujaca dane (zastosowana glownie w celu posortowania po poszczegolnej kolumnie wynikowej)*/
    for (int i = 0; i < 1000; i++) /*do zaalokowania pamieci dla typow char * w struct results*/
    {
        arr[i].perms = (char *)malloc(sizeof(char) * 10);
        arr[i].username = (char *)malloc(sizeof(char) * 20);
        arr[i].groupname = (char *)malloc(sizeof(char) * 20);
        arr[i].mon = (char *)malloc(sizeof(char) * 10);
        arr[i].hour = (char *)malloc(sizeof(char) * 10);
        arr[i].min = (char *)malloc(sizeof(char) * 10);
        arr[i].filename = (char *)malloc(sizeof(char) * 20);
    }

    /*pobranie id uzytkownika i grupy*/
    uid_t id_u = getuid();
    gid_t id_g = getgid();

    /*potrzebne struktury*/
    struct passwd *pas;
    pas = getpwuid(id_u);

    struct group *gr;
    gr = getgrgid(id_g);

    DIR *dir;
    struct dirent *sd;

    dir = opendir(par);

    if (dir == NULL)
    {
        if (errno == ENOENT) /*jezeli sprawdzany katalog nie istnieje ("no such file od directory")*/
            strerror(errno);
        else
            perror("Nie mozna odczytac katalogu"); //w razie innego bledu

        exit(EXIT_FAILURE);
    }

    struct stat st;
    struct tm *t;
    int i = 0;             /*zmienna do iteracji po tablicy typu struct result*/
    int symbolic_link = 0; /*zmienna pomocnicza jako "flaga" do warunku sprawdzajacego czy jest to link symboliczny*/

    while ((sd = readdir(dir)) != NULL) /*petla przechodzi przez cala zawartosc katalogu*/
    {
        lstat(sd->d_name, &st);
        t = localtime(&st.st_atime);

        /*przypisywanie danych do tablicy typu struct results*/
        strcat(arr[i].filename, sd->d_name);

        symbolic_link = 0;
        arr[i].filetype = filetype_check1(sd->d_name, &symbolic_link);
        arr[i].symbolic_links = symbolic_link;

        arr[i].perms = permissions(sd->d_name);
        arr[i].nlink = st.st_nlink;
        arr[i].username = pas->pw_name;
        arr[i].groupname = gr->gr_name;
        arr[i].filesize = st.st_size;

        /*przypisywanie DATY do tablicy typu struct results*/
        arr[i].year = t->tm_year + 1900; /*rok*/
        if ((t->tm_mon + 1) < 10)        /*miesiac (jezeli jest mniejszy niz 10 dopisuje zero z przodu)*/
        {
            char strmon[10];
            snprintf(strmon, 10, "%d", t->tm_mon + 1); /*zamiana int na char* (w celu dopisania zera z przodu)*/
            strcat(arr[i].mon, "0");
            strcat(arr[i].mon, strmon);
        }
        else
        {
            char strmon[10];
            snprintf(strmon, 10, "%d", t->tm_mon + 1);
            strcat(arr[i].mon, strmon);
        }

        arr[i].day = t->tm_mday; /*dzien*/
        if (t->tm_hour < 10)     /*godzina*/
        {
            char strhour[10];
            snprintf(strhour, 10, "%d", t->tm_hour);
            strcat(arr[i].hour, "0");
            strcat(arr[i].hour, strhour);
        }
        else
        {
            char strhour[10];
            snprintf(strhour, 10, "%d", t->tm_hour);
            strcat(arr[i].hour, strhour);
        }

        if (t->tm_min < 10) /*minuta*/
        {
            char strmin[10];
            snprintf(strmin, 10, "%d", t->tm_min);
            strcat(arr[i].min, "0");
            strcat(arr[i].min, strmin);
        }
        else
        {
            char strmin[10];
            snprintf(strmin, 10, "%d", t->tm_min);
            strcat(arr[i].min, strmin);
        }

        i++;
    }

    qsort(arr, i, sizeof(struct results), str_sort); /*wywolanie funkcji sortujacej*/

    int print_year[i];
    for (int p = 0; p < i; p++)
    {
        print_year[p] = 0; /*ustawienie "flag" w tablicy na 0*/

        if ((arr[p].year) < 2021) /*warunek co wypisywac od ktorego roku*/
            print_year[p] = 1;
    }

    /*wywolanie funckji obliczajacych maksymalne szerekosci nastepujacych kolumn*/
    int filesize_width = maks_strlen_of_filesize(arr, i);
    int nlink_width = maks_strlen_of_nlink(arr, i);
    int username_width = maks_strlen_of_username(arr, i);
    int groupname_width = maks_strlen_of_groupname(arr, i);

    char *linkbuf; /*pomocnicza zmienna do linku symbolicznego*/

    /*wypisanie posortowanego wyniku*/
    for (int j = 0; j < i; j++)
    {
        if (print_year[j] == 1)
        {
            printf("%c%s %*d %*s %*s %*d  %d-%s-%d %s", arr[j].filetype, arr[j].perms, nlink_width, arr[j].nlink, username_width, arr[j].username, groupname_width, arr[j].groupname, filesize_width, arr[j].filesize, arr[j].year, arr[j].mon, arr[j].day, arr[j].filename);

            if (arr[j].symbolic_links == 1)
            {
                linkbuf = malloc(st.st_size + 1);
                readlink(arr[j].filename, linkbuf, st.st_size + 1);
                printf(" -> %s\n", linkbuf);
                free(linkbuf);
            }
            else
                printf("\n");
        }
        else
        {
            printf("%c%s %*d %*s %*s %*d %s-%d %s:%s %s", arr[j].filetype, arr[j].perms, nlink_width, arr[j].nlink, username_width, arr[j].username, groupname_width, arr[j].groupname, filesize_width, arr[j].filesize, arr[j].mon, arr[j].day, arr[j].hour, arr[j].min, arr[j].filename);

            if (arr[j].symbolic_links == 1)
            {
                linkbuf = malloc(st.st_size + 1);
                readlink(arr[j].filename, linkbuf, st.st_size + 1);
                printf(" -> %s\n", linkbuf);
                free(linkbuf);
            }
            else
                printf("\n");
        }
    }

    closedir(dir); /*zamkniecie katalogu*/

    for (int i = 0; i < 1000; i++) /*do zwolnienia pamieci z typow char * w struct results*/
    {
        free(arr[i].perms);
        free(arr[i].username);
        free(arr[i].groupname);
        free(arr[i].mon);
        free(arr[i].hour);
        free(arr[i].min);
        free(arr[i].filename);
    }
}

int main(int argc, const char *argv[])
{
    if (argc == 1) /*tryb1*/
        tryb1(".");
    else if (argc == 2) /*tryb2*/
    {
        if (access(argv[1], F_OK) == 0)
            tryb2(argv[1]);
        else
            printf("Plik nie istnieje lub nie masz do niego uprawnien\n");
    }
    else if (argc > 2) /*kontrola niewlasciwego uzycia programu*/
    {
        printf("Nieprawidlowa liczba argumentow\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}