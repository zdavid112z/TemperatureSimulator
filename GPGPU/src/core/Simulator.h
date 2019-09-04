#pragma once

#include "Pch.h"
#include "CL/cl.hpp"
#include "Event.h"
#include "Image.h"
#include <thread>
#include <future>
#include "utils/Timer.h"
#include <Windows.h>

class Graphics;
class Texture2D;

struct SimulatorData
{
	std::string kernelSourcePath = "src/core/Simulator.cl";
	std::string temperatureKernelName = "Simulate";
	std::string sumKernelName = "Sum";
	std::string diffKernelName = "ComputeDiff";
	std::string heatmapKernelName = "Heatmap";
	uint imageWidth = 256;
	uint imageHeight = 256;
	int startTemperature = 200;
	std::pair<uint, uint> localWorkSize = std::make_pair(16, 16);
	uint singleLocalWorkSize = 256;
	int numParamsPerStep = 50;
	int numWorkerThreads = 12;
	int cpuStartRow = 200;
	std::pair<int, int> heaterLocation = std::make_pair(210, 210);
	float heaterTemperature = 10000;
	bool useCPUAndGPU = false;
	bool useWindowsThreads = true;
};

class Simulator;

struct CPUWindowsThreadData
{
	Simulator* simulator;
	uint startPos, endPos;
	HANDLE mutex;
	float* result;
};

class Simulator {
public:
	Simulator(Graphics* g, Texture2D* texture, const SimulatorData& data);
	~Simulator();

	void Reload();
	//void Start();

	void StartNewSession();
	void EnqueueSteps(uint numSteps);
	bool StepsDone(float& sumOfChanges);

	void OutputImage(const std::string& outputPath);
	void WriteToDXTexture(bool differenceImage);
	uint GetSessionTime() const { return m_CurrentTime; }
	void UpdateDynamicData(const SimulatorData& data);

	float GetLastOperationTime() const { return m_OperationTime; }
	float GetLastOperationTimePerStep() const { return m_OperationTimeOverNumSteps; }

	void PrintInfo();
	const std::vector<size_t>& GetMaxWorkItemSizes() const { return m_MaxWorkItemSizes; }
private:
	static void GPUDoneCallback(cl_event event, cl_int type, void* userPointer);
	void StartNewSessionShared();
	void EnqueueStepsShared(uint numSteps);
	bool StepsDoneShared(float& sumOfChanges);

	void StartNewSessionGPU();
	void EnqueueStepsGPU(uint numSteps);
	bool StepsDoneGPU(float& sumOfChanges);

	void CPUThreadMain(uint numSteps);
	void CPUWorkerMain(uint startPos, uint endPos);
	static void CPUWindowsWorkerMain(PTP_CALLBACK_INSTANCE instance, PVOID ptr, PTP_WORK work);
	void CPUSumWorkerMain(float* result, std::mutex* mutex, uint startPos, uint endPos);
	static void CPUWindowsSumWorkerMain(PTP_CALLBACK_INSTANCE instance, PVOID ptr, PTP_WORK work);
	void SyncWithGPU();
	void SendAllSharedToGPU();
	float ComputeCPUSum();
	void ChangeCPURow(uint newRow);

	void ApplyHeater(int destBufferIndex, cl::Event* doneEvent = nullptr);
	void Blur(bool horizontal, int srcBufferIndex, int destBufferIndex, int maxRow = 99999999, cl::Event* doneEvent = nullptr);
	void ComputeDiff(int initialBufferIndex, int finalBufferIndex, int destBufferIndex, int maxRow = 99999999);
	void ComputeSum(int srcBufferIndex, int auxBufferIndex, float* result, bool block, int maxRow = 99999999, cl::Event* doneEvent = nullptr);

	void ClearParamsCache();
	void PrepareParamsStep();
	void PrepareParamsOperation();
	void PushFloatParamToCache(float f);
	void PushIntParamToCache(int f);
	uint32* GetCacheData();
	uint GetCacheSize();
private:
	Graphics* m_Graphics;
	std::string m_ConfigPath;

	bool m_Loaded = false;
	cl::Context m_Context;
	cl::Device m_Device;
	cl::Platform m_Platform;
	cl::Program m_Program;
	cl::Kernel m_TemperatureKernel, m_SumKernel, m_DiffKernel, m_HeatmapKernel;
	cl::Buffer m_ImageBuffers[3], m_MaxChange, m_Parameters;
	cl::CommandQueue m_CommandQueue;
	cl::Image2D m_Image;
	Texture2D* m_DXTexture;
	std::vector<size_t> m_MaxWorkItemSizes;

	float m_HeaterTemperature;
	std::pair<int, int> m_HeaterLocation;
	int m_StartTemperature;
	uint m_ImageWidth, m_ImageHeight;
	std::pair<uint, uint> m_LocalWorkSize;
	uint m_SingleWorkGroupSize;

	std::atomic<uint> m_CurrentTime;
	std::atomic<float> m_OperationTime, m_OperationTimeOverNumSteps;
	std::vector<uint32*> m_Cache;
	int m_CacheCurrentStep, m_CacheCurrentCount, m_CacheOperationStart;
	float m_EnqueueResult;
	cl::Event m_EnqueueEvent;
	uint m_NumParamsPerStep;
	uint m_SessionSrcBufferIndex;
	uint m_SessionDestBufferIndex;
	uint m_SessionAuxBufferIndex;
	bool m_SessionStarted;
	Timer m_GPUTimer;
	uint m_GPUNumSteps;

	FloatImage m_FloatImages[2];
	uint m_CPUStartRow, m_SessionSrcFloatImageIndex, m_SessionDestFloatImageIndex;
	std::thread m_CPUThread;
	std::vector<std::thread> m_WorkerThreads;
	std::atomic_bool m_SharedStepsDone;
	float m_SharedResult;
	std::vector<CPUWindowsThreadData> m_WorkerThreadsData;
	TP_CALLBACK_ENVIRON m_ThreadpoolEnv;
	PTP_POOL m_Threadpool;
	PTP_CLEANUP_GROUP m_ThreadpoolCleanupGroup;
	SimulatorData m_Data;
	bool m_NVExtension;
};