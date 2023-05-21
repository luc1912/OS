#include <bits/stdc++.h>
using namespace std;
typedef long long ll;
typedef long double ld;
typedef pair<ll, ll> ii;
#define pb push_back
#define all(x) (x).begin(), (x).end()
#define sqr(x) ((x) * (x))
#define X first
#define Y second
#define FOR(i, a, b) for (int i = (a); i < (b); ++i)
#define REP(i, n) FOR (i, 0, n)
#define TRACE(x) cout << #x << " = " << x << endl
#define _ << " _ " <<
#define debug(...) fprintf(stderr, __VA_ARGS__)
#define MOD 1000000007LL

int n, m;

const int MEM = 1 << 16;

struct proces {
  vector<int> tablica;

  proces() : tablica(vector<int>(16)) {
  }
};

struct okvir {
  char oktet[64];
  int owner = -1;
  int page;
  int address;
};

vector<proces> procesi;
char disk[MEM];
vector<okvir> ram;

const int MASK_PAGE = 0b1111000000;
const int MASK_SHIFT = 0b0000111111;

const int MASK_OKVIR = 0b1111111111000000;
const int MASK_P = 0b0000000000100000;
const int MASK_LRU = 0b0000000000011111;

int main(int, char* argv[])
{
  srand(7);
  memset(disk, 0, sizeof disk);
  n = 0;
  for (int i = 0; (argv[1])[i]; i++) {
    n *= 10;
    n += (argv[1])[i] - '0';
  }
  m = 0;
  for (int i = 0; (argv[2])[i]; i++) {
    m *= 10;
    m += (argv[2])[i] - '0';
  }
  ram.resize(m);
  //cout << n << " " << m << endl;
  for (int i = 0; i < n; i++) {
    procesi.emplace_back();
    for (int j = 0; j < 16; j++) {
      static int broj = 0;
      procesi[i].tablica[j] |= broj << 6;
      ++broj;
    }
  }
  for (int t = 0; ; ) {
    for (int i = 0; i < n; i++) {
      int x = rand() % MEM;
      ///x = 0x1FE;

      int stranica = (x & MASK_PAGE) >> 6;
      int pomak = x & MASK_SHIFT;

      auto& tab = procesi[i].tablica;

      int& zapis = tab[stranica];

      int p = (zapis & MASK_P) >> 5;


      cout << "---------------------------" << endl;
      cout << "proces: " << i << endl;
      cout << "\tt: " << t << endl;
      cout << "\tlog. adresa: " << bitset<16>(x) << endl;
      printf("0x%04x\n", x);

      if (!p) {
        int mni = 0;
        for (int j = 0; j < m; j++) {
          if (ram[j].owner == -1) {
            mni = j;
            break;
          } else if ((procesi[ram[j].owner].tablica[ram[j].page] & MASK_LRU) <
                     (procesi[ram[mni].owner].tablica[ram[mni].page] & MASK_LRU)) {
                       mni = j;
                     }
        }
        cout << "\tPromasaj!" << endl;
        if (ram[mni].owner != -1) {
          //copy(ram[mni].oktet, ram[mni].oktet + 64, disk[ram[mni].address]);
          for (int l = 0; l < 64; l++) {
            disk[ram[mni].address + l] = ram[mni].oktet[l];
          }
          procesi[ram[mni].owner].tablica[ram[mni].page] &= ~MASK_P;
          cout << "\t\tIzbacujem stranicu " << bitset<16>(ram[mni].page << 6) << " iz procesa " << ram[mni].owner << endl;
      printf("0x%04x\n", ram[mni].page << 6);
          cout << "\t\tlru izbacene stranice: " << bitset<5>(procesi[ram[mni].owner].tablica[ram[mni].page]) << "\n";
      printf("0x%04x\n", procesi[ram[mni].owner].tablica[ram[mni].page]);
        }
        cout << "\t\tdodijeljen okvir " << bitset<4>(mni) << endl;
      printf("0x%04x\n", mni);
        zapis |= MASK_P;
        ///zapis |= (mni << 6);
        ram[mni].owner = i;
        ram[mni].page = stranica;
        ///ram[mni].address = (mni << 6);
        ram[mni].address = zapis & MASK_OKVIR;
        for (int l = 0; l < 64; l++) {
          ram[mni].oktet[l] = disk[(zapis & MASK_OKVIR) + l];
        }
        //copy(disk[mni], disk[mni + 64], ram[mni].oktet);
      }
      zapis &= ~MASK_LRU;
      zapis |= t;

      int okvir_adresa = zapis & MASK_OKVIR;
      ///int lru = zapis & MASK_LRU;

      int konacna_adresa = okvir_adresa | pomak;

      cout << "\tfiz. adresa: " << bitset<16>(konacna_adresa) << "\n";
      printf("0x%04x\n", konacna_adresa);
      cout << "\tzapis tablice: " << bitset<16>(zapis) << endl;
      printf("0x%04x\n", zapis);

      int index = 0;
      for (index = 0; index < m; ++index) {
        if (ram[index].owner == i && ram[index].page == stranica) {
          break;
        }
      }

      cout << "\tsadrzaj adrese: " << int(ram[index].oktet[pomak]) << endl;

      ++ram[index].oktet[pomak];

      ++t;
      if (t == 31) {
        t = 1;
        for (int j = 0; j < n; j++) {
          for (int k = 0; k < 16; k++) {
            procesi[j].tablica[k] &= ~MASK_LRU;
          }
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
  }
  return 0;
}
