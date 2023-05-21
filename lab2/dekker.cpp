#include <signal.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include<iostream>
#include<atomic>
using namespace std;

int shmid;

typedef struct zajednicke
{
    int a;
    atomic<int> PRAVO;
    atomic<int> ZASTAVICA[2];

}zajednicke;

zajednicke *buf;


//Dekkerov algoritam

//funkcija uđi u kritični odsječak
void ulaz(int ja, int proces2)
{
    buf->ZASTAVICA[ja] = 1;
    while(buf->ZASTAVICA[proces2] != 0){
        if(buf->PRAVO == proces2){
            buf->ZASTAVICA[ja] = 0;
            while(buf->PRAVO == proces2);
            buf->ZASTAVICA[ja]=1;
        }
    }
}

//funkcija izađi iz kritičnog odsječka
void izlaz(int ja, int proces2)
{
    buf->PRAVO = proces2;
    buf->ZASTAVICA[ja] = 0;
}


//dijete -> 0
//roditelj -> 1
void proces_roditelj(int M)
{
    cout << "U roditelju sam, PID = " << getpid() << endl;
    for(int i = 0; i < M; i++){
        ulaz(1,0);
        (buf->a)++;
        izlaz(1,0);
    }
}

void proces_dijete(int M)
{
    cout << "U djetetu sam, PID = " << getpid() << endl;
    for(int i = 0; i < M; i++){
        ulaz(0,1);
        (buf->a)++;
        izlaz(0,1);
    }
}


int main(int argc, char *argv[])
//argc -> broj argumenata u terminalu
//argv -> u to polje se spremaju ti argumenti
{

    cout << "Krećemo s radom, PID = " << getpid() << endl;

    if(argc < 2){
        cout << "Unesen nedovoljan broj argumenata" << endl;
        exit(1);
    }

    int M = atoi (argv[1]);
    cout << "M je " << M << endl;

    shmid = shmget (IPC_PRIVATE, sizeof(int), 0600); //zauzimanje zajedničke memorije
    if(shmid == -1) exit(1); //greška -> nema zajedničke memorije


    buf = (zajednicke *)shmat(shmid, NULL, 0);
    buf->a = 0;
    buf->PRAVO = 0;
    for(int i = 0; i < 2; i++){
        buf->ZASTAVICA[i] = 0;
    }


    //forkanje
    switch(fork()){
        case 0:
            proces_dijete(M);
            exit(0);
        case -1:
            cout << "Neuspjelo forkanje";
        default:
            proces_roditelj(M);
    }
    wait(NULL);

    cout << "Varijabla a na kraju je " << buf->a << endl;

    shmdt (buf);
	shmctl (shmid, IPC_RMID, NULL);
 
   return 0;
}