#include <stdio.h>

#include <stdlib.h>

#include <iostream>

#include <CL/cl.h>

#include <vector>
#include <fstream>
#include <string>


using namespace std;
#define MEM_SIZE (128)
#define MAX_SOURCE_SIZE (0x100000)
#define ncols 32
unsigned long long p = 9223372036854775783;
unsigned long long n = p - 1;

void __stdcall pfn_notify(const char* errinfo, const void* private_info, size_t cb, void* user_data)
{
    fprintf(stderr, "OpenCL Error (via pfn_notify): %s\n", errinfo);
}

void error_int(cl_int err)
{
    switch (err)
    {
    case CL_INVALID_PLATFORM:
        printf("CL_INVALID_PLATFORM\n"); break;
    case CL_INVALID_VALUE:
        printf("CL_INVALID_VALUE\n"); break;
    case CL_DEVICE_NOT_AVAILABLE:
        printf("CL_DEVICE_NOT_AVAILABLE \n"); break;
    case CL_DEVICE_NOT_FOUND:
        printf("CL_DEVICE_NOT_FOUND \n"); break;
    case CL_OUT_OF_HOST_MEMORY:
        printf("CL_OUT_OF_HOST_MEMORY  \n"); break;
    case CL_INVALID_DEVICE_TYPE:
        printf("CL_INVALID_DEVICE_TYPE  \n"); break;
    case CL_INVALID_CONTEXT:
        printf("CL_INVALID_CONTEXT \n"); break;
    case CL_INVALID_PROGRAM:
        printf("CL_INVALID_PROGRAM \n"); break;
    case CL_INVALID_DEVICE:
        printf("CL_INVALID_DEVICE \n"); break;
    case CL_INVALID_BINARY:
        printf("CL_INVALID_BINARY \n"); break;
    case CL_INVALID_BUILD_OPTIONS:
        printf("CL_INVALID_BUILD_OPTIONS \n"); break;
    case CL_INVALID_OPERATION:
        printf("CL_INVALID_OPERATION \n"); break;
    case CL_COMPILER_NOT_AVAILABLE:
        printf("CL_COMPILER_NOT_AVAILABLE \n"); break;
    case CL_BUILD_PROGRAM_FAILURE:
        printf("CL_BUILD_PROGRAM_FAILURE \n"); break;
    case CL_INVALID_COMMAND_QUEUE:
        printf("CL_INVALID_COMMAND_QUEUE \n"); break;
    case CL_INVALID_MEM_OBJECT:
        printf("CL_INVALID_MEM_OBJECT \n"); break;
    case CL_INVALID_KERNEL:
        printf("CL_INVALID_KERNEL \n"); break;
    default:
        printf("Other error\n");
    }
}

uint64_t product_mult32_reduce_n(uint64_t tbr) 
{
    tbr = tbr % n;
    uint64_t x1 = tbr >> 32;
    uint64_t y1 = tbr & 0xffffffff;
    uint64_t res = ((y1 << 32) % n) + x1 * 52;
    return res % n;
}

uint64_t product_mult32_reduce_p(uint64_t tbr)
{
    tbr = tbr % p;
    uint64_t x1 = tbr >> 32;
    uint64_t y1 = tbr & 0xffffffff;
    uint64_t res = ((y1 << 32) % p) + x1 * 50;
    return res % p;
}


uint64_t product_mult64_reduce_n(uint64_t tbr) 
{
    tbr = tbr % n;
    uint64_t x1 = tbr >> 32;
    uint64_t y1 = tbr & 0xffffffff;
    uint64_t res = product_mult32_reduce_n(x1 * 52) + y1 * 52;
    return res % n;
}

uint64_t product_mult64_reduce_p(uint64_t tbr)
{
    tbr = tbr % p;
    uint64_t x1 = tbr >> 32;
    uint64_t y1 = tbr & 0xffffffff;
    uint64_t res = product_mult32_reduce_p(x1 * 50) + y1 * 50;
    return res % p;
}

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

