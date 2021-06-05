unsigned long product_mult32_reduce(unsigned long tbr) {
    tbr = tbr % p;
    unsigned long x1 = tbr >> 32;
    unsigned long y1 = tbr & 0xffffffff;
    unsigned long res = ((y1 << 32) % p) + x1 * 50;
    return res % p;
}

unsigned long product_mult64_reduce(unsigned long tbr) {
    tbr = tbr % p;
    unsigned long x1 = tbr >> 32;
    unsigned long y1 = tbr & 0xffffffff;
    unsigned long res = product_mult32_reduce(x1 * 50) + y1 * 50;
    return res % p;
}


unsigned long mult(unsigned long a, unsigned long b) {

    unsigned long x1 = a;
    cout << x1 << endl;
    x1 = x1 >> 32;
    cout << x1 << endl;
    unsigned long y1;
    y1 = a & 0xffffffff;
    unsigned long x2 = b;
    x2 = x2 >> 32;
    unsigned long y2;
    y2 = b & 0xffffffff;

    unsigned long halfa = product_mult64_reduce(x1 * x2);
    unsigned long halfb = product_mult32_reduce(x1 * y2);
    unsigned long halfc = product_mult32_reduce(x2 * y1);
    unsigned long halfd = (y1 * y2) % p;


    return ((((halfa + halfb) % p + halfc) % p + halfd) % p);
}