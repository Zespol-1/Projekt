

#include <iostream>
#include <stdio.h>

unsigned long long halfMult(unsigned long long a, unsigned long long b) {

    unsigned long long p = 9223372036854775783;
    unsigned long long x1 = a % p;
    x1 = x1 >> 16;
    unsigned long long y1;
    y1 = a & 65535;
    unsigned long long x2 = b % p;
    x2 = x2 >> 16;
    unsigned long long y2;
    y2 = b & 65535;
    unsigned long long halfa = (((x1 * x2) % p) * 4294967296) % p;
    unsigned long long halfb = (((x1 * y2) % p) * 65536) % p;
    unsigned long long halfc = (((x2 * y1) % p) * 65536) % p;
    unsigned long long halfd = (y1 * y2) % p;

    return ((((halfa + halfb) % p + halfc) % p + halfd) % p);
}
unsigned long long mult(unsigned long long a, unsigned long long b) {

    unsigned long long p = 9223372036854775783;
    unsigned long long x1 = a;
    x1 = x1 >> 32;
    unsigned long long y1;
    y1 = a & 4294967295;
    unsigned long long x2 = b;
    x2 = x2 >> 32;
    unsigned long long y2;
    y2 = b & 4294967295;

    unsigned long long halfa = (halfMult(x1, x2) * 50) % p;
    unsigned long long halfb = (halfMult(x1, y2) * 4294967296) % p;
    unsigned long long halfc = (halfMult(x2, y1) * 4294967296) % p;
    unsigned long long halfd = halfMult(y1, y2) % p;

    return ((((halfa + halfb)%p + halfc)%p + halfd)% p);
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
    printf("\n%llu\n%llu\n", halfMult(123567, 123456), 15255087552);*/

    //printf("\n%llu\n%llu\n", halfMult(4294967294, 4294967295), 9223372023969873947);
    return 0;


}