unsigned long long mult_n(unsigned long long a, unsigned long long b) 
{
    unsigned long long x1 = a;
    x1 = x1 >> 32;
    unsigned long long y1;
    y1 = a & 0xffffffff;
    unsigned long long x2 = b;
    x2 = x2 >> 32;
    unsigned long long y2;
    y2 = b & 0xffffffff;


    uint64_t halfa = product_mult64_reduce_n(x1 * x2);
    uint64_t halfb = product_mult32_reduce_n(x1 * y2);
    uint64_t halfc = product_mult32_reduce_n(x2 * y1);
    uint64_t halfd = (y1 * y2) % n;


    return ((((halfa + halfb) % n + halfc) % n + halfd) % n);
}


unsigned long long mult_p(unsigned long long a, unsigned long long b)
{
    unsigned long long x1 = a;
    x1 = x1 >> 32;
    unsigned long long y1;
    y1 = a & 0xffffffff;
    unsigned long long x2 = b;
    x2 = x2 >> 32;
    unsigned long long y2;
    y2 = b & 0xffffffff;


    uint64_t halfa = product_mult64_reduce_p(x1 * x2);
    uint64_t halfb = product_mult32_reduce_p(x1 * y2);
    uint64_t halfc = product_mult32_reduce_p(x2 * y1);
    uint64_t halfd = (y1 * y2) % p;


    return ((((halfa + halfb) % p + halfc) % p + halfd) % p);
}

unsigned long long fastExpMod(unsigned long long b, unsigned long long e)
{
    unsigned long long result = 1;
    if (1 & e)
        result = b;
    while (1) {
        if (!e) break;
        e >>= 1;
        b = mult_p(b, b);
        if (e & 1)
            result = mult_p(result, b);
    }
    return result;
}


void primes(unsigned long long tmp_tab[ncols-1])
{
    fstream uchwyt; //obiekt typu fstream (uchwyt do pliku)

    uchwyt.open("primes.txt"); //otwieramy plik: plik.txt (plik - nazwa pliku, txt - rozszerzenie)
    string linia;

    for (int i = 0; i < (ncols-1); i++) {
        getline(uchwyt, linia); //pobierz linijkę
        tmp_tab[i] = stoull(linia, NULL, 0);
    }

    uchwyt.close(); //zamykamy plik
}

void row_generator(unsigned long long a, unsigned long long row[ncols])
{
    srand(time(NULL));
    unsigned long long N[ncols-1];
    primes(N);
    jeszcze_raz:
    unsigned long long e = rand() % p;
    unsigned long long res = fastExpMod(a, e);
    for (int i = 0; i < (ncols - 1); i++)
    {
        int licznik = 0;
        while (res % N[i] == 0 && res != 0)
        {
            res = res / N[i];
            licznik++;
        }
        row[i] = licznik;
    }
    row[ncols - 1] = e;
    if (res != 1)
    {
        goto jeszcze_raz;
    }
}

void toString( bool test ){
	if( test ){
		cout << "Good" << endl;
	}else {
		cout << "Bad"<< endl;
	}
}	


# TESTY funkcji mult_p
void mult_p_test(){
	unsigned long long a,b,result;
//	unsigned long long p = 9223372036854775783;
    bool test;
	
	cout << "Starting mult_p tests..." << endl;
	
	a = 9922926131709852725;
	b = 12934490838497742223;
	result = 6270002602682420120;
	test =  (mult_p(a,b) == result);
	toString(test);

	
	a = 4701584462143343031;
	b = 8780868397663836517;
	result = 5562107464272941798;
	test =  (mult_p(a,b) == result);
	toString(test);
	
	a = 17950515727283794051;
	b = 654787098957677525;
	result = 8510628229123749504;
	test =  (mult_p(a,b) == result);
	toString(test);
	
	a = 5837595787056108429;
	b = 8505627195766366512;
	result = 7012322239095459645;
	test =  (mult_p(a,b) == result);
	toString(test);
	
	a = 6227156793379146467;
	b = 8066372097000515353;
	result = 6327858033736890011;
	test =  (mult_p(a,b) == result);
	toString(test);
	
	a = 11182028815488230963;
	b = 7949740137117062473;
	result = 3334401925302267135;
	test =  (mult_p(a,b) == result);
	toString(test);
	
	a = 13892691981017613899;
	b = 6593183929266105706;
	result = 6327858033736890011;
	test =  (mult_p(a,b) == result);
	toString(test);
	
	a = 11668836387081927041;
	b = 48;
	result = 6701824368645950988;
	test =  (mult_p(a,b) == result);
	toString(test);	
}


