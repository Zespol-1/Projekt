#include <stdio.h>
#include <stdlib.h>


// funkcja faktoryzujaca liczbe w bazie
// wejscie:
//   a - faktoryzowana liczba
//   N[] - tablica bazy rozkladu ([2,3,5,...])
//   size - rozmiar tablicy 'N[]'
// wyjscie:
//   N - wskaznik na przerobiona tablice licznosci czynnikow pierwszych
long long * factor(long long a, long long N[], long long size) {

    long long counter;

    for (int i = 0; i < size ; i++) {
        counter = 0;
        while (a % N[i] == 0) {
            a = a / N[i];
            counter++;
        }
        N[i] = counter;
    }
    return N;
}

int main(int argc, char** argv)
{
// faktoryzowana liczba
    long long a = 45;
// tablica bazy rozkladu 
    static long long N[7] = { 2,3,5,7,11,13,17 };

// wielkosc tablicy N[] (stała wartość)
    const int size = sizeof(N) / 8;

// wskaznik na tablice z czynnikami
    long long * result;

    result = factor(a, N,size);

// Deklaracja tablicy na wyniki
    long long wynik[size];

// Przepisanie wyniku faktoryzacji do oddzielnej tablicy i wyswietlenie na ekranie
    for (int i = 0; i < size; i++) {
        wynik[i] = *(result + i);
        printf("%llu ", wynik[i]);
    }


// TODO:
// odczyt bazy z pliku
}

