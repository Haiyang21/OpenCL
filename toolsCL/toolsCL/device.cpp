#include "device.hpp"
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <ostream>
#include <malloc.h>
#include "dirent.h"
#include "cl_kernels.hpp"

Device::~Device() {
  ReleaseKernels();
  free((void*) platformIDs);
  free (DeviceIDs);
  clReleaseProgram (Program);
  clReleaseCommandQueue (CommandQueue);
  clReleaseCommandQueue (CommandQueue_helper);
  clReleaseContext (Context);
  std::cout << "device destructor" << std::endl;
}

cl_int Device::Init(int deviceId) {

  DisplayPlatformInfo();

  clGetPlatformIDs(0, NULL, &numPlatforms);
  cl_platform_id *PlatformIDs = new cl_platform_id[numPlatforms];
  clGetPlatformIDs(numPlatforms, PlatformIDs, NULL);

  size_t nameLen;
  cl_int res = clGetPlatformInfo(PlatformIDs[0], CL_PLATFORM_NAME, 64,
      platformName, &nameLen);
  if (res != CL_SUCCESS) {
    fprintf(stderr, "Err: Failed to Get Platform Info\n");
    return 0;
  }
  platformName[nameLen] = 0;

  GetDeviceInfo();
  cl_uint uiNumDevices;
  cl_bool unified_memory = false;
  clGetDeviceIDs(PlatformIDs[0], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevices);
  uiNumDevices = numDevices;
  if (0 == uiNumDevices) {
	  std::cout << "Err: No GPU devices" << std::endl;
	  return 0;
  } else {
    pDevices = (cl_device_id *) malloc(uiNumDevices * sizeof(cl_device_id));
    OCL_CHECK(
        clGetDeviceIDs(PlatformIDs[0], CL_DEVICE_TYPE_GPU, uiNumDevices,
            pDevices, &uiNumDevices), "clGetDeviceIDs");
    if (deviceId == -1) {
      int i;
      for (i = 0; i < (int) uiNumDevices; i++) {
        clGetDeviceInfo(pDevices[i], CL_DEVICE_HOST_UNIFIED_MEMORY,
            sizeof(cl_bool), &unified_memory, NULL);
        if (!unified_memory) { //skip iGPU
          //we pick the first dGPU we found
          pDevices[0] = pDevices[i];
          device_id = i;
		  std::cout << "Picked default device type : dGPU " << device_id << std::endl;
          break;
        }
      }
      if (i == uiNumDevices) {
		  std::cout << "Cannot find any dGPU! " << std::endl;
		  //return 0;
      }
    } else if (deviceId >= 0 && deviceId < uiNumDevices) {
      pDevices[0] = pDevices[deviceId];
      device_id = deviceId;
	  std::cout << "Picked device type : GPU " << device_id << std::endl;
    } else {
		std::cout << "  Invalid GPU deviceId! " << std::endl;
    }
  }

  Context = clCreateContext(NULL, 1, pDevices, NULL, NULL, NULL);
  if (NULL == Context) {
    fprintf(stderr, "Err: Failed to Create Context\n");
    return 0;
  }
  CommandQueue = clCreateCommandQueue(Context, pDevices[0],
      CL_QUEUE_PROFILING_ENABLE, NULL);
  CommandQueue_helper = clCreateCommandQueue(Context, pDevices[0],
      CL_QUEUE_PROFILING_ENABLE, NULL);
  if (NULL == CommandQueue || NULL == CommandQueue_helper) {
    fprintf(stderr, "Err: Failed to Create Commandqueue\n");
    return 0;
  }
  BuildProgram (oclKernelPath);

  return 0;
}