# TESTY funkcji mult_n
void mult_n_test(){
	unsigned long long a,b,result;
//	unsigned long long n = 9223372036854775783 - 1;
	
	
	cout << "Starting mult_n tests..." << endl;
	
	a = 9922926131709852725;
	b = 12934490838497742223;
	result = 1738776262491330641;
	test =  (mult_n(a,b) == result);
	toString(test);
	
	a = 4701584462143343031;
	b = 8780868397663836517;
	result = 814755071602430379;
	test =  (mult_n(a,b) == result);
	toString(test);
	
	a = 17950515727283794051;
	b = 654787098957677525;
	result = 561602068256257859;
	test =  (mult_n(a,b) == result);
	toString(test);
	
	a = 5837595787056108429;
	b = 8505627195766366512;
	result = 3172275659120700204;
	test =  (mult_n(a,b) == result);
	toString(test);
	
	a = 6227156793379146467;
	b = 8066372097000515353;
	result = 2550494636786188709;
	test =  (mult_n(a,b) == result);
	toString(test);
	
	a = 11182028815488230963;
	b = 7949740137117062473;
	result = 3748960889569702061;
	test =  (mult_n(a,b) == result);
	toString(test);
	
	a = 13892691981017613899;
	b = 6593183929266105706;
	result = 4401328015788414116;
	test =  (mult_n(a,b) == result);
	toString(test);
	
	a = 11668836387081927041;
	b = 48;
	result = 6701824368645951048;
	test =  (mult_n(a,b) == result);
	toString(test);
	
}


