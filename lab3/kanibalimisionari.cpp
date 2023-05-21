#include<iostream>
#include<pthread.h>
#include<queue>
#include<unistd.h>
#include<sys/shm.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<cstdlib>
#include<ctime>

using namespace std;


//monitor

pthread_mutex_t m;
pthread_cond_t lijeva_obala;
pthread_cond_t desna_obala;
pthread_cond_t uvjet_camac;

//pomocne varijable

int br_kanibala = 0; //trenutni broj kanibala u čamcu
int br_misionara = 0; //trenutni broj misionara u čamcu
int br_ukupno = 0; //trenutni broj ekipe u čamcu

int pom_m = 1; //da pamtimo koji je misionar ušao
int pom_k = 1; // da pamtimo koji je kanibal ušao

int obala = 1; //0 je za lijevo, 1 je za desno
//inicijaliziramo na 1 jer se na početku nalazi na desnoj obali

int obala_misionara_kanibala = 1;

int plovidba = 0; //na početku čamac ne plovi


//pomocni redovi za ispis

queue<string> C; //camac
queue<string> L; //lijevo
queue<string> D; //desno

void nadjiIMakni(queue<string>& mojRed, const string& element) {
    queue<string> tempQueue;

    // Tražimo element i kopiramo sve ostale elemente u privremeni red
    while (!mojRed.empty()) {
        string trenutniElement = mojRed.front();
        mojRed.pop();

        if (trenutniElement != element) {
            tempQueue.push(trenutniElement);
        }
    }

    // Kopiramo elemente natrag u izvorni red
    while (!tempQueue.empty()) {
        mojRed.push(tempQueue.front());
        tempQueue.pop();
    }
}



void printqueue(queue<string> originalqueue){
    queue<string> copyqueue = originalqueue;
    while(!copyqueue.empty()){
        cout << copyqueue.front() << " ";
        copyqueue.pop();
    }
}



void ispis()
{
    cout << "C[";
    if(obala == 0){
        cout << "L] = ";
    }else{
        cout << "D] = ";
    }
    cout << "{ ";
    printqueue(C);
    cout << "} ";

    cout << "LO = ";
    cout << "{ ";
    printqueue(L);
    cout << "} ";

    cout << "DO = ";
    cout << "{ ";
    printqueue(D);
    cout << "} ";


    cout << endl;
    cout << endl;
}



void ispis2()
{
    printqueue(C);
    cout << endl;
    cout << endl;
}



void ispraznicamac()
{
    while(!C.empty()){
        C.pop();
    }
    br_ukupno = 0;
    br_kanibala = 0;
    br_misionara = 0;
}



void *camac(void *arg)
{
    while(1){
        pthread_mutex_lock(&m);
        if(obala == 0){
            cout << "C: Prazan na lijevoj obali" << endl;
        }
        else if(obala == 1){
            cout << "C: Prazan na desnoj obali" << endl;
        }
        ispis();

        while(br_ukupno < 3){
            //moramo čekati da budu barem tri putnika da možemo krenuti
            pthread_cond_wait(&uvjet_camac, &m);
        }

        cout << "C: " << br_ukupno << " putnika ukrcano, polazim za jednu sekundu" << endl;
        ispis();

        pthread_mutex_unlock(&m);
        sleep(1);
        pthread_mutex_lock(&m);

        if(obala == 0){
            cout << "C: prevozim s lijeve na desnu obalu: ";
            ispis2();
        }else{
            cout << "C: prevozim s desne na lijevu obalu: ";
            ispis2();
        }

        plovidba = 1; //krenuli smo ploviti;

        pthread_mutex_unlock(&m);
        sleep(2); //plovimo dvije sekunde
        pthread_mutex_lock(&m);

        //ispis tko je sve prevezen
        if(obala == 0){
            cout << "C: preveo s lijeve na desnu obalu: ";
            ispis2();
        }else{
            cout << "C: preveo s desne na lijevu obalu: ";
            ispis2();
        }

        plovidba = 0;

        ispraznicamac();

        obala = 1 - obala; //sad smo na drugoj obali u odnosu na onu na kojoj smo do sad bili

        if (obala == 0) // lijevo
            pthread_cond_broadcast(&lijeva_obala);
        else
            pthread_cond_broadcast(&desna_obala);

        pthread_mutex_unlock(&m);


    }
   return nullptr;
}



