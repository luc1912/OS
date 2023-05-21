#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <stack>
using namespace std;

// ispis kao i printf uz dodatak trenutnog vremena na pocetku
#define PRINTF(format, ...)            \
    do                                 \
    {                                  \
        vrijeme();                     \
        printf(format, ##__VA_ARGS__); \
    } while (0)


struct timespec t0; //vrijeme početka programa
int T_P; //tekući prioritet
int K_Z[3] = {0}; //kontrolne zastavice
int broj_sigintova = 0;
int broj_sigusr1 = 0;
int broj_sigtermova = 0;

//postavlja tenutno vrijeme u t0
void postavi_pocetno_vrijeme(){
    clock_gettime(CLOCK_REALTIME, &t0);
}

// dohvaca vrijeme proteklo od pokretanja programa
void vrijeme(void)
{
    struct timespec t;
    
    clock_gettime(CLOCK_REALTIME, &t);

    t.tv_sec -= t0.tv_sec;
    t.tv_nsec -= t0.tv_nsec;
    if (t.tv_nsec < 0)
    {
        t.tv_nsec += 1000000000;
        t.tv_sec--;
    }

    printf("%03ld.%03ld:\t", t.tv_sec, t.tv_nsec / 1000000);
}


//spava zadani broj sekundi
//ako se prekine signalom, kasnije nastavlja spavati neprospavano
void spavaj(time_t sekundi)
{
    struct timespec koliko;
    koliko.tv_sec = sekundi;
    koliko.tv_nsec = 0;

    while (nanosleep(&koliko, &koliko) == -1 && errno == EINTR)
        PRINTF("Bio prekinut, nastavljam\n");
}

stack<string> stog;
void printanje()
{
    PRINTF("K_Z = ");
    for(int i = 0; i < 3; i++){
        cout << K_Z[i];
    }
    cout << ", " << "T_P = " << T_P;
    cout << ", stog: ";
   if(!stog.empty()){
        for(stack<string> tmp = stog; !tmp.empty(); tmp.pop()){
            cout << tmp.top() << " ";
        }
   }else{
    cout << "-";
   }

   cout << endl;
   cout << endl;
}

// metode za obradu signala
void obradi_dogadjaj(int sig);
void obradi_sigterm(int sig);
void obradi_sigint(int sig);


int main()
{

    postavi_pocetno_vrijeme();

    //SIGINT -> prekid razine 3
    //SIGUSR1 -> prekid razine 2
    //SIGTERM -> prekid razine 1

    struct sigaction act;

    // maskiranje signala SIGINT
    act.sa_handler = obradi_sigint;
    sigaction(SIGINT, &act, NULL);

    //maskiranje signala SIGUSR1
    act.sa_handler = obradi_dogadjaj;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0; // naprednije mogucnosti preskocene
    sigaction(SIGUSR1, &act, NULL);

    // maskiranje signala SIGTERM
    act.sa_handler = obradi_sigterm;
    sigemptyset(&act.sa_mask);
    sigaction(SIGTERM, &act, NULL);


    PRINTF("Program s PID=%ld krenuo s radom\n", (long)getpid());
    printanje();



   while(1){
    sleep(0);
   }

    PRINTF("Program s PID=%ld zavrsio s radom\n", (long)getpid());
    printanje();

    return 0;
}


void obradi_sigint(int sig)
{
    broj_sigintova++;
    while(broj_sigintova > 0){
        K_Z[2] = 1;
        if (3 <= T_P){
            PRINTF("Dogodio se prekid razine 3 ali se on pamti i ne prosljeđuje procesoru\n");
            printanje();
            return;
        }
        PRINTF("SKLOP: Dogodio se prekid razine 3 i prosljeduje se procesoru\n");
        printanje();
        while(K_Z[2] != 0 && 3 > T_P){
            //obrada signala razine 3
            K_Z[2]--;
            T_P = 3;
            stog.push("3, reg[3]");
            PRINTF("Počela obrada razine 3\n");
            printanje();
            for(int i = 0; i < 9; i++){
                sleep(1); //za prekid razine 3 spavamo 9 sekundi
            }
            PRINTF("Završila obrada prekida razine 3\n");
            broj_sigintova--;
            stog.pop();
            if (!stog.empty()){
                string tmp = stog.top();
                T_P = tmp[0] - '0';
            }else{
                T_P = 0;
            }
            if(T_P == 0){
                PRINTF("Nastavlja se izvođenje glavnog programa\n");
                printanje();
                continue;
            }
            else if(T_P == 2){
                PRINTF("Nastavlja se obrada prekida razine 2\n");
                printanje();
                break;
            }else if(T_P == 1){
                PRINTF("Nastavlja se obrada prekida razine 1\n");
                printanje();
                break;
            }

        }
    }
    if (K_Z[0] == 1 && stog.empty() && K_Z[1] != 1){
        // znači da je neki signal razine 1 "na čekanju"
        broj_sigtermova--;
        obradi_sigterm(2);
    }else if(K_Z[1] == 1){
        broj_sigusr1--;
        obradi_dogadjaj(10);
    }
}


void obradi_dogadjaj(int sig) 
{
    broj_sigusr1++;
    while (broj_sigusr1 > 0) {
        K_Z[1] = 1;
        if (2 <= T_P) {
            PRINTF("Dogodio se prekid razine 2 ali se on pamti i ne prosljeđuje procesoru\n");
            printanje();
            return;
        }
        PRINTF("SKLOP: Dogodio se prekid razine 2 i prosljeduje se procesoru\n");
        printanje();
        while (K_Z[1] != 0 && 2 > T_P) {
            //obrada signala razine 2
            K_Z[1]--;
            T_P = 2;
            stog.push("2, reg[2]");
            PRINTF("Počela obrada razine 2\n");
            printanje();
            for (int i = 0; i < 6; i++) {
                sleep(1); //za prekid razine 2 spavamo 6 sekundi
            }
            PRINTF("Završila obrada prekida razine 2\n");
            broj_sigusr1--;
            stog.pop();
            if (!stog.empty()) {
                string tmp = stog.top();
                T_P = tmp[0] - '0';
            } else {
                T_P = 0;
            }
            if(T_P == 0){
                PRINTF("Nastavlja se izvođenje glavnog programa\n");
                printanje();
                continue;
            }
            else if (T_P == 1) {
                PRINTF("Nastavlja se obrada prekida razine 1\n");
                printanje();
                break;
            }
        }
    }
    if (K_Z[0] == 1) {
        // znači da je neki signal razine 1 "na čekanju"
        broj_sigtermova--;
        obradi_sigterm(2);
    }
}

void obradi_sigterm(int sig)
{
    broj_sigtermova++;
    while(broj_sigtermova > 0){
        K_Z[0] = 1;
        if (1 <= T_P){
            PRINTF("Dogodio se prekid razine 1 ali se on pamti i ne prosljeđuje procesoru\n");
            printanje();
            return;
        }
        PRINTF("SKLOP: Dogodio se prekid razine 1 i prosljeduje se procesoru\n");
        printanje();
        while(K_Z[0] != 0){
            //obrada signala razine 1
            K_Z[0]--;
            T_P = 1;
            stog.push("1, reg[1]");
            PRINTF("Počela obrada razine 1\n");
            printanje();
            for(int i = 0; i < 3; i++){
                sleep(1); //za prekid razine 1 spavamo 3 sekunde
            }
            PRINTF("Završila obrada prekida razine 1\n");
            broj_sigtermova--;
            stog.pop();
            if (!stog.empty()){
                string tmp = stog.top();
                T_P = tmp[0] - '0';
            }else{
                T_P = 0;
            }
            if(T_P == 0){
                PRINTF("Nastavlja se izvođenje glavnog programa\n");
                printanje();
                continue;
            }
        }
    }
}