# cialo glowne funkcji
int main(int argc, char** argv)
{
    #pragma warning(disable : 4996)
    cl_device_id device_id = NULL;
    cl_context context = NULL;
    cl_command_queue command_queue = NULL;
    cl_program program = NULL;
    cl_kernel kernel = NULL;
    cl_platform_id platform_id = NULL;
    cl_uint ret_num_devices;
    cl_uint ret_num_platforms;
    cl_int error;

    char string[MEM_SIZE];

    FILE* fp;
    char fileName[] = "./vectorOperations.cl";
    char* source_str;
    size_t source_size;
    char cBuffer[1024];
    size_t size;

    /* Load the source code containing the kernel*/
    fp = fopen(fileName, "r");
    if (!fp) 
    {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose(fp);

    /* Get Platform and Device Info */
    error = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    if (error != CL_SUCCESS)
    {
        printf("Failed to get platform");
        return -1;
    }

    error = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, &ret_num_devices);
    if (error != CL_SUCCESS)
    {
        printf("Failed to get device \n");
        return -1;
    }

    clGetDeviceInfo(device_id, CL_DEVICE_NAME, sizeof(cBuffer), &cBuffer, NULL);
    printf("CL_DEVICE_NAME: %s\n", cBuffer);
    clGetDeviceInfo(device_id, CL_DRIVER_VERSION, sizeof(cBuffer), &cBuffer, NULL);
    printf("CL_DRIVER_VERSION: %s\n", cBuffer);
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size), &size, NULL);
    printf("CL_DEVICE_MAX_WORK_GROUP_SIZE: %u\n\n", size);

    /* Create OpenCL context */
    cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, (int)platform_id, 0 };
    context = clCreateContext(properties, 1, &device_id, NULL, NULL, &error);
    if (error != CL_SUCCESS)
    {
        printf("Failed to create context: ");
        error_int(error);
        return -1;
    }

    /* Create Command Queue */
    command_queue = clCreateCommandQueue(context, device_id, 0, &error);

    /* Create Kernel Program from the source */
    program = clCreateProgramWithSource(context, 1, (const char**)&source_str,
        (const size_t*)&source_size, &error);
    if (error != CL_SUCCESS)
    {
        printf("Failed to create program: ");
        error_int(error);
        return -1;
    }

    /* Build Kernel Program */
    error = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    if (error != CL_SUCCESS)
    {
        printf("Failed to build program: ");
        error_int(error);
        return -1;
    }

    /* Create OpenCL Kernel */
    cl_kernel kernelAdd = clCreateKernel(program, "vectorAdd", &error);
    if (error != CL_SUCCESS)
    {
        printf("Failed to create Kernel: ");
        error_int(error);
        return -1;
    }
    cl_kernel kernelMult = clCreateKernel(program, "vectorScalarMult", &error);
    if (error != CL_SUCCESS)
    {
        printf("Failed to create Kernel: ");
        error_int(error);
        return -1;
    }

    unsigned long long tmp[ncols];
    unsigned long long b[1];
    vector <unsigned long long*> macierz;
    unsigned long long a[1];
    for (int i = 0; i < ncols; i++)
    {
        unsigned long long row[ncols];
        row_generator(658, row);
        macierz.push_back(row);
    }
    for (int n = 0; n < ncols - 1; n++)
    {
        for (int m = 0; m < ncols; m++)
        {
            cout << macierz[n][m] << " ";
        }
        cout << endl;
    }
    cout << endl;

    size_t WorkSize[1] = { 1024 };
    cl_mem arg1 = clCreateBuffer(context, CL_MEM_READ_WRITE |
        CL_MEM_COPY_HOST_PTR, ncols * sizeof(unsigned long long), macierz[0], &error);
    if (error != CL_SUCCESS)
    {
        printf("Failed to create buffer: ");
        error_int(error);
        return -1;
    }
    cl_mem arg2 = clCreateBuffer(context, CL_MEM_READ_WRITE |
        CL_MEM_COPY_HOST_PTR, ncols * sizeof(unsigned long long), macierz[1], &error);
    if (error != CL_SUCCESS)
    {
        printf("Failed to create buffer: ");
        error_int(error);
        return -1;
    }

    srand(time(NULL));
    for (int j = 0; j < ncols-1; j++)
    {
        a[0] = mul_inv(macierz[j][j], n);
        while (a[0] == 0)
        {
            row_generator(658, macierz[j]);
            for (int k = 0; k < j; k++)
            {
                b[0] = macierz[j][k];
                error = clEnqueueWriteBuffer(command_queue, arg1, CL_TRUE, 0, ncols * sizeof(unsigned long long), macierz[k], 0, NULL, NULL);
                error = clEnqueueWriteBuffer(command_queue, arg2, CL_TRUE, 0, 1 * sizeof(unsigned long long), b, 0, NULL, NULL);
                error = clSetKernelArg(kernelMult, 0, sizeof(cl_mem), (void*)&arg1);
                error = clSetKernelArg(kernelMult, 1, sizeof(cl_mem), (void*)&arg2);
                error = clEnqueueNDRangeKernel(command_queue, kernelMult, 1, NULL, WorkSize, NULL, 0, NULL, NULL);
                error = clEnqueueReadBuffer(command_queue, arg1, CL_TRUE, 0, ncols * sizeof(unsigned long long), tmp, 0, NULL, NULL);

                error = clEnqueueWriteBuffer(command_queue, arg1, CL_TRUE, 0, ncols * sizeof(unsigned long long), macierz[j], 0, NULL, NULL);
                error = clEnqueueWriteBuffer(command_queue, arg2, CL_TRUE, 0, ncols * sizeof(unsigned long long), tmp, 0, NULL, NULL);
                error = clSetKernelArg(kernelAdd, 0, sizeof(cl_mem), (void*)&arg1);
                error = clSetKernelArg(kernelAdd, 1, sizeof(cl_mem), (void*)&arg2);
                clEnqueueNDRangeKernel(command_queue, kernelAdd, 1, NULL, WorkSize, NULL, 0, NULL, NULL);
                clEnqueueReadBuffer(command_queue, arg1, CL_TRUE, 0, ncols * sizeof(unsigned long long), macierz[j], 0, NULL, NULL);
            }
            a[0] = mul_inv(macierz[j][j], n);
        }
        error = clEnqueueWriteBuffer(command_queue, arg1, CL_TRUE, 0, ncols * sizeof(unsigned long long), macierz[j], 0, NULL, NULL);
        error = clEnqueueWriteBuffer(command_queue, arg2, CL_TRUE, 0, 1 * sizeof(unsigned long long), a, 0, NULL, NULL);
        error = clSetKernelArg(kernelMult, 0, sizeof(cl_mem), (void*)&arg1);
        if (error != CL_SUCCESS)
        {
            printf("Failed to set arg 1: ");
            error_int(error);
            return -1;
        }
        error = clSetKernelArg(kernelMult, 1, sizeof(cl_mem), (void*)&arg2);
        if (error != CL_SUCCESS)
        {
            printf("Failed to set arg 2: ");
            error_int(error);
            return -1;
        }
        error = clEnqueueNDRangeKernel(command_queue, kernelMult, 1, NULL, WorkSize, NULL, 0, NULL, NULL);
        if (error != CL_SUCCESS)
        {
            printf("Failed Mult: ");
            error_int(error);
            return -1;
        }
        error = clEnqueueReadBuffer(command_queue, arg1, CL_TRUE, 0, ncols * sizeof(unsigned long long), macierz[j], 0, NULL, NULL);
        if (error != CL_SUCCESS)
        {
            printf("Failed to read result: ");
            error_int(error);
            return -1;
        }
        for (int i = 0; i < macierz.size(); i++)
        {
            if (j != i)
            {
                a[0] = macierz[i][j];
                error = clEnqueueWriteBuffer(command_queue, arg1, CL_TRUE, 0, ncols * sizeof(unsigned long long), macierz[j], 0, NULL, NULL);
                error = clEnqueueWriteBuffer(command_queue, arg2, CL_TRUE, 0, 1 * sizeof(unsigned long long), a, 0, NULL, NULL);
                error = clSetKernelArg(kernelMult, 0, sizeof(cl_mem), (void*)&arg1);
                if (error != CL_SUCCESS)
                {
                    printf("Failed to set arg 1: ");
                    error_int(error);
                    return -1;
                }
                error = clSetKernelArg(kernelMult, 1, sizeof(cl_mem), (void*)&arg2);
                if (error != CL_SUCCESS)
                {
                    printf("Failed to set arg 2: ");
                    error_int(error);
                    return -1;
                }
                error = clEnqueueNDRangeKernel(command_queue, kernelMult, 1, NULL, WorkSize, NULL, 0, NULL, NULL);
                if (error != CL_SUCCESS)
                {
                    printf("Failed Mult: ");
                    error_int(error);
                    return -1;
                }
                error = clEnqueueReadBuffer(command_queue, arg1, CL_TRUE, 0, ncols * sizeof(unsigned long long), tmp, 0, NULL, NULL);
                if (error != CL_SUCCESS)
                {
                    printf("Failed to read result: ");
                    error_int(error);
                    return -1;
                }
                error = clEnqueueWriteBuffer(command_queue, arg1, CL_TRUE, 0, ncols * sizeof(unsigned long long), macierz[i], 0, NULL, NULL);
                error = clEnqueueWriteBuffer(command_queue, arg2, CL_TRUE, 0, ncols * sizeof(unsigned long long), tmp, 0, NULL, NULL);
                error = clSetKernelArg(kernelAdd, 0, sizeof(cl_mem), (void*)&arg1);
                error = clSetKernelArg(kernelAdd, 1, sizeof(cl_mem), (void*)&arg2);
                clEnqueueNDRangeKernel(command_queue, kernelAdd, 1, NULL, WorkSize, NULL, 0, NULL, NULL);
                clEnqueueReadBuffer(command_queue, arg1, CL_TRUE, 0, ncols * sizeof(unsigned long long), macierz[i], 0, NULL, NULL);
            }
        }
    }
    for (int n = 0; n < ncols - 1; n++)
    {
        for (int m = 0; m < ncols; m++)
        {
            cout << macierz[n][m] << " ";
        }
        cout << endl;
    }
    cout << endl;
    /* Finalization */
    error = clFlush(command_queue);
    error = clFinish(command_queue);
    error = clReleaseKernel(kernel);
    error = clReleaseProgram(program);
    error = clReleaseMemObject(arg1);
    error = clReleaseMemObject(arg2);
    error = clReleaseCommandQueue(command_queue);
    error = clReleaseContext(context);

    free(source_str);


}
