#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <CL/cl.h>
#include <vector>
#include <fstream>
#include <string>
#include <random>


#define MEM_SIZE (128)
#define MAX_SOURCE_SIZE (0x100000)
#define ncols 1024

//--------------Liczby--------------
unsigned long long p = 9223372036854775783;
unsigned long long n = p - 1;

using namespace std;

//--------------Urzadzenia--------------
cl_device_id device_id = NULL;
cl_context context = NULL;
cl_command_queue command_queue = NULL;
cl_program program = NULL;
cl_kernel kernelReduce = NULL;
cl_kernel kernelMult = NULL;
cl_platform_id platform_id = NULL;
cl_uint ret_num_devices;
cl_uint ret_num_platforms;
cl_int error;
cl_mem arg1, arg2;
size_t WorkSize[1] = { 1024 };

//----------Generator liczb losowych----------
class RandomGen
{
public:
    RandomGen()
    {
        random_device rd;
        mt = mt19937_64(rd());
        srand(time(NULL));
    }
    unsigned long long random()
    {
        unsigned long long m = dist(mt);
        return m;
    }
private:
    mt19937_64 mt;
    uniform_int_distribution<unsigned long long> dist;
}; RandomGen randomGen;

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

int set_up()
{
#pragma warning(disable : 4996)
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
        return -1;
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
    kernelReduce = clCreateKernel(program, "vectorReduce", &error);
    if (error != CL_SUCCESS)
    {
        printf("Failed to create Kernel: ");
        error_int(error);
        return -1;
    }
    kernelMult = clCreateKernel(program, "vectorScalarMult", &error);
    if (error != CL_SUCCESS)
    {
        printf("Failed to create Kernel: ");
        error_int(error);
        return -1;
    }
    return 0;
}

int create_buffers()
{
    arg1 = clCreateBuffer(context, CL_MEM_READ_WRITE, ncols * sizeof(unsigned long long), NULL, &error);
    if (error != CL_SUCCESS)
    {
        printf("Failed to create buffer: ");
        error_int(error);
        return -1;
    }
    arg2 = clCreateBuffer(context, CL_MEM_READ_WRITE, ncols * sizeof(unsigned long long), NULL, &error);
    if (error != CL_SUCCESS)
    {
        printf("Failed to create buffer: ");
        error_int(error);
        return -1;
    }
    return 0;
}

int write_buffer(cl_mem arg, unsigned long long* data)
{
    error = clEnqueueWriteBuffer(command_queue, arg, CL_FALSE, 0, ncols * sizeof(unsigned long long), data, 0, NULL, NULL);
    if (error != CL_SUCCESS)
    {
        printf("Failed to write arg: ");
        error_int(error);
        return -1;
    }
    return 0;
}

vector<unsigned long long> read_buffer(cl_mem arg)
{
    vector<unsigned long long> tmp(ncols, 0);
    error = clEnqueueReadBuffer(command_queue, arg1, CL_FALSE, 0, ncols * sizeof(unsigned long long), tmp.data(), 0, NULL, NULL);
    if (error != CL_SUCCESS)
    {
        printf("Failed to read result: ");
        error_int(error);
    }
    return tmp;
}

//-------------Pomocnicze funkcje-------------
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