void *kanibal(void *arg)
{
    pthread_mutex_lock(&m);
    int obalaa = rand() % 2; //random biramo na koju će obalu doći

    pom_m = *(int*)arg;

    string kanibal = "K" + to_string(pom_m);
    if(obalaa == 0){
        L.push(kanibal);
    }else{
        D.push(kanibal);
    }



    if(obalaa == 0){
        cout << kanibal << ": došao na lijevu obalu" << endl;
        ispis();
    }else{
        cout << kanibal << ": došao na desnu obalu" << endl;
        ispis();
    }



    while(br_ukupno >= 7 || plovidba == 1 || obala != obalaa || br_kanibala == br_misionara && br_misionara != 0){
        if(obalaa == 0){
            pthread_cond_wait(&lijeva_obala, &m);
        }else{
            pthread_cond_wait(&desna_obala, &m);
        }
    }


    //nakon što je prošao uvjete za ući u čamac, povećava se broj kanibala i ukupan broj u čamcu
    br_ukupno += 1;
    br_kanibala += 1;



    if(obalaa == 0){
        nadjiIMakni(L, kanibal);
    }else{
        nadjiIMakni(D, kanibal);
    }


    C.push(kanibal);



    cout << kanibal << ": ušao u čamac" << endl;
    ispis();


    if(obalaa == 0){
        pthread_cond_broadcast(&lijeva_obala);
    }else{
        pthread_cond_broadcast(&desna_obala);
    }


    if (br_ukupno >= 3) {
    	pthread_cond_signal(&uvjet_camac);
    }


    pthread_mutex_unlock(&m);
    return nullptr;
}




void *misionar(void *arg)
{
    pthread_mutex_lock(&m);
    int obalaa = rand() % 2; //random biramo na koju će obalu doći

    pom_k = *(int*)arg;

    string misionar = "M" + to_string(pom_k);
    if(obalaa == 0){
        L.push(misionar);
    }else{
        D.push(misionar);
    }



    if(obalaa == 0){
        cout << misionar << ": došao na lijevu obalu" << endl;
        ispis();
    }else{
        cout << misionar << ": došao na desnu obalu" << endl;
        ispis();
    }



    while(br_ukupno >= 7 || plovidba == 1 || (br_kanibala > br_misionara) || obala != obalaa){
        if(obalaa == 0){
            pthread_cond_wait(&lijeva_obala, &m);
        }else{
            pthread_cond_wait(&desna_obala, &m);
        }
    }


    //nakon što je prošao uvjete za ući u čamac, povećava se broj kanibala i ukupan broj u čamcu
    br_ukupno += 1;
    br_misionara += 1;




    if(obalaa == 0){
        nadjiIMakni(L, misionar);
    }else{
        nadjiIMakni(D, misionar);
    }

    C.push(misionar);


    cout << misionar << ": ušao u čamac" << endl;
    ispis();


    if(obalaa == 0){
        pthread_cond_broadcast(&lijeva_obala);
    }else{
        pthread_cond_broadcast(&desna_obala);
    }


    if (br_ukupno >= 3) {
    	pthread_cond_signal(&uvjet_camac);
    }


    pthread_mutex_unlock(&m);
    return nullptr;
}


void *misionari_pom(void *arg){
    pthread_t dretva;
    int indeggs = 0;
    int indeksi[1024];
    for(int i = 0; i < 1024; i++){
        indeksi[i] = i;
    }
    while(1){
        sleep(1);
        sleep(1);
        pthread_create(&dretva, NULL, misionar, &indeksi[++indeggs]);
    }
}

void *kanibali_pom(void *arg){
    pthread_t dretva;
    int indeggs = 0;
    int indeksi[1024];
    for(int i = 0; i < 1024; i++){
        indeksi[i] = i;
    }
    while(1){
        sleep(1);
        pthread_create(&dretva, NULL, kanibal, &indeksi[++indeggs]);
    }
}



int main(void)
{

    srand(time(nullptr));

    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&lijeva_obala, NULL);
    pthread_cond_init(&desna_obala, NULL);
    pthread_cond_init(&uvjet_camac, NULL);

    pthread_t dretva_camac, dretva_misionari, dretva_kanibali;
    int pom[3];
    pthread_create(&dretva_camac, NULL, camac, &pom[0]);
    pthread_create(&dretva_misionari, NULL, kanibali_pom, &pom[1]);
    pthread_create(&dretva_kanibali, NULL, misionari_pom, &pom[2]);

    pthread_join(dretva_camac, NULL);
    pthread_join(dretva_misionari, NULL);
    pthread_join(dretva_kanibali, NULL);

    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&lijeva_obala);
    pthread_cond_destroy(&desna_obala);
    pthread_cond_destroy(&uvjet_camac);
}