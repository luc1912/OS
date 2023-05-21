#include<iostream>
#include<semaphore.h>
#include<unistd.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<cstdlib>
#include<ctime>
#include<signal.h>
#include<queue>
#include<atomic>

using namespace std;

int shmid;

sem_t *klijenti; //semafor kojim pamtimo broj klijenata u čekaonici
sem_t *radi_salon;
sem_t *frizerka_slobodna;
sem_t *dosao_klijent;
sem_t *redkl;
sem_t *ispis;

typedef struct zajednicke
{
    int broj_mjesta; //ukupan broj mjesta u čekaoni
    int preostala_mjesta; //semafor klijenti štiti ovu varijablu
    bool otvoreno; //semafor radi_salon štiti ovu varijablu
    bool spava; //semafor frizerka_spava štiti ovu varijablu
    int redni_broj;//ovom varijablom pamtimo redni broj klijenta
    int trenutni_klijent;//trenutni klijent za ispis frizerke
    atomic<int> zatvaram_postavljen;

}zajednicke;

zajednicke *buf;


void frizerka()
{
    sem_wait(radi_salon);
    buf->otvoreno = true;
    cout << "Frizerka: Otvaram salon" << endl;
    cout << "Frizerka: Postavljam znak 'OTVORENO'" << endl;
    sem_post(radi_salon);
    while (true) {
        sem_wait(radi_salon);
        if (!buf->otvoreno && buf->zatvaram_postavljen == 0) {
            cout << "Frizerka: Postavljam znak 'ZATVORENO'" << endl;
            buf->zatvaram_postavljen = 1;
        }
        sem_post(radi_salon);

        sem_wait(klijenti);
        if (buf->preostala_mjesta < buf->broj_mjesta) {
            buf->spava = false; //počela je radit, pa stavimo da je prestala spavat
            sem_post(klijenti);

            //Uzmi prvog klijenta iz čekaonice
            // Radi na frizuri
            sem_post(frizerka_slobodna);
            usleep(1);
            sem_wait(redkl);

            int vrijednost = buf->trenutni_klijent;

            sem_post(redkl);

            //Frizerka: Počinjem raditi na frizuri klijenta 1
            cout << "Frizerka: Počinjem raditi na frizuri klijenta " << vrijednost << endl;
            //Frizerka: Završavam rad na frizuri klijenta 1
            sem_post(ispis);
            sleep(4);
            cout << "Frizerka: Završavam rad na frizuri klijenta " << vrijednost << endl;


        } else if (buf->otvoreno){

            sem_post(klijenti);


            buf->spava = true;
            cout << "Frizerka: Spavam dok klijenti ne dođu" << endl;
            sem_wait(dosao_klijent); //spava dok ne dođu klijenti

        }
        else{
            cout << "Frizerka: Kraj radnog vremena i zavrsavam s radom" << endl;
            kill(0, SIGTERM);
        }
    }
}

void klijent(int redni_broj_klijenta)
{
    cout << "   Klijent " << redni_broj_klijenta << ": Dosao do salona" << endl;
    sem_wait(radi_salon);
    sem_wait(klijenti);
    if(buf->otvoreno && buf->preostala_mjesta > 0){
        cout << "   Klijent " << redni_broj_klijenta << ": Ulazim u čekaonicu (" << (buf->broj_mjesta - buf->preostala_mjesta + 1) << ")" << endl;

        buf->preostala_mjesta--;
        sem_post(radi_salon);
        sem_post(klijenti);

        sem_post(dosao_klijent);
        sem_wait(frizerka_slobodna);

        sem_wait(redkl);
        buf->trenutni_klijent = redni_broj_klijenta;
        sem_post(redkl);


        sem_wait(ispis);

        sem_wait(klijenti);
        buf->preostala_mjesta++;
        sem_post(klijenti);
        cout << "   Klijent " << redni_broj_klijenta << ": Sjedim u stolici frizerke i ona mi radi frizuru" << endl;
        //radi se frizura klijentu





    }
    else if(buf->otvoreno && buf->preostala_mjesta == 0){
        sem_post(radi_salon);
        sem_post(klijenti);
        cout << "   Klijent " << redni_broj_klijenta << ": Nema mjesta u čekaoni, vratit ću se sutra" << endl;
    }else{
        sem_post(radi_salon);
        sem_post(klijenti);
        cout << "   Klijent " << redni_broj_klijenta << ": Salon se sada zatvara i idem kući" << endl;
    }


}


int main(int argc, char *argv[])
{

    srand(time(nullptr));


    shmid = shmget (IPC_PRIVATE, sizeof(zajednicke) + 6*sizeof(sem_t), 0600);
    if(shmid == -1) exit(1); //greška -> nema zajedničke memorije

    buf = (zajednicke*)shmat(shmid, NULL, 0);
    klijenti = (sem_t *)(buf+1);
    radi_salon = klijenti + 1;
    frizerka_slobodna = radi_salon + 1;
    dosao_klijent = frizerka_slobodna + 1;
    redkl = dosao_klijent + 1;
    ispis = redkl + 1;


    buf->broj_mjesta = atoi (argv[1]);
    buf->redni_broj = 0;
    buf->preostala_mjesta = buf->broj_mjesta;
    buf->zatvaram_postavljen = 0;

    sem_init(radi_salon, 1, 1);
    sem_init(klijenti, 1, 1);
    sem_init(frizerka_slobodna, 1, 0);
    sem_init(dosao_klijent, 1, 0);
    sem_init(redkl, 1, 1);
    sem_init(ispis, 1, 0);


    pid_t pid;
    pid_t pid2;

    pid = fork();
    if(pid == -1) exit(-1);
    else if(pid == 0){
        pid2 = fork();
        if(pid2 == -1) exit(1);
        else if(pid2 == 0){
            // stvaranje klijenata
            for(int br_klijenata = 0; br_klijenata < 5; br_klijenata++){
                if(fork() == 0){
                    buf->redni_broj++;
                     klijent(buf->redni_broj);
                     exit(1);
                }

            }

            do{
                int interval_dolaska = rand() % 4 + 1;
                sleep(interval_dolaska);
                if(!buf->otvoreno) break;
                if(fork() == 0){
                     //klijenti dolaze u random intervalu od 1 do 4 sekundi
                     buf->redni_broj++;

                     klijent(buf->redni_broj);
                     exit(1);
                }
            }while(buf->otvoreno);
        }else{
            sleep(15); //radno vrijeme frizerke -> 15 sekundi
            buf->otvoreno = false; //zatvaramo salon
        }
    }else{
        //u roditeljskom procesu radimo frizerku
        frizerka();
    }



    sem_destroy(klijenti);
    sem_destroy(radi_salon);
    sem_destroy(frizerka_slobodna);
    sem_destroy(dosao_klijent);
    sem_destroy(redkl);
    sem_destroy(ispis);


    shmdt (buf);
    shmdt (klijenti);
    shmdt(radi_salon);
    shmdt(frizerka_slobodna);
    shmdt(dosao_klijent);
    shmdt(redkl);
    shmdt(ispis);
    shmctl (shmid, IPC_RMID, NULL);

    return 0;
}