void Device::BuildProgram(std::string kernel_dir) {
	const char *pSource;
#ifdef RUN_Android
	std::string strSource = "";
	RegisterKernels(strSource);
	pSource = strSource.c_str();
#else
  std::string strSource = "";
  DIR *ocl_dir;
  struct dirent *dirp;
  if ((ocl_dir = opendir(kernel_dir.c_str())) == NULL) {
    fprintf(stderr, "Err: Open ocl dir failed!\n");
  }
  while ((dirp = readdir(ocl_dir)) != NULL) {
    //Ignore hidden files
    if (dirp->d_name[0] == '.')
      continue;
    std::string file_name = std::string(dirp->d_name);
    //Skip non *.cl files
    size_t last_dot_pos = file_name.find_last_of(".");
    if (file_name.substr(last_dot_pos + 1) != "cl")
      continue;

    std::string ocl_kernel_full_path = kernel_dir + file_name;
    std::string tmpSource = "";
    ConvertToString(ocl_kernel_full_path.c_str(), tmpSource);
    strSource += tmpSource;
  } 
  pSource = strSource.c_str();	
#endif

  size_t uiArrSourceSize[] = { 0 };
  uiArrSourceSize[0] = strlen(pSource);
  Program = NULL;
  Program = clCreateProgramWithSource(Context, 1, &pSource, uiArrSourceSize,
      NULL);
  if (NULL == Program) {
    fprintf(stderr, "Err: Failed to create program\n");
  }
  cl_int iStatus = clBuildProgram(Program, 1, pDevices, buildOption.c_str(),
      NULL, NULL);
  std::cout << "Build Program";
  if (CL_SUCCESS != iStatus) {
    fprintf(stderr, "Err: Failed to build program\n");
	{
		cl_build_status status;
		clGetProgramBuildInfo(Program, *pDevices, CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &status, NULL);
		std::cout << "Build Status = " << status << " ( Err = " << iStatus << " )" << std::endl;

		char *build_log;
		size_t ret_val_size; // don't use vcl_size_t here
		iStatus = clGetProgramBuildInfo(Program, *pDevices, CL_PROGRAM_BUILD_LOG, 0, NULL, &ret_val_size);
		build_log = new char[ret_val_size + 1];
		iStatus = clGetProgramBuildInfo(Program, *pDevices, CL_PROGRAM_BUILD_LOG, ret_val_size, build_log, NULL);
		build_log[ret_val_size] = '\0';
		//std::cout << "Log: " << build_log << std::endl;
		std::ofstream logfile("build_log.txt");
		logfile << build_log;
		logfile.close();
		delete[] build_log;

		//std::cout << "Sources: " << source << std::endl;
		std::ofstream srcfile("build_source.txt");
		srcfile << pSource;
		srcfile.close();
	}
    clReleaseProgram (Program);
  }
}

bool Device::SetKernelPath(std::string path){
	oclKernelPath = path;
	return true;
}

bool Device::SetBuildOption(std::string option){
	buildOption = option;
	return true;
}

//Use to read OpenCL source code
cl_int Device::ConvertToString(std::string pFileName, std::string &Str) {
  size_t uiSize = 0;
  size_t uiFileSize = 0;
  char *pStr = NULL;
  char *tmp = (char*) pFileName.data();
  std::fstream fFile(tmp, (std::fstream::in | std::fstream::binary));
  if (fFile.is_open()) {
    fFile.seekg(0, std::fstream::end);
    uiSize = uiFileSize = (size_t) fFile.tellg();
    fFile.seekg(0, std::fstream::beg);
    pStr = new char[uiSize + 1];

    if (NULL == pStr) {
      fFile.close();
      return 0;
    }
    fFile.read(pStr, uiFileSize);
    fFile.close();
    pStr[uiSize] = '\0';
    Str = pStr;
    delete[] pStr;
    return 0;
  }else{
	  std::cout << "Err: Failed to open cl file!" << std::endl;
  }

  return -1;
}

cl_kernel Device::GetKernel(std::string kernel_name) {
  std::map<std::string, cl_kernel>::iterator it = Kernels.find(kernel_name);
  if (it == Kernels.end()) {
    cl_int _err = 0;
    cl_kernel kernel = clCreateKernel(Program, kernel_name.c_str(), &_err);
    OCL_CHECK(_err, "GetKernel");
    Kernels[kernel_name] = kernel;
  }
  return Kernels[kernel_name];
}

void Device::ReleaseKernels() {
  std::map<std::string, cl_kernel>::iterator it;
  for (it = Kernels.begin(); it != Kernels.end(); it++) {
    clReleaseKernel(it->second);
  }
}

void Device::DisplayPlatformInfo() {
  cl_int err;

  err = clGetPlatformIDs(0, NULL, &numPlatforms);
  if (err != CL_SUCCESS || numPlatforms <= 0) {
	  std::cout << "Failed to find any OpenCL platform." << std::endl;
    return;
  }

  platformIDs = (cl_platform_id *) malloc(
      sizeof(cl_platform_id) * numPlatforms);
  err = clGetPlatformIDs(numPlatforms, platformIDs, NULL);
  if (err != CL_SUCCESS) {
	  std::cout << "Failed to find any OpenCL platform." << std::endl;
    return;
  }

  std::cout << "Number of platforms found:" << numPlatforms << std::endl;

  //iterate through the list of platforms displaying platform information
  for (cl_uint i = 0; i < numPlatforms; i++) {
    DisplayInfo(platformIDs[i], CL_PLATFORM_NAME, "CL_PLATFORM_NAME");
    DisplayInfo(platformIDs[i], CL_PLATFORM_PROFILE, "CL_PLATFORM_PROFILE");
    DisplayInfo(platformIDs[i], CL_PLATFORM_VERSION, "CL_PLATFORM_VERSION");
    DisplayInfo(platformIDs[i], CL_PLATFORM_VENDOR, "CL_PLATFORM_VENDOR");
    DisplayInfo(platformIDs[i], CL_PLATFORM_EXTENSIONS,
        "CL_PLATFORM_EXTENSIONS");
  }

}

