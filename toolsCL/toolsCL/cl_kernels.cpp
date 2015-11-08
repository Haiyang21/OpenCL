// AUTOMATICALLY GENERATED FILE, DO NOT EDIT
#include "cl_kernels.hpp"
#include <sstream>
#include <string>
std::string header = "#ifndef __OPENCL_VERSION__\n#define __kernel\n#define __global\n#define __constant\n#define __local\n#define get_global_id(x) 0\n#define get_global_size(x) 0\n#define get_local_id(x) 0\n#define get_local_size(x) 0\n#define FLT_MAX 0\n#define FLT_MIN 0\n#define cl_khr_fp64\n#define cl_amd_fp64\n#define DOUBLE_SUPPORT_AVAILABLE\n#define CLK_LOCAL_MEM_FENCE\n#define Dtype float\n#define barrier(x)\n#define atomic_cmpxchg(x, y, z) x\n#endif\n\n#define CONCAT(A,B) A##_##B\n#define TEMPLATE(name,type) CONCAT(name,type)\n\n#define TYPE_FLOAT 1\n#define TYPE_DOUBLE 2\n\n#if defined(cl_khr_fp64)\n#pragma OPENCL EXTENSION cl_khr_fp64 : enable\n#define DOUBLE_SUPPORT_AVAILABLE\n#elif defined(cl_amd_fp64)\n#pragma OPENCL EXTENSION cl_amd_fp64 : enable\n#define DOUBLE_SUPPORT_AVAILABLE\n#endif\n\n#if defined(cl_khr_int64_base_atomics)\n#pragma OPENCL EXTENSION cl_khr_int64_base_atomics : enable\n#define ATOMICS_64_AVAILABLE\n#endif";  // NOLINT
std::string mul2 = "\n__kernel void mul2(__global float* input, \n					__global float* output)\n{\n	unsigned int id = get_global_id(0);\n	output[id] = input[id] * 2;\n}";  // NOLINT
void RegisterKernels(std::string &strSource) {
  std::stringstream ss;
  ss << header << "\n\n";  // NOLINT
  ss << mul2 << "\n\n";  // NOLINT
  strSource = ss.str();
}
