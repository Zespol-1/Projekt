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
        x = mult(x,x);  // Zmien x na x^2
    }
    return res;
}


void tests() {


}


int main()
{
    cout << power(2138,696969);
}