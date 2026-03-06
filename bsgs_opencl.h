// bsgs_opencl.h - OpenCL GPU Acceleration for BSGS
#ifndef BSGS_OPENCL_H
#define BSGS_OPENCL_H

#ifdef USE_OPENCL

#include <CL/cl.h>
#include <vector>
#include <string>
#include <iostream>
#include <cstring>

// Simplified OpenCL BSGS Accelerator
class BSGSOpenCLAccelerator {
private:
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    
    cl_mem d_table;
    cl_mem d_foundI;
    cl_mem d_foundJ;
    cl_mem d_foundFlag;
    
    cl_uint tableSize;
    bool initialized;
    
public:
    BSGSOpenCLAccelerator() {
        platform = 0;
        device = 0;
        context = 0;
        queue = 0;
        program = 0;
        kernel = 0;
        d_table = 0;
        d_foundI = 0;
        d_foundJ = 0;
        d_foundFlag = 0;
        tableSize = 0;
        initialized = false;
    }
    
    ~BSGSOpenCLAccelerator() {
        cleanup();
    }
    
    bool initialize(uint64_t m) {
        cl_int err;
        
        cl_uint numPlatforms;
        err = clGetPlatformIDs(1, &platform, &numPlatforms);
        if (err != CL_SUCCESS || numPlatforms == 0) {
            std::cerr << "[OpenCL] No platform found\n";
            return false;
        }
        
        cl_uint numDevices;
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, &numDevices);
        if (err != CL_SUCCESS || numDevices == 0) {
            err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, &numDevices);
            if (err != CL_SUCCESS) {
                std::cerr << "[OpenCL] No device found\n";
                return false;
            }
        }
        
        context = clCreateContext(0, 1, &device, 0, 0, &err);
        if (err != CL_SUCCESS) {
            std::cerr << "[OpenCL] Failed to create context\n";
            return false;
        }
        
        #if defined(CL_VERSION_2_0)
        queue = clCreateCommandQueueWithProperties(context, device, 0, &err);
        #else
        queue = clCreateCommandQueue(context, device, 0, &err);
        #endif
        
        if (err != CL_SUCCESS) {
            std::cerr << "[OpenCL] Failed to create queue\n";
            return false;
        }
        
        tableSize = (cl_uint)(m * 2);
        if (tableSize > 50000000) tableSize = 50000000;
        
        size_t tableBytes = tableSize * 8 * sizeof(cl_ulong);
        
        d_table = clCreateBuffer(context, CL_MEM_READ_WRITE, tableBytes, 0, &err);
        d_foundI = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_ulong), 0, &err);
        d_foundJ = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_ulong), 0, &err);
        d_foundFlag = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_int), 0, &err);
        
        cl_int zero = 0;
        clEnqueueWriteBuffer(queue, d_foundFlag, CL_TRUE, 0, sizeof(cl_int), &zero, 0, 0, 0);
        
        std::cout << "[OpenCL] Initialized: " << tableSize << " entries\n";
        
        initialized = true;
        return true;
    }
    
    bool launchGiantSteps(uint64_t startI, uint64_t count, size_t localSize) {
        if (!initialized) return false;
        
        cl_int zero = 0;
        clEnqueueWriteBuffer(queue, d_foundFlag, CL_TRUE, 0, sizeof(cl_int), &zero, 0, 0, 0);
        
        size_t globalSize = ((count + localSize - 1) / localSize) * localSize;
        if (globalSize > 1000000) globalSize = 1000000;
        
        // Kernel launch would go here with proper kernel
        // For now, simplified placeholder
        
        clFinish(queue);
        return true;
    }
    
    bool checkResult(uint64_t& outI, uint64_t& outJ) {
        if (!initialized) return false;
        
        cl_int flag = 0;
        clEnqueueReadBuffer(queue, d_foundFlag, CL_TRUE, 0, sizeof(cl_int), &flag, 0, 0, 0);
        
        if (flag) {
            cl_ulong i = 0, j = 0;
            clEnqueueReadBuffer(queue, d_foundI, CL_TRUE, 0, sizeof(cl_ulong), &i, 0, 0, 0);
            clEnqueueReadBuffer(queue, d_foundJ, CL_TRUE, 0, sizeof(cl_ulong), &j, 0, 0, 0);
            outI = i;
            outJ = j;
            return true;
        }
        
        return false;
    }
    
    void synchronize() {
        if (queue) clFinish(queue);
    }
    
    void cleanup() {
        if (d_table) clReleaseMemObject(d_table);
        if (d_foundI) clReleaseMemObject(d_foundI);
        if (d_foundJ) clReleaseMemObject(d_foundJ);
        if (d_foundFlag) clReleaseMemObject(d_foundFlag);
        if (kernel) clReleaseKernel(kernel);
        if (program) clReleaseProgram(program);
        if (queue) clReleaseCommandQueue(queue);
        if (context) clReleaseContext(context);
        
        initialized = false;
    }
    
    static void printDeviceInfo() {
        cl_uint numPlatforms = 0;
        clGetPlatformIDs(0, 0, &numPlatforms);
        
        std::cout << "[OpenCL] " << numPlatforms << " platform(s)\n";
        
        for (cl_uint i = 0; i < numPlatforms; i++) {
            cl_platform_id plat;
            clGetPlatformIDs(i + 1, &plat, 0);
            
            char name[256], vendor[256];
            clGetPlatformInfo(plat, CL_PLATFORM_NAME, sizeof(name), name, 0);
            clGetPlatformInfo(plat, CL_PLATFORM_VENDOR, sizeof(vendor), vendor, 0);
            
            std::cout << "[OpenCL] Platform " << i << ": " << name << " (" << vendor << ")\n";
            
            cl_uint numDevices = 0;
            clGetDeviceIDs(plat, CL_DEVICE_TYPE_ALL, 0, 0, &numDevices);
            
            std::vector<cl_device_id> devices(numDevices);
            clGetDeviceIDs(plat, CL_DEVICE_TYPE_ALL, numDevices, &devices[0], 0);
            
            for (cl_uint j = 0; j < numDevices; j++) {
                char devName[256];
                cl_ulong memSize = 0;
                clGetDeviceInfo(devices[j], CL_DEVICE_NAME, sizeof(devName), devName, 0);
                clGetDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(memSize), &memSize, 0);
                
                std::cout << "[OpenCL]   Device " << j << ": " << devName 
                          << " (" << memSize / (1024*1024) << " MB)\n";
            }
        }
    }
};

#endif // USE_OPENCL

#endif // BSGS_OPENCL_H
