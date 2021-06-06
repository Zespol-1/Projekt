#include <stdio.h>

#include <stdlib.h>

#include <iostream>

#include <CL/cl.h>

#include <vector>

using namespace std;
#define MEM_SIZE (128)
#define MAX_SOURCE_SIZE (0x100000)
#define ncols 5


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

uint64_t product_mult32_reduce(uint64_t tbr) 
{
    unsigned long p = 9223372036854775783;
    tbr = tbr % p;
    uint64_t x1 = tbr >> 32;
    uint64_t y1 = tbr & 0xffffffff;
    uint64_t res = ((y1 << 32) % p) + x1 * 50;
    return res % p;
}

uint64_t product_mult64_reduce(uint64_t tbr) 
{
    unsigned long p = 9223372036854775783;
    tbr = tbr % p;
    uint64_t x1 = tbr >> 32;
    uint64_t y1 = tbr & 0xffffffff;
    uint64_t res = product_mult32_reduce(x1 * 50) + y1 * 50;
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

unsigned long long mult(unsigned long long a, unsigned long long b) 
{
    unsigned long p = 9223372036854775783;
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

    unsigned long long row1[5] = { 1, 2, 3, 4, 5 };
    unsigned long long row2[5] = { 4, 5, 6, 7, 2 };
    unsigned long long row3[5] = { 5, 7, 9, 8, 3 };
    unsigned long long row4[5] = { 6, 7, 10, 8, 4 };
    unsigned long long row5[5] = { 7, 7, 10, 8, 1 };
    unsigned long long tmp[5];
    unsigned long p = 9223372036854775783;

    vector <unsigned long long*> macierz;
    unsigned long long a[1];
    macierz.push_back(row1);
    macierz.push_back(row2);
    macierz.push_back(row3);
    macierz.push_back(row4);
    macierz.push_back(row5);

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

    for (int j = 0; j < ncols; j++)
    {
        for (int i = 0; i < macierz.size(); i++)
        {
            if (j != i)
            {
                a[0] = mult(macierz[i][j], mul_inv(macierz[j][j],p));
                cout << mult(macierz[j][j], mul_inv(macierz[j][j], p)) << endl;
                error = clEnqueueWriteBuffer(command_queue, arg1, CL_TRUE, 0, ncols * sizeof(unsigned long long), macierz[i], 0, NULL, NULL);
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
            for (int k = 0; k < 5; k++)
            {
                cout << macierz[i][k] << " ";
            }
            cout << endl;
        }
        cout << endl << endl;
    }
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
