#include <iostream>
#include <stdio.h>
#include<cassert>
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
    uint64_t res = product_mult32_reduce(x1*50) + y1 * 50;
    return res % p;
}

unsigned long long halfMult(uint32_t a, uint32_t b) {
    assert(a & 0xffffffff00000000 == 0);
    assert(b & 0xffffffff00000000 == 0);
    //unsigned long long p = 9223372036854775783;
    unsigned long long x1 = a;
    x1 = x1 >> 16;
    unsigned long long y1;
    y1 = a & 0xffff;
    unsigned long long x2 = b;
    x2 = x2 >> 16;
    unsigned long long y2;
    y2 = b & 0xffff;
    //unsigned long long p2 = 1ull << 32;
    unsigned long long halfa = (((x1 * x2)) * (1ull<<32)) % p;
    unsigned long long halfb = (((x1 * y2) % p) * (1<<16)) % p;
    unsigned long long halfc = (((x2 * y1) % p) * (1<<16)) % p;
    unsigned long long halfd = (y1 * y2) % p;

    return ((((halfa + halfb) % p + halfc) % p + halfd) % p);
}
unsigned long long mult(unsigned long long a, unsigned long long b) {

    //unsigned long long p = 9223372036854775783;
    unsigned long long x1 = a;
    cout << x1 << endl;
    x1 = x1 >> 32;
    cout << x1<<endl;
    unsigned long long y1;
    y1 = a & 0xffffffff;
    unsigned long long x2 = b;
    x2 = x2 >> 32;
    unsigned long long y2;
    y2 = b & 0xffffffff;

    //unsigned long long halfa = ((halfMult(x1, x2)%p) * 50) % p;
    //unsigned long long temp = (x1 * y2) % p;
    //unsigned long long halfb = halfMult((x1 * y2) % p, 1ull << 32); //(halfMult((halfMult(x1, y2)%p) , (1ull<<32))) % p;
    //unsigned long long halfc = ((halfMult(x2, y1)%p) * (1ull<<32)) % p;
    //unsigned long long halfd = halfMult(y1, y2) % p;

    uint64_t halfa = product_mult64_reduce(x1 * x2);
    uint64_t halfb = product_mult32_reduce(x1 * y2);
    uint64_t halfc = product_mult32_reduce(x2 * y1);
    uint64_t halfd = (y1 * y2) % p;


    return ((((halfa + halfb) % p + halfc) % p + halfd) % p);
}

int main()
{
    

    
    printf("%llu\n%llu", mult(2345676543234, 23456743234), 4518184817900433161);
    /*printf("\n%llu\n%llu\n", halfMult(625436, 203450), 127244954200);
    printf("\n%llu\n%llu\n", halfMult(234536, 234573), 55015813128);
    printf("\n%llu\n%llu\n", halfMult(123344, 235678), 29069467232);
    printf("\n%llu\n%llu\n", halfMult(987635, 234567), 231666579045);
    printf("\n%llu\n%llu\n", halfMult(213456, 987653), 210820458768);
    printf("\n%llu\n%llu\n", halfMult(23455, 276543), 6486316065);
    printf("\n%llu\n%llu\n", halfMult(12345, 487656), 6020113320);
    printf("\n%llu\n%llu\n", halfMult(334567, 125672), 42045704024);
    printf("\n%llu\n%llu\n", halfMult(12422, 334456), 4154612432);
    printf("\n%llu\n%llu\n", halfMult(123567, 123456), 15255087552);

 

    printf("\n%llu\n%llu\n", halfMult(4294967294, 4294967295), 9223372023969873947);*/
    //printf("\n%llu\n%llu\n", mult(1ull << 32, 1ull << 32), 50ull);

    return 0;


}
