#include "Pch.h"
#include <iomanip>
#include "Simulator.h"
#include "nlohmann/json.hpp"
#include "utils/Utils.h"
#include "CLExtensions.h"
#include "graphics/Graphics.h"
#include "utils/Timer.h"
#include "Image.h"
#include "graphics/Texture2D.h"

Simulator::Simulator(Graphics* g, Texture2D* texture, const SimulatorData& data) :
	m_Data(data), m_Graphics(g), m_DXTexture(texture)
{
	m_SessionStarted = false;
}

Simulator::~Simulator()
{
	if (m_CPUThread.joinable())
		m_CPUThread.join();
	CloseThreadpoolCleanupGroupMembers(m_ThreadpoolCleanupGroup, FALSE, NULL);
	CloseThreadpoolCleanupGroup(m_ThreadpoolCleanupGroup);
	DestroyThreadpoolEnvironment(&m_ThreadpoolEnv);
	CloseThreadpool(m_Threadpool);
	ClearParamsCache();
}

void Simulator::Reload() 
{
	if (m_Loaded)
	{

	}
	m_Loaded = true;
	//nlohmann::json json = nlohmann::json::parse(Utils::LoadFile(m_ConfigPath));

	cl_device_type deviceType = CL_DEVICE_TYPE_ALL;

	m_HeaterLocation = m_Data.heaterLocation;
	m_HeaterTemperature = m_Data.heaterTemperature;
	m_ImageWidth = m_Data.imageWidth;
	m_ImageHeight = m_Data.imageHeight;
	m_StartTemperature = m_Data.startTemperature;
	m_LocalWorkSize = m_Data.localWorkSize;
	m_SingleWorkGroupSize = m_Data.singleLocalWorkSize;
	m_NumParamsPerStep = m_Data.numParamsPerStep;
	m_SessionStarted = false;

	cl_int err = CL_SUCCESS;
	std::vector<cl::Platform> platforms;
	err = cl::Platform::get(&platforms);
	Utils::AssertCLError(err);

	bool found = false;
	for (uint i = 0; i < platforms.size(); i++)
	{
		//if (i == 0)
		//	continue;
		auto& platform = platforms[i];
		std::string extensions = platform.getInfo<CL_PLATFORM_EXTENSIONS>();
		bool foundNV = extensions.find("cl_nv_d3d11_sharing") != std::string::npos;
		bool foundKHR = extensions.find("cl_khr_d3d11_sharing") != std::string::npos;
		if (foundNV || foundKHR)
		{
			m_Platform = platform;
			CLExtensions::Init(m_Platform, foundNV);

			std::vector< cl_context_properties> properties;
			if(foundNV)
			{
				properties = {
					CL_CONTEXT_PLATFORM, (cl_context_properties)(m_Platform()),
					CL_CONTEXT_D3D11_DEVICE_NV, (cl_context_properties)m_Graphics->GetDevice(),
					//CL_CONTEXT_INTEROP_USER_SYNC, CL_FALSE, 
					0 };
			}
			else
			{
				properties = {
					CL_CONTEXT_PLATFORM, (cl_context_properties)(m_Platform()),
					CL_CONTEXT_D3D11_DEVICE_KHR, (cl_context_properties)m_Graphics->GetDevice(),
					CL_CONTEXT_INTEROP_USER_SYNC, CL_FALSE, 
					0 };
			}

			cl_uint numDevs = 0;
			if (foundNV)
			{
				err = clGetDeviceIDsFromD3D11NV(m_Platform(), CL_D3D11_DEVICE_NV, 
					(void*)m_Graphics->GetDevice(), CL_PREFERRED_DEVICES_FOR_D3D11_NV, 0, NULL, &numDevs);
			}
			else
			{
				err = clGetDeviceIDsFromD3D11KHR(m_Platform(), CL_D3D11_DEVICE_KHR,
					(void*)m_Graphics->GetDevice(), CL_PREFERRED_DEVICES_FOR_D3D11_KHR, 0, NULL, &numDevs);
			}
			//err = clGetDeviceIDs(m_Platform(), CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevs);
			if (err == CL_DEVICE_NOT_FOUND)
				continue;
			Utils::AssertCLError(err);

			cl_device_id * devices = new cl_device_id[numDevs];
			if (foundNV)
			{
				err = clGetDeviceIDsFromD3D11NV(m_Platform(), CL_D3D11_DEVICE_NV, 
					(void*)m_Graphics->GetDevice(), CL_PREFERRED_DEVICES_FOR_D3D11_NV, numDevs, devices, NULL);
			}
			else
			{
				err = clGetDeviceIDsFromD3D11KHR(m_Platform(), CL_D3D11_DEVICE_KHR,
					(void*)m_Graphics->GetDevice(), CL_PREFERRED_DEVICES_FOR_D3D11_KHR, numDevs, devices, NULL);
			}
			//err = clGetDeviceIDs(m_Platform(), CL_DEVICE_TYPE_ALL, numDevs, devices, nullptr);
			Utils::AssertCLError(err);

			m_Device = devices[0];

			//m_Context = cl::Context(m_Device, properties, NULL, NULL, &err);
			m_Context = clCreateContext(properties.data(), 1, devices, NULL, NULL, &err);
			Utils::AssertCLError(err);
			delete[] devices;

			m_NVExtension = foundNV;
			found = true;
			break;
		}
	}
	if (!found)
	{
		std::cerr << "Could not find a platform that supports cl_nv_d3d11_sharing!";
		assert(false);
	}
	PrintInfo();

	//std::vector<cl::Device> devices = m_Context.getInfo<CL_CONTEXT_DEVICES>(&err);
	//Utils::AssertCLError(err);
	//m_Device = devices[0];

	/*
	for (auto device : devices)
	{
		std::string name, vendor, profile, version, driverVersion, clVersion, extensions;
		device.getInfo(CL_DEVICE_NAME, &name);
		device.getInfo(CL_DEVICE_VENDOR, &vendor);
		device.getInfo(CL_DEVICE_PROFILE, &profile);
		device.getInfo(CL_DEVICE_VERSION, &version);
		device.getInfo(CL_DRIVER_VERSION, &driverVersion);
		device.getInfo(CL_DEVICE_OPENCL_C_VERSION, &clVersion);
		device.getInfo(CL_DEVICE_EXTENSIONS, &extensions);

		printf(
			"Name: %s\n" "Vendor: %s\n" "Profile: %s\n" "Version: %s\n"
			"Driver Version: %s\n" "CL Version: %s\n" "Extensions: %s\n\n",
			name.c_str(), vendor.c_str(), profile.c_str(), version.c_str(),
			driverVersion.c_str(), clVersion.c_str(), extensions.c_str());
	}*/

	std::string sourceFromFile = Utils::LoadFile(m_Data.kernelSourcePath);
	cl::Program::Sources source(1,
		std::make_pair(sourceFromFile.c_str(), sourceFromFile.length()));
	m_Program = cl::Program(m_Context, source, &err);
	Utils::AssertCLError(err);
	err = m_Program.build();
	if (err != 0)
	{
		cl_build_status status = m_Program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(m_Device);
		if (status == CL_BUILD_ERROR)
		{
			// Get the build log
			std::string name = m_Device.getInfo<CL_DEVICE_NAME>();
			std::string buildlog = m_Program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_Device);
			std::cerr << "Build log for " << name << ":" << std::endl
				<< buildlog << std::endl;
		}
	}
	Utils::AssertCLError(err);

	m_CommandQueue = cl::CommandQueue(m_Context, m_Device, 0, &err);
	Utils::AssertCLError(err);

	m_TemperatureKernel = cl::Kernel(m_Program, m_Data.temperatureKernelName.c_str(), &err);
	Utils::AssertCLError(err);

	m_SumKernel = cl::Kernel(m_Program, m_Data.sumKernelName.c_str(), &err);
	Utils::AssertCLError(err);

	m_DiffKernel = cl::Kernel(m_Program, m_Data.diffKernelName.c_str(), &err);
	Utils::AssertCLError(err);

	m_HeatmapKernel = cl::Kernel(m_Program, m_Data.heatmapKernelName.c_str(), &err);
	Utils::AssertCLError(err);

	m_ImageBuffers[0] = cl::Buffer(m_Context,
		CL_MEM_READ_WRITE | CL_MEM_HOST_WRITE_ONLY,
		m_Data.imageWidth * m_Data.imageHeight * sizeof(float), nullptr, &err);
	Utils::AssertCLError(err);

	m_ImageBuffers[1] = cl::Buffer(m_Context,
		CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY,
		m_Data.imageWidth * m_Data.imageHeight * sizeof(float), nullptr, &err);
	Utils::AssertCLError(err);

	m_ImageBuffers[2] = cl::Buffer(m_Context,
		CL_MEM_READ_WRITE | CL_MEM_HOST_WRITE_ONLY,
		m_Data.imageWidth * m_Data.imageHeight * sizeof(float), nullptr, &err);
	Utils::AssertCLError(err);

	m_MaxChange = cl::Buffer(m_Context,
		CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
		m_Data.imageWidth * m_Data.imageHeight * sizeof(float), nullptr, &err);
	Utils::AssertCLError(err);

	m_Parameters = cl::Buffer(m_Context,
		CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY,
		10 * sizeof(float), nullptr, &err);
	Utils::AssertCLError(err);

	if (m_NVExtension)
	{
		m_Image = clCreateFromD3D11Texture2DNV(m_Context(), CL_MEM_WRITE_ONLY, m_DXTexture->GetTexture(), 0, &err);
	}
	else
	{
		m_Image = clCreateFromD3D11Texture2DKHR(m_Context(), CL_MEM_WRITE_ONLY, m_DXTexture->GetTexture(), 0, &err);
	}
	Utils::AssertCLError(err);

	m_FloatImages[0].Init(m_Data.imageWidth, m_Data.imageHeight);
	m_FloatImages[1].Init(m_Data.imageWidth, m_Data.imageHeight);
	for (int i = 0; i < m_Data.numWorkerThreads; i++)
	{
		m_WorkerThreads.emplace_back();
	}
	m_CPUStartRow = m_Data.cpuStartRow;

	InitializeThreadpoolEnvironment(&m_ThreadpoolEnv);
	m_Threadpool = CreateThreadpool(NULL);
	SetThreadpoolThreadMinimum(m_Threadpool, m_Data.numWorkerThreads);
	SetThreadpoolThreadMaximum(m_Threadpool, m_Data.numWorkerThreads);
	m_ThreadpoolCleanupGroup = CreateThreadpoolCleanupGroup();
	SetThreadpoolCallbackPool(&m_ThreadpoolEnv, m_Threadpool);
	SetThreadpoolCallbackCleanupGroup(&m_ThreadpoolEnv, m_ThreadpoolCleanupGroup, NULL);
	m_WorkerThreadsData.resize(m_Data.numWorkerThreads);
}

