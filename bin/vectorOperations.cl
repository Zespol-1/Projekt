
unsigned long product_mult32_reduce(unsigned long tbr) 
{
    unsigned long p = 9223372036854775783-1;
    tbr = tbr % p;
    unsigned long x1 = tbr >> 32;
    unsigned long y1 = tbr & 0xffffffff;
    unsigned long res = ((y1 << 32) % p) + x1 * 52;
    return res % p;
}

unsigned long product_mult64_reduce(unsigned long tbr) 
{
    unsigned long p = 9223372036854775783-1;
    tbr = tbr % p;
    unsigned long x1 = tbr >> 32;
    unsigned long y1 = tbr & 0xffffffff;
    unsigned long res = product_mult32_reduce(x1 * 52) + y1 * 52;
    return res % p;
}

unsigned long mult(unsigned long a, unsigned long b) 
{
    unsigned long p = 9223372036854775783-1;
    unsigned long x1 = a;
    x1 = x1 >> 32;
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


__kernel void vectorReduce(__global unsigned long* a, __global unsigned long* b,  unsigned long r)
{
	unsigned long p = 9223372036854775783-1;
	unsigned int n = get_global_id(0);
	a[n] = (a[n] + (p - mult(b[n], r))) % (p);
}

__kernel void vectorScalarMult(__global unsigned long* a, unsigned long b)
{
	unsigned int n = get_global_id(0);
	a[n] = mult(a[n], b);
}