#include "Pch.h"
#include "CLExtensions.h"

clGetDeviceIDsFromD3D11NV_fn clGetDeviceIDsFromD3D11NV = NULL;
clCreateFromD3D11BufferNV_fn clCreateFromD3D11BufferNV = NULL;
clCreateFromD3D11Texture2DNV_fn clCreateFromD3D11Texture2DNV = NULL;
clEnqueueAcquireD3D11ObjectsNV_fn clEnqueueAcquireD3D11ObjectsNV = NULL;
clEnqueueReleaseD3D11ObjectsNV_fn clEnqueueReleaseD3D11ObjectsNV = NULL;

clGetDeviceIDsFromD3D11KHR_fn clGetDeviceIDsFromD3D11KHR = NULL;
clCreateFromD3D11BufferKHR_fn clCreateFromD3D11BufferKHR = NULL;
clCreateFromD3D11Texture2DKHR_fn clCreateFromD3D11Texture2DKHR = NULL;
clEnqueueAcquireD3D11ObjectsKHR_fn clEnqueueAcquireD3D11ObjectsKHR = NULL;
clEnqueueReleaseD3D11ObjectsKHR_fn clEnqueueReleaseD3D11ObjectsKHR = NULL;

void CLExtensions::Init(cl::Platform platform, bool nv)
{
	if (nv)
	{
		INITPFN(platform(), clGetDeviceIDsFromD3D11NV);
		INITPFN(platform(), clCreateFromD3D11BufferNV);
		INITPFN(platform(), clCreateFromD3D11Texture2DNV);
		INITPFN(platform(), clEnqueueAcquireD3D11ObjectsNV);
		INITPFN(platform(), clEnqueueReleaseD3D11ObjectsNV);
	}
	else
	{
		INITPFN(platform(), clGetDeviceIDsFromD3D11KHR);
		INITPFN(platform(), clCreateFromD3D11BufferKHR);
		INITPFN(platform(), clCreateFromD3D11Texture2DKHR);
		INITPFN(platform(), clEnqueueAcquireD3D11ObjectsKHR);
		INITPFN(platform(), clEnqueueReleaseD3D11ObjectsKHR);
	}
}