void Simulator::ApplyHeater(int destBufferIndex, cl::Event* doneEvent)
{
	cl_int err;

	PrepareParamsOperation();
	PushFloatParamToCache(m_HeaterTemperature);
	err = m_CommandQueue.enqueueWriteBuffer(m_ImageBuffers[destBufferIndex], CL_FALSE,
		(m_HeaterLocation.first + m_HeaterLocation.second * m_ImageWidth) * sizeof(float),
		sizeof(float), GetCacheData());
	Utils::AssertCLError(err);

	err = m_CommandQueue.enqueueBarrierWithWaitList(nullptr, doneEvent);
	Utils::AssertCLError(err);
}

void Simulator::Blur(bool horizontal, int srcBufferIndex, int destBufferIndex, int maxRow, cl::Event* doneEvent)
{
	maxRow = std::min<int>(maxRow, m_ImageHeight);
	PrepareParamsOperation();
	PushIntParamToCache(m_ImageWidth);
	PushIntParamToCache(m_ImageHeight);
	PushIntParamToCache(horizontal);
	PushIntParamToCache(!horizontal);
	PushIntParamToCache(m_StartTemperature);
	PushIntParamToCache(maxRow);
	cl_int err = m_CommandQueue.enqueueWriteBuffer(m_Parameters, CL_FALSE, 0, GetCacheSize(), GetCacheData());
	Utils::AssertCLError(err);

	err = m_CommandQueue.enqueueBarrierWithWaitList();
	Utils::AssertCLError(err);

	m_TemperatureKernel.setArg(0, m_Parameters);
	m_TemperatureKernel.setArg(1, m_ImageBuffers[srcBufferIndex]);
	m_TemperatureKernel.setArg(2, m_ImageBuffers[destBufferIndex]);

	std::pair<uint, uint> globalWorkSize = std::make_pair(
		((m_ImageWidth + m_LocalWorkSize.first  - 1) / m_LocalWorkSize.first ) * m_LocalWorkSize.first,
		((maxRow       + m_LocalWorkSize.second - 1) / m_LocalWorkSize.second) * m_LocalWorkSize.second
	);
	err = m_CommandQueue.enqueueNDRangeKernel(m_TemperatureKernel, cl::NullRange,
		cl::NDRange(globalWorkSize.first, globalWorkSize.second),
		cl::NDRange(m_LocalWorkSize.first, m_LocalWorkSize.second));
	Utils::AssertCLError(err);

	err = m_CommandQueue.enqueueBarrierWithWaitList(nullptr, doneEvent);
	Utils::AssertCLError(err);
}