typedef struct tag_uint64AndSign
{
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
    if (a == 0)
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

//-------------Rzeczy-----------------
vector <unsigned long long> read_primes()
{
    ifstream f("first_2048_prime_numbers.txt");
    vector<unsigned long long> v;

    int i = ncols - 1;
    while (i--)
    {
        int x;
        f >> x;
        v.push_back(x);
    }

    cout << "Pomyslnie odczytano " << v.size() << " liczb pierwszych " << endl;
    cout << "Ostatnia liczba: " << v.at(v.size() - 1) << "\n";
    cout << "Pierwsza liczba: " << v.at(0) << "\n";

    return v;
}

vector<unsigned long long> row_generator(unsigned long long a, vector<unsigned long long>& primes)
{
    bool is_set = false;
    vector<unsigned long long> row(ncols, 0);
    while (!is_set)
    {
        unsigned long long e = randomGen.random() % p;
        unsigned long long res = fastExpMod(a, e);
        for (int i = 0; i < (ncols - 1); i++)
        {
            int licznik = 0;
            while (res % primes[i] == 0 && res != 0)
            {
                res = res / primes[i];
                licznik++;
            }
            row[i] = licznik;
        }
        row[ncols - 1] = e;
        if (res == 1)
        {
            is_set = true;
            return row;
        }
    }
}

vector<vector<unsigned long long>> matrix_gen(unsigned long long a, vector<unsigned long long>& primes)
{
    vector<vector<unsigned long long>> matrix;
    for (int i = 0; i < ncols; i++)
    {
        matrix.push_back(row_generator(a, primes));
        cout << "Pozostalo: " << ncols - i << endl;
    }
    return matrix;
}

vector<unsigned long long> row_reduce(vector<unsigned long long> row_row_counter, vector<unsigned long long> row_column_counter, unsigned long long r)
{
    vector<unsigned long long> tmp;
    int res;
    res = write_buffer(arg1, row_row_counter.data());
    res = write_buffer(arg2, row_column_counter.data());
    error = clSetKernelArg(kernelReduce, 0, sizeof(cl_mem), (void*)&arg1);
    if (error != CL_SUCCESS)
    {
        printf("Failed to set arg 1: ");
        error_int(error);
    }
    error = clSetKernelArg(kernelReduce, 1, sizeof(cl_mem), (void*)&arg2);
    if (error != CL_SUCCESS)
    {
        printf("Failed to set arg 2: ");
        error_int(error);
    }
    error = clSetKernelArg(kernelReduce, 2, sizeof(unsigned long long), (void*)&r);
    if (error != CL_SUCCESS)
    {
        printf("Failed to set arg 3: ");
        error_int(error);
    }
    error = clEnqueueNDRangeKernel(command_queue, kernelReduce, 1, NULL, WorkSize, NULL, 0, NULL, NULL);
    if (error != CL_SUCCESS)
    {
        printf("Failed Reduce: ");
        error_int(error);
    }
    tmp = read_buffer(arg1);
    return tmp;
}

vector<unsigned long long> row_mult(vector<unsigned long long> row, unsigned long long scalar)
{
    int res;
    vector<unsigned long long> tmp;
    res = write_buffer(arg1, row.data());
    error = clSetKernelArg(kernelMult, 0, sizeof(cl_mem), (void*)&arg1);
    if (error != CL_SUCCESS)
    {
        printf("Failed to set arg 1: ");
        error_int(error);
    }
    error = clSetKernelArg(kernelMult, 1, sizeof(unsigned long long), (void*)&scalar);
    if (error != CL_SUCCESS)
    {
        printf("Failed to set arg 2: ");
        error_int(error);
    }
    error = clEnqueueNDRangeKernel(command_queue, kernelMult, 1, NULL, WorkSize, NULL, 0, NULL, NULL);
    if (error != CL_SUCCESS)
    {
        printf("Failed Reduce: ");
        error_int(error);
    }
    tmp = read_buffer(arg1);
    return tmp;
}

vector<unsigned long long> gauss_reduction(vector<vector<unsigned long long>> matrix, vector<unsigned long long>& primes, unsigned long long gen)
{
    vector<unsigned long long> result;
    for (int column_counter = 0; column_counter < ncols - 1; column_counter++)
    {
        unsigned long long a = mul_inv(matrix[column_counter][column_counter], n);
        while (a == 0)
        {
            int k = column_counter + 1;
            while ((k < matrix.size()) && (mul_inv(matrix[k][column_counter], n) == 0))
            {
                k++;
            }
            if (k == matrix.size())
            {
                cout << "Poszukiwanie dodatkowych relacji dla kolumny " << column_counter << " (liczba " << primes[column_counter] << ")" << endl;
                for (int l = 0; l < 100; l++)
                {
                    vector<unsigned long long> tmp = row_generator(gen, primes);
                    matrix.push_back(tmp);
                }
                cout << "Dolozono 100 relacacji " << endl;
            }
            else
            {
                matrix[column_counter].swap(matrix[k]);
                cout << "Zamiana wiersza " << column_counter << " z " << k << endl;
                for (int tmp_row_counter = 0; tmp_row_counter < column_counter; tmp_row_counter++)
                {
                    matrix[column_counter] = row_reduce(matrix[column_counter], matrix[tmp_row_counter], matrix[column_counter][tmp_row_counter]);
                }
                a = mul_inv(matrix[column_counter][column_counter], n);
            }
        }
        matrix[column_counter] = row_mult(matrix[column_counter], a);
        for (int row_counter = 0; row_counter < ncols - 1; row_counter++)
        {
            if (row_counter != column_counter)
            {
                matrix[row_counter] = row_reduce(matrix[row_counter], matrix[column_counter], matrix[row_counter][column_counter]);
            }
        }
        cout << "Pozostalo " << ncols - 1 - column_counter << " kolumn" << endl;
    }

    for (int i = 0; i < ncols; i++)
    {
        result.push_back(matrix[i][ncols - 1]);
    }
    return result;
}

void row_reduce_test()
{
    vector<unsigned long long> row1, row2, res;
    for (int i = 0; i < 10; i++)
    {
        row1.push_back(i);
        row2.push_back(2 * i + 1);
    }
    res = row_reduce(row2, row1, 2);
    for (int k = 0; k < 10; k++)
    {
        cout << res[k] << " ";
    }
}

void row_mult_test()
{
    vector<unsigned long long> row, res;
    for (int i = 0; i < 10; i++)
    {
        row.push_back(i);
    }
    res = row_mult(row, 2);
    for (int k = 0; k < 10; k++)
    {
        cout << res[k] << " ";
    }
}

void mult_p_test()
{
    unsigned long long a, b, result;
    int counter = 0; 
    a = 9922926131709852725;
    b = 12934490838497742223;
    result = 6270002602682420120;
    cout << "Test 1" << endl;
    if (mult_p(a, b) == result)
    {
        counter++;
        cout << "Sukces" << endl;
    }
    cout << "Test 2" << endl;
    a = 4701584462143343031;
    b = 8780868397663836517;
    result = 5562107464272941798;
    if (mult_p(a, b) == result)
    {
        counter++;
        cout << "Sukces" << endl;
    }
    cout << "Test 3" << endl;
    a = 17950515727283794051;
    b = 654787098957677525;
    result = 8510628229123749504;
    if (mult_p(a, b) == result)
    {
        counter++;
        cout << "Sukces" << endl;
    }
    cout << "Test 4" << endl;
    a = 5837595787056108429;
    b = 8505627195766366512;
    result = 7012322239095459645;
    if (mult_p(a, b) == result)
    {
        counter++;
        cout << "Sukces" << endl;
    }
    cout << "Test 5" << endl;
    a = 6227156793379146467;
    b = 8066372097000515353;
    result = 6327858033736890011;
    if (mult_p(a, b) == result)
    {
        counter++;
        cout << "Sukces" << endl;
    }
    cout << "Test 6" << endl;
    a = 11182028815488230963;
    b = 7949740137117062473;
    result = 3334401925302267135;
    if (mult_p(a, b) == result)
    {
        counter++;
        cout << "Sukces" << endl;
    }
    cout << "Test 7" << endl;
    a = 13892691981017613899;
    b = 6593183929266105706;
    result = 3693725444063422420;
    if (mult_p(a, b) == result)
    {
        counter++;
        cout << "Sukces" << endl;
    }
    cout << "Test 8" << endl;
    a = 11668836387081927041;
    b = 48;
    result = 6701824368645950988;
    if (mult_p(a, b) == result)
    {
        counter++;
        cout << "Sukces" << endl;
    }
    cout << "Liczba testow zakonczona sukcesem: " << counter << "/8" << endl;
}

void mult_n_test()
{
    unsigned long long a, b, result;
    int counter = 0;
    a = 9922926131709852725;
    b = 12934490838497742223;
    result = 1738776262491330641;
    cout << "Test 1" << endl;
    if (mult_n(a, b) == result)
    {
        counter++;
        cout << "Sukces" << endl;
    }

    a = 4701584462143343031;
    b = 8780868397663836517;
    result = 814755071602430379;
    cout << "Test 2" << endl;
    if (mult_n(a, b) == result)
    {
        counter++;
        cout << "Sukces" << endl;
    }

    a = 17950515727283794051;
    b = 654787098957677525;
    result = 561602068256257859;
    cout << "Test 3" << endl;
    if (mult_n(a, b) == result)
    {
        counter++;
        cout << "Sukces" << endl;
    }

    a = 5837595787056108429;
    b = 8505627195766366512;
    result = 3172275659120700204;
    cout << "Test 4" << endl;
    if (mult_n(a, b) == result)
    {
        counter++;
        cout << "Sukces" << endl;
    }

    a = 6227156793379146467;
    b = 8066372097000515353;
    result = 2550494636786188709;
    cout << "Test 5" << endl;
    if (mult_n(a, b) == result)
    {
        counter++;
        cout << "Sukces" << endl;
    }

    a = 11182028815488230963;
    b = 7949740137117062473;
    result = 3748960889569702061;
    cout << "Test 6" << endl;
    if (mult_n(a, b) == result)
    {
        counter++;
        cout << "Sukces" << endl;
    }

    a = 13892691981017613899;
    b = 6593183929266105706;
    result = 4401328015788414116;
    cout << "Test 7" << endl;
    if (mult_n(a, b) == result)
    {
        counter++;
        cout << "Sukces" << endl;
    }

    a = 11668836387081927041;
    b = 48;
    result = 6701824368645951048;
    cout << "Test 8" << endl;
    if (mult_n(a, b) == result)
    {
        counter++;
        cout << "Sukces" << endl;
    }
    cout << "Liczba testow zakonczona sukcesem: " << counter << "/8" << endl;
}

void mul_inv_test()
{
    unsigned long long a;
    int counter = 0;
    a = 9922926131709852725;
    cout << "Test 1" << endl;
    if (mul_inv(a, n) == 393971022975603563)
    {
        counter++;
        cout << "Sukces" << endl;
    }

    a = 4701584462143343031;
    cout << "Test 2" << endl;
    if (mul_inv(a, n) == 0)
    {
        counter++;
        cout << "Sukces" << endl;
    }

    a = 17950515727283794051;
    cout << "Test 3" << endl;
    if (mul_inv(a, n) == 4529990210987414101)
    {
        counter++;
        cout << "Sukces" << endl;
    }

    a = 5837595787056108429;
    cout << "Test 4" << endl;
    if (mul_inv(a, n) == 0)
    {
        counter++;
        cout << "Sukces" << endl;
    }

    a = 6227156793379146467;
    cout << "Test 5" << endl;
    if (mul_inv(a, n) == 313726998112735169)
    {
        counter++;
        cout << "Sukces" << endl;
    }

    a = 11182028815488230963;
    cout << "Test 6" << endl;
    if (mul_inv(a, n) == 7715557655632627055)
    {
        counter++;
        cout << "Sukces" << endl;
    }

    a = 13892691981017613899;
    cout << "Test 7" << endl;
    if (mul_inv(a, n) == 2226176799301441985)
    {
        counter++;
        cout << "Sukces" << endl;
    }

    a = 11668836387081927041;
    cout << "Test 8" << endl;
    if (mul_inv(a, n) == 8007805973839656239)
    {
        counter++;
        cout << "Sukces" << endl;
    }
    cout << "Liczba testow zakonczona sukcesem: " << counter << "/8" << endl;
}

void row_gen_test()
{
    vector<unsigned long long> primes = read_primes();
    unsigned long long result;
    vector<unsigned long long> R = row_generator(3, primes);
    int counter = 0;
    for (int j = 0; j < 15; j++)
    {
        unsigned long long tmp = 1;
        for (int i = 0; i < ncols - 1; i++)
        {
            unsigned long long e = fastExpMod(primes[i], R[i]);
            tmp = mult_p(tmp, e);
        }
        result = fastExpMod(3, R[ncols - 1]);
        cout << "Test " << j << endl;
        if (result == tmp)
        {
            counter++;
            cout << "Dziala" << endl;
        }
    }
    cout << "Liczba testow zakonczona sukcesem: " << counter << "/15" << endl;
}

int main()
{

    int set = set_up();
    create_buffers();
    vector<unsigned long long> primes = read_primes();
    vector<unsigned long long> result;
    vector<vector<unsigned long long>> matrix = matrix_gen(3, primes);
    result = gauss_reduction(matrix, primes, 3);
    return 0;
}
