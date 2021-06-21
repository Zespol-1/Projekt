// potegowanie.cpp : Ten plik zawiera funkcję „main”. W nim rozpoczyna się i kończy wykonywanie programu.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cassert>

using namespace std;
const unsigned long long p = 9223372036854775783;

uint64_t product_mult32_reduce(uint64_t tbr) {
    tbr = tbr % p;
    uint64_t x1 = tbr >> 32;
    uint64_t y1 = tbr & 0xffffffff;
    uint64_t res = ((y1 << 32) % p) + x1 * 50;
    return res % p;
}

uint64_t product_mult64_reduce(uint64_t tbr) {
    tbr = tbr % p;
    uint64_t x1 = tbr >> 32;
    uint64_t y1 = tbr & 0xffffffff;
    uint64_t res = product_mult32_reduce(x1 * 50) + y1 * 50;
    return res % p;
}


unsigned long long mult(unsigned long long a, unsigned long long b) {


    unsigned long long x1 = a;
    x1 = x1 >> 32;
    unsigned long long y1;
    y1 = a & 0xffffffff;
    unsigned long long x2 = b;
    x2 = x2 >> 32;
    unsigned long long y2;
    y2 = b & 0xffffffff;


    uint64_t halfa = product_mult64_reduce(x1 * x2);
    uint64_t halfb = product_mult32_reduce(x1 * y2);
    uint64_t halfc = product_mult32_reduce(x2 * y1);
    uint64_t halfd = (y1 * y2) % p;


    return ((((halfa + halfb) % p + halfc) % p + halfd) % p);
}

unsigned long long power(unsigned long long x, unsigned long long y)
{
    unsigned long long res = 1;     // wynik

    while (y > 0)
    {
        // Jesli y jest nieparzyste, mnozy x z wynikiem
        if (y & 1)
            res = mult(res, x);

        // y musi byc parzyste
        y = y >> 1; // podziel y = y/2
        x = mult(x, x);  // Zmien x na x^2
    }
    return res;
}

/*
PROBNA WERSJA ODWRACANIA MODULO
unsigned long long inverse(unsigned long a, unsigned long b) {
    unsigned long long u = 1;
    unsigned long long w = a;
    unsigned long long x = 0;
    unsigned long long z = b;

    unsigned long long q;

    unsigned long long temp;

    while (w != 0) {
        if (w < z) {
            temp = x;
            x = u;
            u = temp;

            temp = w;
            w = z;
            z = temp;
        }

        q = w / z;
        u = u - q * x;
        w = w - q * z;
    }

    if (z != 1) {
        return NULL;
    }
    if (x < 0) {
        x = x + b;
    }
    return x % b;
}

*/

// Struktura implementujaca 64-bitowa liczbe UnsignedLongLong wraz ze znakiem
// value - wartosc liczbowa
// isNegative - boolean okreslajacy znak liczby
typedef struct tag_uint64AndSign {
    uint64_t  value;
    bool isNegative;
} uint64AndSign;

// Funkcja wykonujaca odwracanie modulo
// Wejscie: a - liczba naturalna (64-bitowa)
//          b - liczba naturalna (64-bitowa)
// Wyjscie: x1.value - wartosc taka, ze 'a * x1.value = 1 (mod b)'
uint64_t mul_inv(uint64_t a, uint64_t b)
{
    if (b <= 1)
        return 0;

    uint64_t b0 = b;
    uint64AndSign x0 = { 0, false }; // b = 1*b + 0*a
    uint64AndSign x1 = { 1, false }; // a = 0*b + 1*a

    while (a > 1)
    {
        if (b == 0)
            return 0;
        uint64_t q = a / b;
        uint64_t t = b; b = a % b; a = t;

        uint64AndSign t2 = x0;
        uint64_t qx0 = q * x0.value;
        if (x0.isNegative != x1.isNegative)
        {
            x0.value = x1.value + qx0;
            x0.isNegative = x1.isNegative;
        }
        else
        {
            x0.value = (x1.value > qx0) ? x1.value - qx0 : qx0 - x1.value;
            x0.isNegative = (x1.value > qx0) ? x1.isNegative : !x0.isNegative;
        }
        x1 = t2;
    }

    return x1.isNegative ? (b0 - x1.value) : x1.value;
}

// Funkcja testujaca implementacje power() oraz mul_inv()
void tests() {
    cout << "Testy potegowania" << endl;
    unsigned long long x;
    bool good = false;

    x = power(2138, 696969);
    if (x != 8107794421691675220) cout << "BLAD\n";

    x = power(2138, 969);
    if (x != 2804770264334761214) cout << "BLAD\n";
    
    x = power(2137, 969);
    if (x != 5232906398635066975) cout << "BLAD\n";

    cout << "Koniec testow potegowania\nTesty odwracania modulo \n";

    x = mul_inv(6, 13);
    if (x != 11) cout << "BLAD\n";

    x = mul_inv(69, 13);
    if (x != 10) cout << "BLAD\n";

    x = mul_inv(69, 2137);
    if (x != 1084) cout << "BLAD\n";

    x = mul_inv(420, 2137);
    if (x != 1155) cout << "BLAD\n";

    x = mul_inv(69, p);
    if (x != 9089699978349634105) cout << "BLAD\n";

    x = mul_inv(2137, p);
    if (x != 5878443010854564631) cout << "BLAD\n";

    x = mul_inv(2138, p);
    if (x != 7829943988256042117) cout << "BLAD\n";


    cout << "koniec testow odwracania!\n";

    // WARTOSCI TESTOWE LICZONE W WOLFRAMIE
    // 69 ^ (-1) mod 13 = 10
    // 69 ^ (-1) mod 2137 = 1084
    // 420 ^ (-1) mod 2137 = 1155
    // 69 ^ (-1) mod p = 9089699978349634105
    // 2137 ^ (-1) mod p = 5878443010854564631
    // 2138 ^ (-1) mod p = 7829943988256042117
}


int main()
{
    tests();
}