void Simulator::ComputeDiff(int initialBufferIndex, int finalBufferIndex, int destBufferIndex, int maxRow)
{
	int size = m_ImageWidth * std::min<int>(maxRow, m_ImageHeight);
	PrepareParamsOperation();
	PushIntParamToCache(size);
	cl_int err = m_CommandQueue.enqueueWriteBuffer(m_Parameters, CL_FALSE, 0, GetCacheSize(), GetCacheData());
	Utils::AssertCLError(err);

	err = m_CommandQueue.enqueueBarrierWithWaitList();
	Utils::AssertCLError(err);

	err = m_DiffKernel.setArg(0, m_Parameters);
	Utils::AssertCLError(err);
	err = m_DiffKernel.setArg(1, m_ImageBuffers[initialBufferIndex]);
	Utils::AssertCLError(err);
	err = m_DiffKernel.setArg(2, m_ImageBuffers[finalBufferIndex]);
	Utils::AssertCLError(err);
	err = m_DiffKernel.setArg(3, m_ImageBuffers[destBufferIndex]);
	Utils::AssertCLError(err);

	size = ((size + m_SingleWorkGroupSize - 1) / m_SingleWorkGroupSize) * m_SingleWorkGroupSize;
	err = m_CommandQueue.enqueueNDRangeKernel(m_DiffKernel, cl::NullRange,
		cl::NDRange(size), cl::NDRange(m_SingleWorkGroupSize));
	Utils::AssertCLError(err);

	err = m_CommandQueue.enqueueBarrierWithWaitList();
	Utils::AssertCLError(err);
}

