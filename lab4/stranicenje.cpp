#include <iostream>
#include <vector>
#include <ctime>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include <windows.h>
#include <climits>
#include <tuple>

using namespace std;

class Proces {
private:
    int N;
    int M;
    char*** disk; //disk -> N procesa, 16 logičkih stranica, 64 okteta, disk[N][16][64]
    short** okvir; //okvir -> M okvira od 64 okteta, okvir[M][64]
    short** tablica; //tablica -> za svaki od N procesa za svaku od 16 logičkih stranica, tablica[N][16]
    int t;
    int pocetni;

public:
    Proces(int n, int m) {
        N = n;
        M = m;

        // Alokacija memorije za polje disk
        disk = new char**[N];
        for (int i = 0; i < N; i++) {
            disk[i] = new char*[16];
            for (int j = 0; j < 16; j++) {
                disk[i][j] = new char[64];
            }
        }

        // Alokacija memorije za polje okvir
        okvir = new short*[M];
        for (int i = 0; i < M; i++) {
            okvir[i] = new short[64];
        }

        // Alokacija memorije za polje tablica
        tablica = new short*[N];
        for (int i = 0; i < N; i++) {
            tablica[i] = new short[16];
        }

        pocetni = 0;
    }

    ~Proces() {
        // Dealokacija memorije za polje disk
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < 16; j++) {
                delete[] disk[i][j];
            }
            delete[] disk[i];
        }
        delete[] disk;

        // Dealokacija memorije za polje okvir
        for (int i = 0; i < M; i++) {
            delete[] okvir[i];
        }
        delete[] okvir;

        // Dealokacija memorije za polje tablica
        for (int i = 0; i < N; i++) {
            delete[] tablica[i];
        }
        delete[] tablica;
    }

    void inicijalizacija(int broj_procesa) { //inicijaliziramo tablicu za i-ti proces
        for (int i = 0; i < 16; i++) {
            tablica[broj_procesa][i] = 0;
        }
    }

    char dohvati_fizicku_adresu(int proces, unsigned short x, int ispis) {
        char indeks = x >> 6;
        int okvir_tmp = (tablica[proces][x >> 6] >> 5) & 0x1;

        if (okvir_tmp == 0 && !ispis) {
            cout << "\tPromasaj!" << endl;
            int i_najmanji, j_najmanji;
            char adresa_najmanji;
            tie(i_najmanji, j_najmanji, adresa_najmanji) = pronadi_i_dodijeli_okvir(proces, x);
            ucitaj_sadrzaj_stranice_s_diska(proces, x, adresa_najmanji >> 6);
            azuriraj_tablicu_prevodenja(proces, x, i_najmanji, j_najmanji, adresa_najmanji);
            if (pocetni < M) pocetni++;
        }
        return indeks;
    }

    char dohvati_sadrzaj(int proces, unsigned short x) {
        char fizicka_adresa = dohvati_fizicku_adresu(proces, x, 0);
        char sadrzaj = okvir[tablica[proces][fizicka_adresa] >> 6][x & 0x3F];
        return sadrzaj;
    }

    void zapis_vrijednost(int proces, unsigned short x, char sadrzaj) {
        char fizicka_adresa = dohvati_fizicku_adresu(proces, x, 1);
        // Zapiši vrijednost i na adresu fizicka_adresa
        okvir[tablica[proces][fizicka_adresa] >> 6][x & 0x3F] = sadrzaj;
    }

    tuple<int, int, char> pronadi_i_dodijeli_okvir(int proces, char x) {
        int i_najmanji = 0;
        int j_najmanji = 0;
        char adresa_najmanji = 0;
        if (pocetni < M) {
            cout << "Dodjeljen okvir: 0x" << hex << pocetni << endl;
        } else {
            int najmanji_t = INT_MAX;
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < 16; j++) {
                    if (((tablica[i][j] >> 5) & 1) == 1 && (tablica[i][j] & 0x1F) < najmanji_t) {
                        i_najmanji = i;
                        j_najmanji = j;
                        adresa_najmanji = tablica[i][j] & 0xffc0;
                    }
                }
            }

            cout << "\t\tIzbacujem stranicu 0x" << hex << (x & 0xfc0) << " iz procesa" << i_najmanji << endl;
            cout << "\t\tLRU izbacene stranice: 0x" << hex << (tablica[i_najmanji][j_najmanji] & 0x1f) << endl;
            cout << "\t\tDodjeljen okvir: 0x" << hex << (adresa_najmanji >> 6) << endl;
        }

        cout << "\tFizicka adresa: 0x" << hex << ((tablica[proces][x >> 6] & 0xffc0) | (x & 0x3f)) << endl;
        cout << "\tZapis tablice: 0x" << hex << tablica[proces][x >> 6] << endl;
        cout << "\tSadrzaj adrese: 0x" << hex << okvir[tablica[proces][x >> 6] >> 6][x & 0x3f] << endl;
        return make_tuple(i_najmanji, j_najmanji, adresa_najmanji);
    }

    void ucitaj_sadrzaj_stranice_s_diska(int proces, unsigned short x, int adresa) {
        //učitamo sadržaj stranice X sa diska u taj okvir
        int pom;
        if (pocetni < M) {
            pom = pocetni;
        } else {
            pom = adresa;
        }
        for (int i = 0; i < 64; i++) {
            okvir[pom][i] = disk[proces][x >> 6][i];
        }
    }

    void azuriraj_tablicu_prevodenja(int proces, unsigned short x, int i_najmanji, int j_najmanji, char adresa_najmanji) {
        //stranica koja je prije bila u okviru ju maknemo iz tablice prevođenja
        //trenutnu stranicu stavimo u tablicu prevođenja
        if (pocetni < M) {
            tablica[proces][x >> 6] = (pocetni << 6) + 32 + t;
        } else {
            tablica[i_najmanji][j_najmanji] &= 0xffdf;
            tablica[proces][x >> 6] = adresa_najmanji | 32 | t;
        }
    }

    void algoritam1() {
        t = 0;
        srand(time(nullptr));

        while (1) {
            for (int proces = 0; proces < N; proces++) {
                cout << "-----------------------------" << endl;
                cout << "Proces: " << proces << endl;
                cout << "\tt: " << dec << t << endl;

                //unsigned short x = rand() % 0x3FE; //generiranje random logičke adrese
                unsigned short x = 0x01fe;
                cout << "Logicka adresa: 0x" << hex << x << endl;
                char sadrzaj = dohvati_sadrzaj(proces, x);
                sadrzaj++;
                zapis_vrijednost(proces, x, sadrzaj);
                t++;
                Sleep(1000);
            }
        }
    }

};


int main() {
    int N;
    int M;

    cout << "Unesite broj procesa: ";
    cin >> N;

    cout << "Unesite broj okvira: ";
    cin >> M;

    Proces proces(N, M);

    for (int i = 0; i < N; i++) {
        proces.inicijalizacija(i);
    }

    proces.algoritam1();

    return 0;
}