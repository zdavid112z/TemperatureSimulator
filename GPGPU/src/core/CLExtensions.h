#pragma once

#include "CL/cl.hpp"
#include "CL/cl_d3d11.h"
#include "CL/cl_d3d11_ext.h"

#define INITPFN(p, x) \
    x = (x ## _fn)clGetExtensionFunctionAddressForPlatform(p, #x);\
    if(!x) { printf("failed getting %s", #x); }

extern clGetDeviceIDsFromD3D11NV_fn clGetDeviceIDsFromD3D11NV;
extern clCreateFromD3D11BufferNV_fn clCreateFromD3D11BufferNV;
extern clCreateFromD3D11Texture2DNV_fn clCreateFromD3D11Texture2DNV;
extern clEnqueueAcquireD3D11ObjectsNV_fn clEnqueueAcquireD3D11ObjectsNV;
extern clEnqueueReleaseD3D11ObjectsNV_fn clEnqueueReleaseD3D11ObjectsNV;

extern clGetDeviceIDsFromD3D11KHR_fn clGetDeviceIDsFromD3D11KHR;
extern clCreateFromD3D11BufferKHR_fn clCreateFromD3D11BufferKHR;
extern clCreateFromD3D11Texture2DKHR_fn clCreateFromD3D11Texture2DKHR;
extern clEnqueueAcquireD3D11ObjectsKHR_fn clEnqueueAcquireD3D11ObjectsKHR;
extern clEnqueueReleaseD3D11ObjectsKHR_fn clEnqueueReleaseD3D11ObjectsKHR;

class CLExtensions
{
public:
	static void Init(cl::Platform, bool nv);
};