void Simulator::ComputeSum(int srcBufferIndex, int auxBufferIndex, float* result, bool block, int maxRow, cl::Event* doneEvent)
{
	cl_int err;
	int totalSize = m_ImageWidth * std::min<int>(maxRow, m_ImageHeight);
	int localSize = m_SingleWorkGroupSize;

	int srcBuffer = srcBufferIndex, destBuffer = auxBufferIndex;

	while (totalSize > 1)
	{
		PrepareParamsOperation();
		PushIntParamToCache(totalSize);
		err = m_CommandQueue.enqueueWriteBuffer(m_Parameters, CL_FALSE, 0, GetCacheSize(), GetCacheData());
		Utils::AssertCLError(err);

		err = m_CommandQueue.enqueueBarrierWithWaitList();
		Utils::AssertCLError(err);

		err = m_SumKernel.setArg(0, m_Parameters);
		Utils::AssertCLError(err);
		err = m_SumKernel.setArg(1, m_ImageBuffers[srcBuffer]);
		Utils::AssertCLError(err);
		err = m_SumKernel.setArg(2, m_ImageBuffers[destBuffer]);
		Utils::AssertCLError(err);
		err = m_SumKernel.setArg(3, sizeof(float) * localSize, nullptr);
		Utils::AssertCLError(err);

		int globalSize = ((totalSize + localSize - 1) / localSize) * localSize;
		err = m_CommandQueue.enqueueNDRangeKernel(m_SumKernel, cl::NullRange,
			cl::NDRange(globalSize), cl::NDRange(localSize));
		Utils::AssertCLError(err);

		std::swap(srcBuffer, destBuffer);
		totalSize = (totalSize + localSize - 1) / localSize;

		if (totalSize <= 1 && srcBuffer == 1)
		{
			err = m_CommandQueue.enqueueBarrierWithWaitList(nullptr, doneEvent);
			Utils::AssertCLError(err);
		}
		else
		{
			err = m_CommandQueue.enqueueBarrierWithWaitList();
			Utils::AssertCLError(err);
		}

	}
	if (srcBuffer != 1)
	{
		err = m_CommandQueue.enqueueCopyBuffer(m_ImageBuffers[srcBuffer], m_ImageBuffers[destBuffer], 0, 0, sizeof(float));
		Utils::AssertCLError(err);

		err = m_CommandQueue.enqueueBarrierWithWaitList(nullptr, doneEvent);
		Utils::AssertCLError(err);

		std::swap(srcBuffer, destBuffer);
	}
}
#if 0
void Simulator::Start()
{
	if (!m_Loaded)
		Reload();

	cl_int err = m_CommandQueue.enqueueFillBuffer<float>
		(m_ImageBuffers[0], m_StartTemperature, 0, m_ImageWidth * m_ImageHeight * sizeof(float));
	Utils::AssertCLError(err);

	uint currentTime = 0;
	float sumOfChanges = 0;
	int i = 0;
	Timer timer;
	do
	{
		HandleEvents(0, currentTime);
		Blur(true, 0, 1);
		Blur(false, 1, 2);
		ComputeDiff(0, 2, 1);
		ComputeSum(1, 0, &sumOfChanges, true);
		err = m_CommandQueue.enqueueCopyBuffer(m_ImageBuffers[2], m_ImageBuffers[0], 0, 0, m_ImageWidth * m_ImageHeight * sizeof(float));
		Utils::AssertCLError(err);

		currentTime++;
		timer.Loop();
		std::cout << std::setprecision(6) << std::fixed << sumOfChanges << " ( " << std::setprecision(6) << std::fixed << timer.GetDelta() * 1000.f << " ms); t = " << std::setw(6) << currentTime << std::endl;
		if (currentTime >= 100000)
			break;
	} while (sumOfChanges >= m_ImageHeight * m_ImageWidth);

	err = m_CommandQueue.enqueueCopyBuffer(m_ImageBuffers[0], m_ImageBuffers[1], 0, 0, m_ImageWidth * m_ImageHeight * sizeof(float));
	Utils::AssertCLError(err);

	float* finalImage = new float[m_ImageWidth * m_ImageHeight];
	err = m_CommandQueue.enqueueReadBuffer(m_ImageBuffers[1], CL_TRUE, 0, m_ImageWidth * m_ImageHeight * sizeof(float), finalImage);
	Utils::AssertCLError(err);

	Image* image = new Image(m_ImageWidth, m_ImageHeight, finalImage);
	image->WriteToFile("out.bmp");

	delete[] finalImage;
	delete image;
}
#endif
void Simulator::StartNewSessionGPU()
{
	if (!m_Loaded)
		Reload();
	m_SessionStarted = true;

	m_SessionSrcBufferIndex = 0;
	m_SessionAuxBufferIndex = 1;
	m_SessionDestBufferIndex = 2;

	cl_int err = m_CommandQueue.enqueueFillBuffer<float>
		(m_ImageBuffers[m_SessionSrcBufferIndex], (float)m_StartTemperature, 0, 
			m_ImageWidth * m_ImageHeight * sizeof(float));
	Utils::AssertCLError(err);

	err = m_CommandQueue.enqueueBarrierWithWaitList();
	Utils::AssertCLError(err);

	ClearParamsCache();
	PrepareParamsStep();
	ApplyHeater(m_SessionSrcBufferIndex);

	err = m_CommandQueue.flush();
	Utils::AssertCLError(err);

	m_CurrentTime = 0;
}

void Simulator::EnqueueStepsGPU(uint numSteps)
{
	m_GPUTimer.Start();
	m_GPUNumSteps = numSteps;
	cl_int err;
	ClearParamsCache();
	for (uint i = 0; i < numSteps; i++)
	{
		PrepareParamsStep();
		Blur(false, m_SessionSrcBufferIndex, m_SessionAuxBufferIndex);
		Blur(true, m_SessionAuxBufferIndex, m_SessionDestBufferIndex);
		ApplyHeater(m_SessionDestBufferIndex);
		std::swap(m_SessionSrcBufferIndex, m_SessionDestBufferIndex);
		m_CurrentTime++;
	}

	ComputeDiff(m_SessionDestBufferIndex, m_SessionSrcBufferIndex, m_SessionAuxBufferIndex);
	ComputeSum(m_SessionAuxBufferIndex, m_SessionDestBufferIndex, &m_EnqueueResult, false, m_ImageHeight, &m_EnqueueEvent);

	m_EnqueueEvent.setCallback(CL_COMPLETE, &Simulator::GPUDoneCallback, this);

	err = m_CommandQueue.flush();
	Utils::AssertCLError(err);
}

void Simulator::GPUDoneCallback(cl_event event, cl_int type, void* userPointer)
{
	Simulator* sim = (Simulator*)userPointer;
	sim->m_GPUTimer.Loop();
	sim->m_OperationTime = sim->m_GPUTimer.GetDelta() * 1000.f;
	sim->m_OperationTimeOverNumSteps = sim->m_GPUTimer.GetDelta() * 1000.f / sim->m_GPUNumSteps;
}

bool Simulator::StepsDoneGPU(float& sumOfChanges)
{
	cl_int result;
	cl_int err = m_EnqueueEvent.getInfo(CL_EVENT_COMMAND_EXECUTION_STATUS, &result);
	Utils::AssertCLError(err);
	if (result == CL_COMPLETE)
	{
		err = m_CommandQueue.enqueueReadBuffer(m_ImageBuffers[m_SessionAuxBufferIndex], CL_TRUE, 0, sizeof(float), &sumOfChanges);
		Utils::AssertCLError(err);

		err = m_CommandQueue.enqueueBarrierWithWaitList();
		Utils::AssertCLError(err);
		return true;
	}
	return false;
}

