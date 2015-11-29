
// ImageFilter2D.cpp
//
//    This example demonstrates performing gaussian filtering on a 2D image using
//    OpenCL
//
//    Requires FreeImage library for image I/O:
//      http://freeimage.sourceforge.net/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "FreeImage.h"
#include "device.hpp"


///
//  Load an image using the FreeImage library and create an OpenCL
//  image out of it
//
cl_mem LoadImage(cl_context context, char *fileName, int &width, int &height)
{
    FREE_IMAGE_FORMAT format = FreeImage_GetFileType(fileName, 0);
    FIBITMAP* image = FreeImage_Load(format, fileName);

    // Convert to 32-bit image
    FIBITMAP* temp = image;
    image = FreeImage_ConvertTo32Bits(image);
    FreeImage_Unload(temp);

    width = FreeImage_GetWidth(image);
    height = FreeImage_GetHeight(image);

    char *buffer = new char[width * height * 4];
    memcpy(buffer, FreeImage_GetBits(image), width * height * 4);

    FreeImage_Unload(image);

    // Create OpenCL image
    cl_image_format clImageFormat;
    clImageFormat.image_channel_order = CL_RGBA;
    clImageFormat.image_channel_data_type = CL_UNORM_INT8;

    cl_int err;
    cl_mem clImage;
    clImage = clCreateImage2D(context,
                            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            &clImageFormat,
                            width,
                            height,
                            0,
                            buffer,
                            &err);

    if (err != CL_SUCCESS)
    {
        std::cerr << "Error creating CL image object" << std::endl;
        return 0;
    }

    return clImage;
}

///
//  Save an image using the FreeImage library
//
bool SaveImage(char *fileName, char *buffer, int width, int height)
{
    FREE_IMAGE_FORMAT format = FreeImage_GetFIFFromFilename(fileName);
    FIBITMAP *image = FreeImage_ConvertFromRawBits((BYTE*)buffer, width,
                        height, width * 4, 32,
                        0xFF000000, 0x00FF0000, 0x0000FF00);
    return (FreeImage_Save(format, image, fileName) == TRUE) ? true : false;
}

///
//  Round up to the nearest multiple of the group size
//
size_t roundUp(int groupSize, int globalSize)
{
    int r = globalSize % groupSize;
    if(r == 0)
    {
     	return globalSize;
    }
    else
    {
     	return globalSize + groupSize - r;
    }
}

///
//	main() for HelloBinaryWorld example
//
int ImageFilter2D()
{
    cl_kernel kernel = 0;
    cl_mem imageObjects[2] = { 0, 0 };
    cl_sampler sampler = 0;
    cl_int err;
	char *file_in = "image_Lena512rgb.bmp";
	char *file_out = "image_Lena512rgb_out.bmp";

	//! Setup device
	Device clDevice;
	clDevice.Init();

    //! Make sure the device supports images, otherwise exit
    cl_bool imageSupport = CL_FALSE;
	clGetDeviceInfo(clDevice.DeviceIDs[0], CL_DEVICE_IMAGE_SUPPORT, sizeof(cl_bool),
                    &imageSupport, NULL);
    if (imageSupport != CL_TRUE)
    {
        std::cerr << "OpenCL device does not support images." << std::endl;
        
        return 1;
    }

    //! Init data
    int width, height;	
	imageObjects[0] = LoadImage(clDevice.Context, file_in, width, height);
    if (imageObjects[0] == 0)
    {
        std::cerr << "Error loading: " << std::string(file_in) << std::endl;
        return 1;
    }

    // Create ouput image object
    cl_image_format clImageFormat;
    clImageFormat.image_channel_order = CL_RGBA;
    clImageFormat.image_channel_data_type = CL_UNORM_INT8;
    imageObjects[1] = clCreateImage2D(clDevice.Context,
                                       CL_MEM_WRITE_ONLY,
                                       &clImageFormat,
                                       width,
                                       height,
                                       0,
                                       NULL,
                                       &err);

    if (err != CL_SUCCESS)
    {
        std::cerr << "Error creating CL output image object." << std::endl;
        return 1;
    }


    //! Create sampler for sampling image object
    sampler = clCreateSampler(clDevice.Context,
                              CL_FALSE, // Non-normalized coordinates
                              CL_ADDRESS_CLAMP_TO_EDGE, // Set the color outside edge to color on edge
                              CL_FILTER_NEAREST,
                              &err);

    if (err != CL_SUCCESS)
    {
        std::cerr << "Error creating CL sampler object." << std::endl;
        return 1;
    }


	//! Get kernel
	std::string kernel_name = "gaussian_filter";
	kernel = clDevice.GetKernel(kernel_name);
    if (kernel == NULL)
    {
        std::cerr << "Failed to create kernel" << std::endl;
        return 1;
    }

    //! Set the kernel arguments
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &imageObjects[0]);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &imageObjects[1]);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_sampler), &sampler);
    err |= clSetKernelArg(kernel, 3, sizeof(cl_int), &width);
    err |= clSetKernelArg(kernel, 4, sizeof(cl_int), &height);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Error setting kernel arguments." << std::endl;
        return 1;
    }

	//! Set global and local work size
    size_t localWorkSize[2] = { 16, 16 };
    size_t globalWorkSize[2] = { roundUp(localWorkSize[0], width),
                                 roundUp(localWorkSize[1], height) };

    //! Excute kernel
	err = clEnqueueNDRangeKernel(clDevice.CommandQueue, kernel, 2, NULL,
                                    globalWorkSize, localWorkSize,
                                    0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Error queuing kernel for execution." << std::endl;
        return 1;
    }

    //! Get outputs to host
    char *buffer = new char [width * height * 4];
    size_t origin[3] = { 0, 0, 0 };
    size_t region[3] = { width, height, 1};
	err = clEnqueueReadImage(clDevice.CommandQueue, imageObjects[1], CL_TRUE,
                                origin, region, 0, 0, buffer,
                                0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        std::cerr << "Error reading result buffer." << std::endl;
        return 1;
    }

    std::cout << std::endl;
    std::cout << "Executed program succesfully." << std::endl;

    //! Save the image out to disk
	if (!SaveImage(file_out, buffer, width, height))
    {
        std::cerr << "Error writing output image: " << file_out << std::endl;
        delete [] buffer;
        return 1;
    }

    delete [] buffer;

    return 0;
}