void Device::DisplayInfo(cl_platform_id id, cl_platform_info name,
    std::string str) {
  cl_int err;
  std::size_t paramValueSize;

  err = clGetPlatformInfo(id, name, 0, NULL, &paramValueSize);
  if (err != CL_SUCCESS) {
	  std::cout << "Failed to find OpenCL platform:" << str << std::endl;
    return;
  }

  char * info = (char *) alloca(sizeof(char) * paramValueSize);
  err = clGetPlatformInfo(id, name, paramValueSize, info, NULL);
  if (err != CL_SUCCESS) {
	  std::cout << "Failed to find OpenCL platform:" << str << std::endl;
    return;
  }

  std::cout << "\t" << str << "\t" << info << std::endl;
}

void Device::GetDeviceInfo() {
  cl_int err;
  //by default, we select the first platform. can be extended for more platforms
  //query GPU device for now
  err = clGetDeviceIDs(platformIDs[0], CL_DEVICE_TYPE_GPU, 0, NULL,
      &numDevices);
  // we allow program run if no GPU is found. Just return. No error reported.
  if (numDevices < 1) {
	  std::cout << "No GPU Devices found for platform " << platformIDs[0] << std::endl;
    return;
  }

  DeviceIDs = (cl_device_id *) malloc(sizeof(cl_device_id) * numDevices);
  err = clGetDeviceIDs(platformIDs[0], CL_DEVICE_TYPE_GPU, numDevices,
      DeviceIDs, NULL);
  if (err != CL_SUCCESS) {
	  std::cout << "Failed to find any GPU devices." << std::endl;
    return;
  }

  std::cout << "Number of devices found:" << numDevices << std::endl;
  for (cl_uint i = 0; i < numDevices; i++) {
	std::cout << "\t" << "DeviceID" << ":\t" << DeviceIDs[i] << std::endl;
    DisplayDeviceInfo < cl_device_type
        > (DeviceIDs[i], CL_DEVICE_TYPE, "Device Type");
    DisplayDeviceInfo < cl_bool
        > (DeviceIDs[i], CL_DEVICE_HOST_UNIFIED_MEMORY, "Is it integrated GPU?");
    DisplayDeviceInfo < cl_uint
        > (DeviceIDs[i], CL_DEVICE_MAX_CLOCK_FREQUENCY, "Max clock frequency MHz");
    DisplayDeviceInfo < cl_bool
        > (DeviceIDs[i], CL_DEVICE_HOST_UNIFIED_MEMORY, "Host-Device unified mem");
    DisplayDeviceInfo < cl_bool
        > (DeviceIDs[i], CL_DEVICE_ERROR_CORRECTION_SUPPORT, "ECC support");
    DisplayDeviceInfo < cl_bool
        > (DeviceIDs[i], CL_DEVICE_ENDIAN_LITTLE, "Endian little");
    DisplayDeviceInfo < cl_uint
        > (DeviceIDs[i], CL_DEVICE_MAX_COMPUTE_UNITS, "Max compute units");
    DisplayDeviceInfo < size_t
        > (DeviceIDs[i], CL_DEVICE_MAX_WORK_GROUP_SIZE, "Max work group size");
    DisplayDeviceInfo < cl_uint
        > (DeviceIDs[i], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, "Max work item dimensions");
    DisplayDeviceInfo<size_t *>(DeviceIDs[i], CL_DEVICE_MAX_WORK_ITEM_SIZES,
        "Max work item sizes");
    DisplayDeviceInfo < cl_command_queue_properties
        > (DeviceIDs[i], CL_DEVICE_QUEUE_PROPERTIES, "CL_DEVICE_QUEUE_PROPERTIES");
    DisplayDeviceInfo < cl_device_exec_capabilities
        > (DeviceIDs[i], CL_DEVICE_EXECUTION_CAPABILITIES, "CL_DEVICE_EXECUTION_CAPABILITIES");
    DisplayDeviceInfo < cl_ulong
        > (DeviceIDs[i], CL_DEVICE_MAX_MEM_ALLOC_SIZE, "Max mem alloc size");
    DisplayDeviceInfo < cl_ulong
        > (DeviceIDs[i], CL_DEVICE_GLOBAL_MEM_SIZE, "Global mem size");
    DisplayDeviceInfo < cl_ulong
        > (DeviceIDs[i], CL_DEVICE_LOCAL_MEM_SIZE, "Local mem size");
  }

}