void Simulator::OutputImage(const std::string& outputPath)
{
	cl_int err = m_CommandQueue.enqueueCopyBuffer(
		m_ImageBuffers[m_SessionSrcBufferIndex], m_ImageBuffers[m_SessionAuxBufferIndex], 
		0, 0, m_ImageWidth * m_ImageHeight * sizeof(float));
	Utils::AssertCLError(err);

	err = m_CommandQueue.enqueueBarrierWithWaitList();
	Utils::AssertCLError(err);

	float* finalImage = new float[m_ImageWidth * m_ImageHeight];
	err = m_CommandQueue.enqueueReadBuffer(m_ImageBuffers[m_SessionAuxBufferIndex], CL_TRUE, 0, m_ImageWidth * m_ImageHeight * sizeof(float), finalImage);
	Utils::AssertCLError(err);

	Image* image = new Image(m_ImageWidth, m_ImageHeight, finalImage);
	image->WriteToFile(outputPath);

	delete[] finalImage;
	delete image;
}

void Simulator::WriteToDXTexture(bool differenceImage)
{
	cl_int err;
	uint displayedBufferIndex = m_SessionSrcBufferIndex;

	if (m_Data.useCPUAndGPU)
		SendAllSharedToGPU();

	if (differenceImage)
	{
		PrepareParamsStep();
		Blur(false, m_SessionSrcBufferIndex, m_SessionAuxBufferIndex);
		Blur(true, m_SessionAuxBufferIndex, m_SessionDestBufferIndex);
		ApplyHeater(m_SessionDestBufferIndex);

		ComputeDiff(m_SessionSrcBufferIndex, m_SessionDestBufferIndex, m_SessionAuxBufferIndex);
		displayedBufferIndex = m_SessionAuxBufferIndex;
	}

	PrepareParamsOperation();
	PushIntParamToCache(m_ImageWidth);
	PushIntParamToCache(m_ImageHeight);
	if (differenceImage)
		PushIntParamToCache(1);
	else PushIntParamToCache(100000);
	err = m_CommandQueue.enqueueWriteBuffer(m_Parameters, CL_FALSE, 0, GetCacheSize(), GetCacheData());
	Utils::AssertCLError(err);

	err = m_CommandQueue.enqueueBarrierWithWaitList();
	Utils::AssertCLError(err);

	m_HeatmapKernel.setArg(0, m_Parameters);
	m_HeatmapKernel.setArg(1, m_ImageBuffers[displayedBufferIndex]);
	m_HeatmapKernel.setArg(2, m_Image);

	if (m_NVExtension)
	{
		err = clEnqueueAcquireD3D11ObjectsNV(m_CommandQueue(), 1, &m_Image(), 0, NULL, NULL);
	}
	else
	{
		err = clEnqueueAcquireD3D11ObjectsKHR(m_CommandQueue(), 1, &m_Image(), 0, NULL, NULL);
	}
	Utils::AssertCLError(err);

	err = m_CommandQueue.enqueueBarrierWithWaitList();
	Utils::AssertCLError(err);

	std::pair<uint, uint> globalWorkSize = std::make_pair(
		((m_ImageWidth + m_LocalWorkSize.first - 1) / m_LocalWorkSize.first) * m_LocalWorkSize.first,
		((m_ImageHeight + m_LocalWorkSize.second - 1) / m_LocalWorkSize.second) * m_LocalWorkSize.second
	);
	err = m_CommandQueue.enqueueNDRangeKernel(m_HeatmapKernel, cl::NullRange,
		cl::NDRange(globalWorkSize.first, globalWorkSize.second),
		cl::NDRange(m_LocalWorkSize.first, m_LocalWorkSize.second));
	Utils::AssertCLError(err);

	err = m_CommandQueue.enqueueBarrierWithWaitList();
	Utils::AssertCLError(err);

	if (m_NVExtension)
	{
		err = clEnqueueReleaseD3D11ObjectsNV(m_CommandQueue(), 1, &m_Image(), 0, NULL, NULL);
	}
	else
	{
		err = clEnqueueReleaseD3D11ObjectsKHR(m_CommandQueue(), 1, &m_Image(), 0, NULL, NULL);
	}
	Utils::AssertCLError(err);

	err = m_CommandQueue.enqueueBarrierWithWaitList();
	Utils::AssertCLError(err);

	err = m_CommandQueue.flush();
	Utils::AssertCLError(err);
}

void Simulator::UpdateDynamicData(const SimulatorData& data)
{
	m_Data.startTemperature = data.startTemperature;
	m_Data.heaterTemperature = data.heaterTemperature;
	m_Data.heaterLocation = data.heaterLocation;
	m_Data.numWorkerThreads = data.numWorkerThreads;
	m_Data.useWindowsThreads = data.useWindowsThreads;
	m_Data.localWorkSize = data.localWorkSize;
	m_Data.singleLocalWorkSize = data.singleLocalWorkSize;

	m_LocalWorkSize = data.localWorkSize;
	m_SingleWorkGroupSize = data.singleLocalWorkSize;

	m_StartTemperature = data.startTemperature;
	m_HeaterTemperature = data.heaterTemperature;
	m_HeaterLocation = data.heaterLocation;

	if (m_WorkerThreads.size() != data.numWorkerThreads)
	{
		m_WorkerThreads.resize(data.numWorkerThreads);
		m_WorkerThreadsData.resize(data.numWorkerThreads);
		SetThreadpoolThreadMaximum(m_Threadpool, data.numWorkerThreads);
		SetThreadpoolThreadMinimum(m_Threadpool, data.numWorkerThreads);
	}

	ChangeCPURow(data.cpuStartRow);
}

void Simulator::ClearParamsCache()
{
	for (auto cacheLine : m_Cache)
		delete[] cacheLine;
	m_Cache.clear();
}

void Simulator::PrepareParamsStep()
{
	m_Cache.push_back(new uint32[m_NumParamsPerStep]);
	m_CacheCurrentCount = 0;
}

void Simulator::PrepareParamsOperation()
{
	m_CacheOperationStart = m_CacheCurrentCount;
}

