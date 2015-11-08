# OpenCL
toolCL provide a serials of tools to make the project easy with OpenCL.

Instructions:
  Device clDevice;
  clDevice.Init();  
  //clDevice.SetKernelPath("");//default is "./kernelGen/cl_kernels/"
	//clDevice.SetBuildOption("");//default is ""
	
	//! Init data
	//create input data on CPU
	int num = 1024;
	float *h_idata = (float*)malloc(sizeof(float)* num);
	for (int i = 0; i < num; i++){
		h_idata[i] = i;
	}
	//allocate memory for the results on CPU
	float *h_odata = (float*)malloc(sizeof(float)* num);
	//allocate memory and data on GPU
	cl_mem d_idata = clCreateBuffer(clDevice.Context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float)*num, h_idata, NULL);
	cl_mem d_odata = clCreateBuffer(clDevice.Context, CL_MEM_READ_WRITE, sizeof(float)*num, NULL, NULL);

	//! Get kernel
	std::string kernel_name = "mul2";
	cl_kernel Kernel = clDevice.GetKernel(kernel_name);

	//! Set argments
	cl_int ret;
	ret  = clSetKernelArg(Kernel, 0, sizeof(cl_mem), (void*)&d_idata);
	ret |= clSetKernelArg(Kernel, 1, sizeof(cl_mem), (void*)&d_odata);
	OCL_CHECK(ret, "mul2: clSetKernelArg");

	//! Set global and local work size
	size_t global_work_size[] = { (size_t)num };
	size_t local_work_size[] = { 256 };

	//! Excute kernel
	OCL_CHECK(
		clEnqueueNDRangeKernel(clDevice.CommandQueue, Kernel, 1, NULL,global_work_size, local_work_size, 0, NULL, NULL), 
		"mul2: kernel");

	//! Get outputs
	// copy result from device to host
	clEnqueueReadBuffer(clDevice.CommandQueue, d_odata, CL_TRUE, 0, num * sizeof(float), h_odata, 0, NULL, NULL);
	for(int i=0; i<10; i++)
		std::cout << h_idata[i] << "  " << h_odata[i] << std::endl;
