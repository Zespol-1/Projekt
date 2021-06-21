#include <iostream>
#include <fstream>
#include <string>
using namespace std;

void tablica(unsigned long long tmp_tab[1024])
{
    fstream uchwyt; //obiekt typu fstream (uchwyt do pliku)

    uchwyt.open("plik.txt"); //otwieramy plik: plik.txt (plik - nazwa pliku, txt - rozszerzenie)
    string linia;
    
    for (int i = 0; i < 1023; i++) {
        getline(uchwyt, linia); //pobierz linijkę
        tmp_tab[i] = stoull(linia, NULL, 0);
    }
    
    uchwyt.close(); //zamykamy plik
}