void Simulator::PushFloatParamToCache(float f)
{
	assert(m_CacheCurrentCount < (int)m_NumParamsPerStep);
	memcpy(&m_Cache.back()[m_CacheCurrentCount], &f, sizeof(float));
	m_CacheCurrentCount++;
}

void Simulator::PushIntParamToCache(int f)
{
	assert(m_CacheCurrentCount < (int)m_NumParamsPerStep);
	m_Cache.back()[m_CacheCurrentCount] = f;
	m_CacheCurrentCount++;
}

uint32* Simulator::GetCacheData()
{
	return m_Cache.back() + m_CacheOperationStart;
}

uint Simulator::GetCacheSize()
{
	return (m_CacheCurrentCount - m_CacheOperationStart) * sizeof(uint32);
}

void Simulator::PrintInfo()
{
	std::string name = m_Platform.getInfo<CL_PLATFORM_NAME>();
	std::string profile = m_Platform.getInfo<CL_PLATFORM_PROFILE>();
	std::string vendor = m_Platform.getInfo<CL_PLATFORM_VENDOR>();
	std::string version = m_Platform.getInfo<CL_PLATFORM_VERSION>();
	std::string extensions = m_Platform.getInfo<CL_PLATFORM_EXTENSIONS>();
	std::cout << "Platform info:\n";
	std::cout << "  Name: " << name << "\n";
	std::cout << "  Profile: " << profile << "\n";
	std::cout << "  Vendor: " << vendor << "\n";
	std::cout << "  Version: " << version << "\n";
	std::cout << "  Extensions:\n" << extensions << "\n\n";

	std::string driverVersion, clVersion;
	m_Device.getInfo(CL_DEVICE_MAX_WORK_ITEM_SIZES, &m_MaxWorkItemSizes);
	m_Device.getInfo(CL_DEVICE_NAME, &name);
	m_Device.getInfo(CL_DEVICE_VENDOR, &vendor);
	m_Device.getInfo(CL_DEVICE_PROFILE, &profile);
	m_Device.getInfo(CL_DEVICE_VERSION, &version);
	m_Device.getInfo(CL_DRIVER_VERSION, &driverVersion);
	m_Device.getInfo(CL_DEVICE_OPENCL_C_VERSION, &clVersion);
	m_Device.getInfo(CL_DEVICE_EXTENSIONS, &extensions);

	std::cout << "Device info:\n";
	std::cout << "  Name: " << name << "\n";
	std::cout << "  Profile: " << profile << "\n";
	std::cout << "  Vendor: " << vendor << "\n";
	std::cout << "  Version: " << version << "\n";
	std::cout << "  Driver Version: " << driverVersion << "\n";
	std::cout << "  OpenCL Version: " << clVersion << "\n";
	std::cout << "  Max Work Item Sizes: (";
	for (int i = 0; i < m_MaxWorkItemSizes.size(); i++)
	{
		std::cout << m_MaxWorkItemSizes[i] << ((i != m_MaxWorkItemSizes.size() - 1) ? ", " : ")\n");
	}
	std::cout << "  Extensions:\n" << extensions << "\n\n";
}

void Simulator::StartNewSessionShared()
{
	if (m_CPUThread.joinable())
		m_CPUThread.join();
	StartNewSessionGPU();
	for (uint i = 0; i < m_ImageHeight; i++)
	{
		for (uint j = 0; j < m_ImageWidth; j++)
		{
			m_FloatImages[0][i][j] = (float)m_StartTemperature;
		}
	}
	m_FloatImages[0][m_HeaterLocation.second][m_HeaterLocation.first] = m_HeaterTemperature;
	m_SessionSrcFloatImageIndex = 0;
	m_SessionDestFloatImageIndex = 1;
}

void Simulator::EnqueueStepsShared(uint numSteps)
{
	if (m_CPUThread.joinable())
		m_CPUThread.join();
	m_SharedStepsDone.store(false);
	m_CPUThread = std::thread(&Simulator::CPUThreadMain, this, numSteps);
}

