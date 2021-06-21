//************************************************************

// Demo OpenCL application to compute a simple vector addition

// computation between 2 arrays on the GPU

// ************************************************************

#include <stdio.h>

#include <stdlib.h>

#include <CL/cl.h>


//

// OpenCL source code

const char* OpenCLSourceAdd[] = {

"__kernel void VectorAdd(__global int* c, __global int* a,__global int* b)",

"{",

" // Index of the elements to add \n",

" unsigned int n = get_global_id(0);",

" // Sum the nth element of vectors a and b and store in c \n",

" c[n] = a[n] + b[n];",

"}"

};

const char* OpenCLSourceSwap[] = {

"__kernel void VectorSwap(__global int* a, __global int* b)",

"{",

" // Index of the elements to add \n",

" unsigned int n = get_global_id(0);",

" int n = tmp;",

" tmp = a[n];",

" a[n] = b[n];",

" b[n] = tmp;",

"}"

};


const char* OpenCLSourceSub[] = {

"__kernel void VectorSub(__global int* c, __global int* a,__global int* b)",

"{",

" // Index of the elements to add \n",

" unsigned int n = get_global_id(0);",

" // Sum the nth element of vectors a and b and store in c \n",

" c[n] = a[n] - b[n];",

"}"

};


const char* OpenCLSourceScalarMult[] = {

"__kernel void VectorScalarMult(__global int* c, __global int* a,__global int s)",

"{",

" // Index of the elements to add \n",

" unsigned int n = get_global_id(0);",

" // Sum the nth element of vectors a and b and store in c \n",

" c[n] = a[n] * s;",

"}"

};


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
    default:
        printf("Other error\n");
    }
}

// Some interesting data for the vectors

int InitialData1[20] = { 37,50,54,50,56,0,43,43,74,71,32,36,16,43,56,100,50,25,15,17 };

int InitialData2[20] = { 35,51,54,58,55,32,36,69,27,39,35,40,16,44,55,14,58,75,18,15 };

// Number of elements in the vectors to be added

#define SIZE 100

cl_platform_id get_cl_platfrom_id()
{
    cl_int error;
    cl_platform_id cpPlatform;
    error = clGetPlatformIDs(1, &cpPlatform, NULL);
    if (error != CL_SUCCESS)
    {
        printf("Failed to get platform");
    }
    return cpPlatform;
}

cl_device_id get_cl_device_id(cl_platform_id cpPlatform)
{
    cl_int error;
    cl_device_id cdDevice;
    error = clGetDeviceIDs(cpPlatform, CL_DEVICE_TYPE_GPU, 1, &cdDevice, NULL);
    if (error != CL_SUCCESS)
    {
        printf("Failed to get device \n");
    }
    return cdDevice;
}

void get_cl_device_info(cl_device_id cdDevice)
{
    char cBuffer[1024];
    size_t size;
    clGetDeviceInfo(cdDevice, CL_DEVICE_NAME, sizeof(cBuffer), &cBuffer, NULL);
    printf("CL_DEVICE_NAME: %s\n", cBuffer);
    clGetDeviceInfo(cdDevice, CL_DRIVER_VERSION, sizeof(cBuffer), &cBuffer, NULL);
    printf("CL_DRIVER_VERSION: %s\n\n", cBuffer);
    clGetDeviceInfo(cdDevice, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size), &size, NULL);
    printf("CL_DEVICE_MAX_WORK_GROUP_SIZE: %u\n\n", size);
}

size_t get_max_work_group_size(cl_device_id cdDevice)
{
    size_t size;
    clGetDeviceInfo(cdDevice, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size), &size, NULL);
    return size;
}

cl_context get_cl_gpu_context(cl_platform_id cpPlatform)
{
    cl_int error;
    cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, (int)cpPlatform, 0 };
    cl_context GPUContext = clCreateContextFromType(properties, CL_DEVICE_TYPE_ALL, &pfn_notify, NULL, &error);
    if (error != CL_SUCCESS)
    {
        printf("Failed to create context: ");
        error_int(error);
    }
    return GPUContext;
}

cl_mem get_writing_buffer(cl_context GPUContext, int *HostVector, int size)
{
    cl_mem GPUVector = clCreateBuffer(GPUContext, CL_MEM_READ_ONLY |
        CL_MEM_COPY_HOST_PTR, sizeof(int) * size, HostVector, NULL);
    return GPUVector;
}