void Device::DeviceQuery() {
  DisplayPlatformInfo();

  clGetPlatformIDs(0, NULL, &numPlatforms);
  cl_platform_id *PlatformIDs = new cl_platform_id[numPlatforms];
  clGetPlatformIDs(numPlatforms, PlatformIDs, NULL);

  size_t nameLen;
  cl_int res = clGetPlatformInfo(PlatformIDs[0], CL_PLATFORM_NAME, 64,
      platformName, &nameLen);
  if (res != CL_SUCCESS) {
    fprintf(stderr, "Err: Failed to Get Platform Info\n");
    return;
  }
  platformName[nameLen] = 0;

  GetDeviceInfo();
}

template <typename T>
void Device::DisplayDeviceInfo(cl_device_id id, cl_device_info name,
    std::string str) {
  cl_int err;
  std::size_t paramValueSize;

  err = clGetDeviceInfo(id, name, 0, NULL, &paramValueSize);
  if (err != CL_SUCCESS) {
	std::cout << "Failed to find OpenCL device info:" << str << std::endl;
    return;
  }

  std::string content;
  T * info = (T *) alloca(sizeof(T) * paramValueSize);
  err = clGetDeviceInfo(id, name, paramValueSize, info, NULL);
  if (err != CL_SUCCESS) {
	std::cout << "Failed to find OpenCL device info:" << str << std::endl;
    return;
  }

  switch (name) {
  case CL_DEVICE_TYPE: {
    std::string deviceType;
    appendBitfield < cl_device_type
        > (*(reinterpret_cast<cl_device_type*>(info)), CL_DEVICE_TYPE_CPU, "CL_DEVICE_TYPE_CPU", deviceType);

    appendBitfield < cl_device_type
        > (*(reinterpret_cast<cl_device_type*>(info)), CL_DEVICE_TYPE_GPU, "CL_DEVICE_TYPE_GPU", deviceType);

    appendBitfield < cl_device_type
        > (*(reinterpret_cast<cl_device_type*>(info)), CL_DEVICE_TYPE_ACCELERATOR, "CL_DEVICE_TYPE_ACCELERATOR", deviceType);

    appendBitfield < cl_device_type
        > (*(reinterpret_cast<cl_device_type*>(info)), CL_DEVICE_TYPE_DEFAULT, "CL_DEVICE_TYPE_DEFAULT", deviceType);

	std::cout << "\t " << str << ":\t" << deviceType;
  }
    break;
  case CL_DEVICE_EXECUTION_CAPABILITIES: {
    std::string memType;
    appendBitfield < cl_device_exec_capabilities
        > (*(reinterpret_cast<cl_device_exec_capabilities*>(info)), CL_EXEC_KERNEL, "CL_EXEC_KERNEL", memType);

    appendBitfield < cl_device_exec_capabilities
        > (*(reinterpret_cast<cl_device_exec_capabilities*>(info)), CL_EXEC_NATIVE_KERNEL, "CL_EXEC_NATIVE_KERNEL", memType);

	std::cout << "\t " << str << ":\t" << memType << std::endl;

  }
    break;
  case CL_DEVICE_QUEUE_PROPERTIES: {
    std::string memType;
    appendBitfield < cl_device_exec_capabilities
        > (*(reinterpret_cast<cl_device_exec_capabilities*>(info)), CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, "CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE", memType);

    appendBitfield < cl_device_exec_capabilities
        > (*(reinterpret_cast<cl_device_exec_capabilities*>(info)), CL_QUEUE_PROFILING_ENABLE, "CL_QUEUE_PROFILING_ENABLE", memType);

	std::cout << "\t " << str << ":\t" << memType << std::endl;
  }
    break;
  default:
	std::cout << "\t" << str << ":\t" << *info << std::endl;
    break;
  }

}

template <typename T>
void Device::appendBitfield(T info, T value, std::string name,
    std::string &str) {
  if (info & value) {
    if (str.length() > 0) {
      str.append(" | ");
    }
    str.append(name);
  }
}