void Simulator::CPUThreadMain(uint numSteps)
{
	cl_int err;
	Timer totalTimer;
	ClearParamsCache();
	for (uint i = 0; i < numSteps; i++)
	{
		////////////////////////////////////////// Send work to GPU
		PrepareParamsStep();
		Blur(false, m_SessionSrcBufferIndex, m_SessionAuxBufferIndex, m_CPUStartRow);
		Blur(true, m_SessionAuxBufferIndex, m_SessionDestBufferIndex, m_CPUStartRow);

		if(m_HeaterLocation.second < (int)m_CPUStartRow)
			ApplyHeater(m_SessionDestBufferIndex);
		err = m_CommandQueue.flush();
		Utils::AssertCLError(err);
		std::swap(m_SessionSrcBufferIndex, m_SessionDestBufferIndex);
		
		////////////////////////////////////////// Send work to CPU
		Timer cpuTimer;
		cpuTimer.Start();
		int start = m_CPUStartRow * m_ImageWidth, size = m_ImageWidth * (m_ImageHeight - m_CPUStartRow);
		if (m_WorkerThreads.size() == 1)
		{
			CPUWorkerMain(start, start + size);
		}
		else if(!m_Data.useWindowsThreads)
		{
			for (uint i = 0; i < m_WorkerThreads.size(); i++)
			{
				int workerSize = size / ((int)m_WorkerThreads.size() - i);
				m_WorkerThreads[i] = std::thread(&Simulator::CPUWorkerMain, this, start, start + workerSize);
				size -= workerSize;
				start += workerSize;
			}
			for (uint i = 0; i < m_WorkerThreads.size(); i++)
			{
				if(m_WorkerThreads[i].joinable())
					m_WorkerThreads[i].join();
			}
		}
		else
		{
			for (int i = 0; i < m_Data.numWorkerThreads; i++)
			{
				int workerSize = size / (m_Data.numWorkerThreads - i);
				m_WorkerThreadsData[i].simulator = this;
				m_WorkerThreadsData[i].startPos = start;
				m_WorkerThreadsData[i].endPos = start + workerSize;
				PTP_WORK work = CreateThreadpoolWork((PTP_WORK_CALLBACK)&Simulator::CPUWindowsWorkerMain, &m_WorkerThreadsData[i], &m_ThreadpoolEnv);
				SubmitThreadpoolWork(work);
				size -= workerSize;
				start += workerSize;
			}
			// WaitForThreadpoolWorkCallbacks()
			CloseThreadpoolCleanupGroupMembers(m_ThreadpoolCleanupGroup, FALSE, NULL);
		}
		if (m_HeaterLocation.second >= (int)m_CPUStartRow)
			m_FloatImages[m_SessionDestFloatImageIndex][m_HeaterLocation.second][m_HeaterLocation.first] = m_HeaterTemperature;
		std::swap(m_SessionSrcFloatImageIndex, m_SessionDestFloatImageIndex);
		cpuTimer.Loop();

		err = m_CommandQueue.finish();
		Utils::AssertCLError(err);

		float cpuTime = cpuTimer.GetDelta();
		////////////////////////////////////////// Sync and setup for the next step
		SyncWithGPU();
		m_CurrentTime++;
		//std::cout << "Total: " << totalTimer.GetDelta() * 1000.f << " ms; CPU: " << cpuTime * 1000.f << " ms;\n";
	}

	ComputeDiff(m_SessionDestBufferIndex, m_SessionSrcBufferIndex, m_SessionAuxBufferIndex, m_CPUStartRow);
	ComputeSum(m_SessionAuxBufferIndex, m_SessionDestBufferIndex, &m_EnqueueResult, false, m_CPUStartRow, &m_EnqueueEvent);

	float cpuSum = ComputeCPUSum(), gpuSum;

	err = m_CommandQueue.finish();
	Utils::AssertCLError(err);

	StepsDoneGPU(gpuSum);
	m_SharedResult = cpuSum + gpuSum;
	m_SharedStepsDone.store(true);

	totalTimer.Loop();
	m_OperationTime = totalTimer.GetDelta() * 1000.f;
	m_OperationTimeOverNumSteps = totalTimer.GetDelta() * 1000.f / numSteps;
}

void Simulator::SyncWithGPU()
{
	cl_int err;
	err = m_CommandQueue.enqueueWriteBuffer(m_ImageBuffers[m_SessionSrcBufferIndex], CL_FALSE,
		m_CPUStartRow * m_ImageWidth * sizeof(float), m_ImageWidth * sizeof(float), 
		m_FloatImages[m_SessionSrcFloatImageIndex][m_CPUStartRow]);
	Utils::AssertCLError(err);

	cl::Event copyEvent;
	err = m_CommandQueue.enqueueCopyBuffer(m_ImageBuffers[m_SessionSrcBufferIndex], m_ImageBuffers[m_SessionAuxBufferIndex],
		(m_CPUStartRow - 1) * m_ImageWidth * sizeof(float), (m_CPUStartRow - 1) * m_ImageWidth * sizeof(float), 
		m_ImageWidth * sizeof(float), nullptr, &copyEvent);
	Utils::AssertCLError(err);

	std::vector<cl::Event> copyEventInVector;
	copyEventInVector.push_back(copyEvent);
	err = m_CommandQueue.enqueueReadBuffer(m_ImageBuffers[m_SessionAuxBufferIndex], 
		CL_TRUE, (m_CPUStartRow - 1) * m_ImageWidth * sizeof(float), m_ImageWidth * sizeof(float),
		m_FloatImages[m_SessionSrcFloatImageIndex][m_CPUStartRow - 1], &copyEventInVector);
	Utils::AssertCLError(err);
}

void Simulator::SendAllSharedToGPU()
{
	cl_int err;
	err = m_CommandQueue.enqueueWriteBuffer(m_ImageBuffers[m_SessionSrcBufferIndex], CL_TRUE,
		m_CPUStartRow * m_ImageWidth * sizeof(float), (m_ImageHeight - m_CPUStartRow) * m_ImageWidth * sizeof(float),
		m_FloatImages[m_SessionSrcFloatImageIndex][m_CPUStartRow]);
	Utils::AssertCLError(err);

	err = m_CommandQueue.enqueueBarrierWithWaitList();
	Utils::AssertCLError(err);

	err = m_CommandQueue.flush();
	Utils::AssertCLError(err);
}

void Simulator::CPUWorkerMain(uint startPos, uint endPos)
{
	int offsets[9][2] = {
		{1, 1},
		{1, 0},
		{1, -1},
		{0, 1},
		{0, 0},
		{0, -1},
		{-1, 1},
		{-1, 0},
		{-1, -1}
	};
	for (uint pos = startPos; pos < endPos; pos++)
	{
		int x = pos % m_ImageWidth;
		int y = pos / m_ImageWidth;
		float sum = 0;

		for (uint i = 0; i < 9; i++)
		{
			int nx = x + offsets[i][0];
			int ny = y + offsets[i][1];

			if (nx < 0 || ny < 0 || nx >= (int)m_ImageWidth || ny >= (int)m_ImageHeight)
				sum += m_StartTemperature;
			else sum += m_FloatImages[m_SessionSrcFloatImageIndex][ny][nx];
		}

		m_FloatImages[m_SessionDestFloatImageIndex][y][x] = sum / 9.f;
	}
}

void Simulator::CPUWindowsWorkerMain(PTP_CALLBACK_INSTANCE instance, PVOID ptr, PTP_WORK work)
{
	CPUWindowsThreadData* data = (CPUWindowsThreadData*)ptr;
	data->simulator->CPUWorkerMain(data->startPos, data->endPos);
}