cl_mem get_reading_buffer(cl_context GPUContext, int size)
{
    cl_mem GPUOutputVector = clCreateBuffer(GPUContext, CL_MEM_WRITE_ONLY,
        sizeof(int) * size, NULL, NULL);
    return GPUOutputVector;
}

cl_kernel get_cl_kernel(cl_context GPUContext, const char* source[], const char* name)
{
    cl_int error;
    cl_program OpenCLProgram = clCreateProgramWithSource(GPUContext, 7, source, NULL, &error);
    if (error != CL_SUCCESS)
    {
        printf("Failed to create program");
    }
    error = clBuildProgram(OpenCLProgram, 0, NULL, NULL, NULL, NULL);
    if (error != CL_SUCCESS)
    {
        printf("Failed to build program");
    }
    cl_kernel OpenKernel = clCreateKernel(OpenCLProgram, name, NULL);
    clReleaseProgram(OpenCLProgram);
    return OpenKernel;
}

int main(int argc, char** argv)
{
    #pragma warning(disable : 4996)
    cl_int error;
    // Two integer source vectors in Host memory
    int HostVector1[SIZE], HostVector2[SIZE];
    //Output Vector
    int HostOutputVector[SIZE];

    // Initialize with some interesting repeating data
    for (int c = 0; c < SIZE; c++)
    {
        HostVector1[c] = InitialData1[c % 20];
        HostVector2[c] = InitialData2[c % 20];
        HostOutputVector[c] = 0;
    }

    //Get an OpenCL platform

    cl_platform_id cpPlatform = get_cl_platfrom_id();

    // Get a GPU device
    cl_device_id cdDevice = get_cl_device_id(cpPlatform);
    size_t max_work_group_size = get_max_work_group_size(cdDevice);

    get_cl_device_info(cdDevice);

    // Create a context to run OpenCL enabled GPU
    cl_context GPUContext = get_cl_gpu_context(cpPlatform);

    // Create a command-queue on the GPU device
    cl_command_queue cqCommandQueue = clCreateCommandQueue(GPUContext, cdDevice, 0, NULL);

    // Allocate GPU memory for source vectors AND initialize from CPU memory

    cl_mem GPUVector1 = get_writing_buffer(GPUContext, HostVector1, 100);
    cl_mem GPUVector2 = get_writing_buffer(GPUContext, HostVector2, 100);

    // Allocate output memory on GPU
    cl_mem GPUOutputVector = get_reading_buffer(GPUContext, 100);

    // Create OpenCL program with source code
  
    cl_kernel OpenCLVectorAdd = get_cl_kernel(GPUContext, OpenCLSourceSub, "VectorSub");

    // In the next step we associate the GPU memory with the Kernel arguments

    clSetKernelArg(OpenCLVectorAdd, 0, sizeof(cl_mem), (void*)&GPUOutputVector);
    clSetKernelArg(OpenCLVectorAdd, 1, sizeof(cl_mem), (void*)&GPUVector1);
    clSetKernelArg(OpenCLVectorAdd, 2, sizeof(cl_mem), (void*)&GPUVector2);

    // Launch the Kernel on the GPU

    // This kernel only uses global data

    size_t WorkSize[1] = { SIZE }; // one dimensional Range

    clEnqueueNDRangeKernel(cqCommandQueue, OpenCLVectorAdd, 1, NULL,
        WorkSize, NULL, 0, NULL, NULL);

    // Copy the output in GPU memory back to CPU memory
    clEnqueueReadBuffer(cqCommandQueue, GPUOutputVector, CL_TRUE, 0,
        SIZE * sizeof(int), HostOutputVector, 0, NULL, NULL);

    // Cleanup
    clReleaseKernel(OpenCLVectorAdd);
    clReleaseCommandQueue(cqCommandQueue);
    clReleaseContext(GPUContext);
    clReleaseMemObject(GPUVector1);
    clReleaseMemObject(GPUVector2);
    clReleaseMemObject(GPUOutputVector);

    for (int i = 0; i < SIZE; i++)
        printf("[%d - %d = %d]\n", HostVector1[i], HostVector2[i], HostOutputVector[i]);
    
    return 0;

}
