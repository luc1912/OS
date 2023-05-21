#include<iostream>
#include<cstring>
#include<vector>
#include<unistd.h>
#include<signal.h>
#include <sys/wait.h>
using namespace std;


pid_t provjera = 0;
bool stanje = false;


void obradi_sigint(int sig)
{
    switch (provjera){
        case 0:
            stanje = true;
            cout << endl;
            return;

        default:
            kill(provjera, SIGINT);
            wait(nullptr);
            cout << endl;
            return;
        }
}

int main(void)
{

    //maskiranje signala SIGINT
    struct sigaction act;
    act.sa_handler = obradi_sigint;
    act.sa_flags = 0;
    sigaction(SIGINT, &act, nullptr);


    while(true){

        string naredba, spremi_naredbu, putanja, naredba2;
        int poc = 0;
        int kraj = -1;
       


        //ispisujemo "fsh> " i čekamo unos naredbe
        cout << "fsh> ";
        cin.clear();
        getline(cin, naredba);

        if(stanje){
            stanje = false;
            continue;
        }

         else if(naredba == "exit" || naredba  == ""){
            break;
        }



        //varijable okoline
        //parsiranje po ':'
        putanja = getenv("PATH");
        putanja += ":";
        vector<string> lista_putanja;
         while (kraj != putanja.length()-1){
            kraj = putanja.find(':', poc);
            string rijec = putanja.substr(poc, kraj-poc);
            lista_putanja.push_back(rijec);

            poc = kraj + 1;
        }

        if(naredba[0] != '.' && naredba.substr(0, 2) != "cd"){
            if(naredba.find(" ") != string::npos){
                //ako nađemo razmak unutar naredbe, znači da imamo još jedan argument
                naredba2 = naredba.substr(naredba.find(" ") + 1);
            }

            naredba = naredba.substr(0, naredba.find(" "));


            for(int i = 0; i < lista_putanja.size(); i++){
                lista_putanja[i] += "/" + naredba;
                if(access(lista_putanja[i].c_str(), F_OK) == 0){
                    if(naredba2.empty()){
                        naredba = lista_putanja[i];
                    }else{
                        naredba = lista_putanja[i] + " " + naredba2;
                    }
                    break;

                }
            }
        }


        poc = 0;
        kraj = -1;

        naredba += " ";
        //naredbu koju je unio koristik ćemo spremiti u listu riječiž
        //parsiramo po razmaku
        vector<string> lista_rijeci;
        while (kraj != naredba.length()-1){
            kraj = naredba.find(' ', poc);
            string rijec = naredba.substr(poc, kraj-poc);
            lista_rijeci.push_back(rijec);

            poc = kraj + 1;
        }


        //izvršavanje naredbe
        if(lista_rijeci[0] == "cd"){
            if(chdir(lista_rijeci[1].c_str()) != 0){ //.c_str pretvara u polje charactera jer to prima fja chdir
                if(lista_rijeci[1].length() != 0 && lista_rijeci.size() > 1)
                cout << "The directory " << lista_rijeci[1] << " does not exist" << endl;
                else if(lista_rijeci.size() == 1){ //upisano samo cd
                    cout << "The directory null does not exist" << endl;
                }
            }
            else{
                //ako je sve u redu ispisujemo trenutni direktorij
                char cwd[100];
                cout << getcwd(cwd, sizeof(cwd)) << endl;
            }


        }


        else{
            provjera = fork();
            switch(provjera){
                case -1:
                    cerr << "Neuspjelo forkanje" << endl;
                    exit(1);
                case 0:
                {
                    //pretvaramo lista_rijeci u char polje pointera
                    char** pp = new char*[lista_rijeci.size()+1]; //dvodimenzionalno polje charova
                    for(int i=0; i<lista_rijeci.size(); i++) {
                        pp[i] = new char[lista_rijeci[i].size()];
                        strcpy(pp[i], lista_rijeci[i].c_str());
                    }
                    pp[lista_rijeci.size()] = NULL; //zadni element mora biti NULL


                    if (execve(pp[0], pp, NULL) == -1) {
                        cerr << "Command not found" << endl;
                        exit(1);
                    }
                    break;
                    
                }
                default:
                    wait(NULL);
                    provjera = 0;
            }
        }
    }
    return 0;
}