float Simulator::ComputeCPUSum()
{
	float result = 0.f;
	std::mutex mutex;
	int start = m_CPUStartRow * m_ImageWidth, size = m_ImageWidth * (m_ImageHeight - m_CPUStartRow);
	if (m_WorkerThreads.size() == 1)
	{
		CPUSumWorkerMain(&result, &mutex, start, start + size);
		return result;
	}
	else if(!m_Data.useWindowsThreads)
	{
		for (uint i = 0; i < m_WorkerThreads.size(); i++)
		{
			int workerSize = size / ((int)m_WorkerThreads.size() - i);
			m_WorkerThreads[i] = std::thread(&Simulator::CPUSumWorkerMain, this, &result, &mutex, start, start + workerSize);
			size -= workerSize;
			start += workerSize;
		}
		for (uint i = 0; i < m_WorkerThreads.size(); i++)
		{
			if(m_WorkerThreads[i].joinable())
				m_WorkerThreads[i].join();
		}
	}
	else
	{
		HANDLE mutex = CreateMutex(NULL, FALSE, NULL);
		for (int i = 0; i < m_Data.numWorkerThreads; i++)
		{
			int workerSize = size / ((int)m_WorkerThreads.size() - i);
			m_WorkerThreadsData[i].simulator = this;
			m_WorkerThreadsData[i].startPos = start;
			m_WorkerThreadsData[i].endPos = start + workerSize;
			m_WorkerThreadsData[i].mutex = mutex;
			m_WorkerThreadsData[i].result = &result;
			PTP_WORK work = CreateThreadpoolWork((PTP_WORK_CALLBACK)&Simulator::CPUWindowsSumWorkerMain, &m_WorkerThreadsData[i], &m_ThreadpoolEnv);
			SubmitThreadpoolWork(work);
			size -= workerSize;
			start += workerSize;
		}
		CloseThreadpoolCleanupGroupMembers(m_ThreadpoolCleanupGroup, FALSE, NULL);
		CloseHandle(mutex);
	}
	return result;
}

void Simulator::CPUSumWorkerMain(float* result, std::mutex* mutex, uint startPos, uint endPos)
{
	float sum = 0.f;
	for (uint i = startPos; i < endPos; i++)
		sum += fabsf(m_FloatImages[m_SessionSrcFloatImageIndex].data[i] -
			m_FloatImages[m_SessionDestFloatImageIndex].data[i]);
	mutex->lock();
	*result += sum;
	mutex->unlock();
}

void Simulator::CPUWindowsSumWorkerMain(PTP_CALLBACK_INSTANCE instance, PVOID ptr, PTP_WORK work)
{
	CPUWindowsThreadData* data = (CPUWindowsThreadData*)ptr;
	float sum = 0.f;
	for (uint i = data->startPos; i < data->endPos; i++)
		sum += fabsf(data->simulator->m_FloatImages[data->simulator->m_SessionSrcFloatImageIndex].data[i] -
			data->simulator->m_FloatImages[data->simulator->m_SessionDestFloatImageIndex].data[i]);
	WaitForSingleObject(data->mutex, INFINITE);
	*data->result += sum;
	ReleaseMutex(data->mutex);
}

bool Simulator::StepsDoneShared(float& sumOfChanges)
{
	bool done = m_SharedStepsDone.load();
	if (done)
	{
		sumOfChanges = m_SharedResult;
	}
	return done;
}

void Simulator::ChangeCPURow(uint newRow)
{
	cl_int err;
	if (newRow == m_CPUStartRow)
		return;
	if (newRow > m_CPUStartRow)
	{
		err = m_CommandQueue.enqueueWriteBuffer(
			m_ImageBuffers[m_SessionSrcBufferIndex],
			CL_TRUE, (m_CPUStartRow + 1) * m_ImageWidth * sizeof(float), 
			(newRow - m_CPUStartRow) * m_ImageWidth * sizeof(float), 
			m_FloatImages[m_SessionSrcFloatImageIndex][m_CPUStartRow + 1]);
		Utils::AssertCLError(err);
	}
	else
	{
		err = m_CommandQueue.enqueueCopyBuffer(
			m_ImageBuffers[m_SessionSrcBufferIndex], m_ImageBuffers[m_SessionAuxBufferIndex],
			(newRow - 1) * m_ImageWidth * sizeof(float), (newRow - 1) * m_ImageWidth * sizeof(float),
			(m_CPUStartRow - newRow) * m_ImageWidth * sizeof(float));
		Utils::AssertCLError(err);

		err = m_CommandQueue.enqueueBarrierWithWaitList();
		Utils::AssertCLError(err);

		err = m_CommandQueue.enqueueReadBuffer(m_ImageBuffers[m_SessionAuxBufferIndex], 
			CL_TRUE, (newRow - 1) * m_ImageWidth * sizeof(float), 
			(m_CPUStartRow - newRow) * m_ImageWidth * sizeof(float), 
			m_FloatImages[m_SessionSrcFloatImageIndex][newRow - 1]);
		Utils::AssertCLError(err);
	}
	m_Data.cpuStartRow = newRow;
	m_CPUStartRow = newRow;
}

void Simulator::StartNewSession()
{
	if (m_Data.useCPUAndGPU)
		StartNewSessionShared();
	else StartNewSessionGPU();
}

void Simulator::EnqueueSteps(uint numSteps)
{
	cl_int err = m_CommandQueue.finish();
	Utils::AssertCLError(err);

	if (m_Data.useCPUAndGPU)
		EnqueueStepsShared(numSteps);
	else EnqueueStepsGPU(numSteps);
}

bool Simulator::StepsDone(float& sumOfChanges)
{
	if (m_Data.useCPUAndGPU)
		return StepsDoneShared(sumOfChanges);
	return StepsDoneGPU(sumOfChanges);
}