#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include <malloc.h>
#include <unistd.h>
#include<atomic>
using namespace std;

int N;
int M;
int a = 0;
atomic<int> *BROJ;
atomic<int> *ULAZ;


int izracunaj_max_element()
{
   int max = BROJ[0];
   for(int i = 1; i < N; i++){
      if(BROJ[i] > max){
         max = BROJ[i];
      }
   }
   return max;
}


//Lamportov algoritam

//funkcija uđi u kritični odsječak
void ulaz(int ja)
{
   ULAZ[ja] = 1;
   BROJ[ja] = izracunaj_max_element() + 1;
   ULAZ[ja] = 0;
   for(int druga = 0; druga < N; druga++){
      while(ULAZ[druga] != 0);
      while(BROJ[druga] != 0 && (BROJ[druga] < BROJ[ja] || (BROJ[druga] == BROJ[ja] && druga < ja)));
   }
}

//funkcija izađi iz kritičnog odsječka
void izlaz(int ja)
{
   BROJ[ja] = 0;
}

void *dretva(void *arg)
{
   int *arg2 = (int*) arg;
   for(int i = 0; i < M; i++){
      ulaz(*arg2);
      a++;
      izlaz(*arg2);
   } 

   return nullptr;
}


int main(int argc, char* argv[])
{

   cout << "Krećemo s radom, PID = " << getpid() << endl;
   if(argc != 3){
      cout << "Neispravan broj argumenata" << endl;
      exit(1);
   }
   
   N =  atoi (argv[1]); //broj dretvi
   M =  atoi (argv[2]); //za koliko se treba povečati varijabla u svakoj dretvi

   cout << "N je " << N << endl;
   cout << "M je " << M << endl;

   *BROJ = new atomic<int>[N];
   *ULAZ = new atomic<int>[N];

   for(int i = 0; i < N; i++){
      BROJ[i] = 0;
      ULAZ[i] = 0;
   }

   pthread_t dretve[N] = {0}; //polje dretvi
   int BR[N] = {0};
   
   for(int i = 0; i < N; i++){
      BR[i] = i;
      if(pthread_create (&dretve[i], NULL, dretva, &BR[i]) ) {
         cout << "Ne mogu stvoriti dretvu" << endl;
			exit(1);
		}
   }

   for(int i = 0; i < N; i++){
      pthread_join(dretve[i], NULL); //čekamo kraj dretve dretva[i]
   }


   cout << "Varijabla a na kraju je " << a << endl;

   return 0;
}