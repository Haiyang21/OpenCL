#ifndef DEVICE_HPP
#define DEVICE_HPP
#include <string>
#include <fstream>
#include <map>
#include <CL/cl.h>
#include <iostream>

#define OCL_CHECK(condition, content) \
do {\
	cl_int error = condition; \
	if (error != CL_SUCCESS) std::cout << "error: " << error << " [ " << content << " ]" << std::endl; \
} while (0)


class Device {
  public:
    Device()
        : numPlatforms(0), numDevices(0), device_id(INT_MIN), oclKernelPath("./kernelGen/cl_kernels/"), buildOption(" ") {
    }
    ~Device();
    cl_uint numPlatforms;
    cl_platform_id * platformIDs;
    char platformName[64];
    char openclVersion[64];
    cl_uint numDevices;
    cl_device_id * DeviceIDs;

    cl_context Context;
    cl_command_queue CommandQueue;
    cl_command_queue CommandQueue_helper;
    cl_program Program;
    cl_device_id * pDevices;
    int device_id;
	std::string oclKernelPath;
	std::string buildOption;

    std::map<std::string, cl_kernel> Kernels;

    cl_int Init(int device_id = -1);
    cl_int ConvertToString(std::string pFileName, std::string &Str);
	cl_kernel GetKernel(std::string kernel_name);
    void DisplayPlatformInfo();
    void DisplayInfo(cl_platform_id id, cl_platform_info name, std::string str);

	int GetDevice() { return device_id; }
    void GetDeviceInfo();
    void DeviceQuery();    
    void BuildProgram(std::string kernel_dir);
	bool SetKernelPath(std::string path);
	bool SetBuildOption(std::string option);

    template <typename T>
    void DisplayDeviceInfo(cl_device_id id, cl_device_info name, std::string str);
    template <typename T>
    void appendBitfield(T info, T value, std::string name, std::string &str);

    void ReleaseKernels();
}; 

#endif //DEVICE_HPP

