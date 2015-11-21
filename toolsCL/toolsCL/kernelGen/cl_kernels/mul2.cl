
__kernel void mul2(__global float* input, __global float* output, float factor)
{
	unsigned int id = get_global_id(0);
	output[id] = input[id] * factor;
}