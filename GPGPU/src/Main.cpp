#include "Pch.h"
#include "graphics/Display.h"
#include <iostream>
#include "CL/cl.hpp"
#include <algorithm>
#include "core/Simulator.h"
#include "graphics/Application.h"
#include "graphics/Shader.h"
#include "graphics/Mesh.h"
#include "graphics/Camera.h"
#include "core/CLExtensions.h"
#include "graphics/Texture2D.h"
#include "vendor/imgui/imgui_nonewmacro.h"
#include "vendor/imgui/imgui_stdlib.h"
#include "vendor/imgui/imgui_internal.h"
#include "vendor/imgui/imgui_plot.h"
#include <random>
#include <ctime>

using namespace DirectX;

class SimulatorApplication : public Application
{
public:
	Mesh* mesh;
	Shader* shader;
	Camera* camera;
	Texture2D* texture;
	Simulator* simulator;
	SimulatorData simData, toChangeSimData;
	ApplicationData initialData;
	bool paused = false;
	int stepsPerOperation = 1;
	float meshScaledownFactor = 256;
	bool followCursor = false;
	float lastSumOfChanges = 0;
	bool displayDifference = false;
	bool singleStep = false;

	int plotNumGridLines = 5;
	int plotMaxHeight = 25;
	int historySize = 100;
	float operationTimesColor[3] = { 0.99f, 0.91f, 0.83f };
	float operationTimesPerStepColor[3] = { 0.83f, 0.99f, 0.92f };
	std::vector<float> operationTimes, operationTimesPerStep;
public:
	SimulatorApplication(const ApplicationData& data) :
		Application(data)
	{
		initialData = data;
		simData = data.simData;
		m_ClearColor.x = 0.84f;
		m_ClearColor.y = 0.89f;
		m_ClearColor.z = 0.44f;
		Graphics* g = Display::Main->GetGraphics();

		MeshData meshData = MeshFactory::GeneratePlane(1);
		mesh = new Mesh(g, meshData);
		shader = new Shader(g, L"src/graphics/shaders/SimpleShader.hlsl", L"src/graphics/shaders/SimpleShader.hlsl");
		camera = new Camera(
			XM_PIDIV2, 1.f * data.displayData.width / data.displayData.height, 
			0.01f, 1000.f, 0.0025f, 0.002f, 0.002f);
		texture = new Texture2D(g, simData.imageWidth, simData.imageHeight);

		simulator = new Simulator(g, texture, simData);
		simulator->StartNewSession();

		simData.singleLocalWorkSize = (uint)simulator->GetMaxWorkItemSizes()[0];
		int worksizeX = 1, worksizeY = (int)simulator->GetMaxWorkItemSizes()[1], bestWorksizeX, bestWorksizeY;
		bestWorksizeX = worksizeX;
		bestWorksizeY = worksizeY;
		while (worksizeY > 0)
		{
			worksizeX *= 2;
			worksizeY /= 2;
			if (std::abs(worksizeX - worksizeY) < std::abs(bestWorksizeX - bestWorksizeY))
			{
				bestWorksizeX = worksizeX;
				bestWorksizeY = worksizeY;
			}
		}
		simData.localWorkSize = std::make_pair(bestWorksizeX, bestWorksizeY);

		toChangeSimData.localWorkSize = simData.localWorkSize;
		toChangeSimData.singleLocalWorkSize = simData.singleLocalWorkSize;

		simulator->UpdateDynamicData(simData);

		simulator->EnqueueSteps(stepsPerOperation);
	}

	~SimulatorApplication()
	{
		delete simulator;
		delete texture;
		delete shader;
		delete mesh;
		delete camera;
		Display::Destroy();
	}

	void OnResize(uint newWidth, uint newHeight) override
	{
		camera->UpdateProjection(1.f * newWidth / newHeight);
	}

	char* Strdup(const char* str)
	{
		size_t len = strlen(str);
		char* copy = new char[len + 1];
		memcpy(copy, str, len + 1);
		return copy;
	}

	void PlotHistogram(const char* label, const std::vector<float>& times, float color[3])
	{
		std::mt19937 randEngine(42069);
		int size = std::min(historySize, (int)times.size());
		if (size == 0)
		{
			ImGui::PlotMultiLines(label, 0, nullptr, nullptr, [](const void* data, int idx) -> float
			{
				float* v = (float*)data;
				return v[idx];
			}, nullptr, 0, 0, (float)plotMaxHeight, ImVec2(0, 120));
			return;
		}
		std::vector<float*> grid;
		std::vector<char*> names;
		std::vector<ImColor> colors;

		std::uniform_real_distribution<float> urdh(0, 1);
		for (int i = 0; i < plotNumGridLines; i++)
		{
			float* line = new float[size];
			line[0] = 1.f * plotMaxHeight * (plotNumGridLines - i) / (plotNumGridLines + 1);
			for(int j = 1; j < size; j++)
				line[j] = line[0];
			grid.push_back(line);
			names.push_back(Strdup(std::to_string(line[0]).c_str()));
			float r, g, b;
			ImGui::ColorConvertHSVtoRGB(urdh(randEngine), 0.25f, 0.4f, r, g, b);
			colors.emplace_back(r, g, b);
		}
		grid.push_back((float*)&times[0]);
		names.push_back(Strdup(label));
		colors.emplace_back(color[0], color[1], color[2], 1.f);
		ImGui::PlotMultiLines(label, plotNumGridLines + 1, (const char**)&names[0], &colors[0], [](const void* data, int idx) -> float
		{
			float* v = (float*)data;
			return v[idx];
		}, (void**)&grid[0], size, 0, (float)plotMaxHeight, ImVec2(0, 120));
		for (int i = 0; i < plotNumGridLines; i++)
		{
			delete[] grid[i];
			delete[] names[i];
		}
		delete[] names[plotNumGridLines];
	}

	virtual void RenderUI() override
	{
		ImGui::Begin("Temperature Simulator Control Panel");
		ImGui::Text("Stats");
		PlotHistogram("Operation times", operationTimes, operationTimesColor);
		PlotHistogram("Operation times per step", operationTimesPerStep, operationTimesPerStepColor);
		ImGui::SliderInt("History size", &historySize, 10, 2000);
		ImGui::SliderInt("Num plot lines", &plotNumGridLines, 0, 5);
		ImGui::SliderInt("Max plot height", &plotMaxHeight, 5, 200);

		ImGui::Separator();
		ImGui::Text("Dynamic Simulator Data");
		ImGui::Text("Current step: %d", simulator->GetSessionTime());
		ImGui::Text("Sum of changes / size: %f",lastSumOfChanges);
		if (ImGui::TreeNode("CPU Settings"))
		{
			ImGui::Text("CPU doing %.2f %% of the work", (float)(simData.imageHeight - simData.cpuStartRow) / simData.imageHeight * 100.f);
			ImGui::SliderInt("CPU Row", &simData.cpuStartRow, 2, simData.imageHeight - 2);
			ImGui::SliderInt("CPU Workers", &simData.numWorkerThreads, 1, 32);
			ImGui::Checkbox("Use Windows Threadpools", &simData.useWindowsThreads);
			ImGui::TreePop();
			ImGui::Separator();
		}
		if (ImGui::TreeNode("Environment Settings"))
		{
			ImGui::SliderInt("Ambient Temperature", &simData.startTemperature, 0, 1000);
			ImGui::SliderInt("Ambient Temperature (fast)", &simData.startTemperature, 0, 50000);
			ImGui::SliderFloat("Heater Temperature", &simData.heaterTemperature, 0, 10000);
			ImGui::SliderFloat("Heater Temperature (fast)", &simData.heaterTemperature, 10000, 1000000);
			ImGui::SliderInt("Heater Location X", &simData.heaterLocation.first, 0, simData.imageWidth - 1);
			ImGui::SliderInt("Heater Location Y", &simData.heaterLocation.second, 0, simData.imageHeight - 1);
			ImGui::Checkbox("Heater follow cursor ('F' key)", &followCursor);
			ImGui::TreePop();
			ImGui::Separator();
		}
		if (ImGui::TreeNode("Graphics Settings"))
		{
			ImGui::SliderFloat("Mesh scaledown factor", &meshScaledownFactor, 1, 4096);
			ImGui::ColorEdit3("Clear color", (float*)&m_ClearColor);
			ImGui::ColorEdit3("Operation times color", operationTimesColor);
			ImGui::ColorEdit3("Operation times per step color", operationTimesPerStepColor);
			ImGui::Checkbox("Display difference ('D' key)", &displayDifference);
			if (ImGui::Button("Reset view"))
			{
				camera->ResetView();
			}
			ImGui::TreePop();
			ImGui::Separator();
		}
		if (ImGui::TreeNode("Simulation Settings"))
		{
			ImGui::InputInt("Single Local Work Size", (int*)&toChangeSimData.singleLocalWorkSize, 0, 0);
			ImGui::InputInt("Local Work Size on X", (int*)&toChangeSimData.localWorkSize.first, 0, 0);
			ImGui::InputInt("Local Work Size on Y", (int*)&toChangeSimData.localWorkSize.second, 0, 0);
			ImGui::SliderInt("Steps per operation", &stepsPerOperation, 1, 200);
			if (ImGui::Button("Update worksizes"))
			{
				simData.localWorkSize = toChangeSimData.localWorkSize;
				simData.singleLocalWorkSize = toChangeSimData.singleLocalWorkSize;
			}
			ImGui::TreePop();
			ImGui::Separator();
		}

		simData.heaterLocation.first = std::max(std::min(simData.heaterLocation.first, (int)simData.imageWidth - 1), 0);
		simData.heaterLocation.second = std::max(std::min(simData.heaterLocation.second, (int)simData.imageHeight - 1), 0);
		
		if (ImGui::Button("Pause"))
		{
			paused = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Resume"))
		{
			paused = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("Step Once"))
		{
			singleStep = true;
		}
		
		if (ImGui::Button("Restart Session"))
		{
			simulator->UpdateDynamicData(simData);
			simulator->StartNewSession();
			simulator->EnqueueSteps(stepsPerOperation);
		}
		ImGui::Separator();
		ImGui::Text("Static Simulator Data");
		ImGui::InputInt("Image width", (int*)&toChangeSimData.imageWidth, 0, 0);
		ImGui::InputInt("Image height", (int*)&toChangeSimData.imageHeight, 0, 0);
		ImGui::Checkbox("Use GPU and CPU", &toChangeSimData.useCPUAndGPU);
		std::string workItemSizesDisplayString = "Max work item sizes: (";
		for (int i = 0; i < simulator->GetMaxWorkItemSizes().size(); i++)
		{
			workItemSizesDisplayString += std::to_string(simulator->GetMaxWorkItemSizes()[i]);
			workItemSizesDisplayString += ((i != simulator->GetMaxWorkItemSizes().size() - 1) ? ", " : ")");
		}
		ImGui::Text(workItemSizesDisplayString.c_str());
		ImGui::SliderInt("Paraps per step cache", (int*)&toChangeSimData.numParamsPerStep, 40, 120);
		if (ImGui::Button("Restart"))
		{
			simData.imageWidth = toChangeSimData.imageWidth;
			simData.imageHeight = toChangeSimData.imageHeight;
			simData.numParamsPerStep = toChangeSimData.numParamsPerStep;
			simData.heaterLocation.first = std::min(simData.heaterLocation.first, (int)simData.imageWidth - 1);
			simData.heaterLocation.second = std::min(simData.heaterLocation.second, (int)simData.imageHeight - 1);
			simData.useCPUAndGPU = toChangeSimData.useCPUAndGPU;

			if (texture->GetWidth() != simData.imageWidth || texture->GetHeight() != simData.imageHeight)
			{
				delete texture;
				texture = new Texture2D(Display::Main->GetGraphics(), simData.imageWidth, simData.imageHeight);
			}
			delete simulator;
			simulator = new Simulator(Display::Main->GetGraphics(), texture, simData);
			simulator->StartNewSession();
			simulator->EnqueueSteps(stepsPerOperation);
		}
#if 0
		static int currentAdapter = 0;
		ImGui::Combo("Video Adapters", &currentAdapter, 
			[](void* data, int idx, const char** out_text) -> bool {
				Graphics* g = (Graphics*)data;
				*out_text = g->GetAdaptersInfo()[idx].second.c_str();
				return true;
			}, 
			Display::Main->GetGraphics(), 
			Display::Main->GetGraphics()->GetAdaptersInfo().size());
		if (ImGui::Button("Change Adapter"))
		{
			initialData.displayData.adapterIndex = Display::Main->GetGraphics()->GetAdaptersInfo()[currentAdapter].first;
			Restart();
		}
#endif
		ImGui::End();
	}

	virtual void Render(Graphics* g) override
	{
		if ((!paused || singleStep) && simulator->StepsDone(lastSumOfChanges))
		{
			singleStep = false;
			lastSumOfChanges /= simData.imageWidth;
			lastSumOfChanges /= simData.imageHeight;

			operationTimes.push_back(simulator->GetLastOperationTime());
			operationTimesPerStep.push_back(simulator->GetLastOperationTimePerStep());
			if (operationTimes.size() > historySize)
			{
				operationTimes.erase(operationTimes.begin(), 
					operationTimes.begin() + operationTimes.size() - historySize);
			}
			if (operationTimesPerStep.size() > historySize)
			{
				operationTimesPerStep.erase(operationTimesPerStep.begin(), 
					operationTimesPerStep.begin() + operationTimesPerStep.size() - historySize);
			}
			
			simulator->WriteToDXTexture(displayDifference);
			simulator->UpdateDynamicData(simData);
			simulator->EnqueueSteps(stepsPerOperation);
			//simulator->OutputImage("out" + std::to_string(simulator->GetSessionTime()) + ".bmp");
			//std::cout << (diff / 1024) / 1024 << " : " << simulator->GetSessionTime() << std::endl;
		}

		XMMATRIX scale = XMMatrixScaling((float)simData.imageWidth / meshScaledownFactor, (float)simData.imageHeight / meshScaledownFactor, 1.f);
		shader->UpdateBuffer(g, camera, scale);
		g->Render(shader, mesh, texture);
	}

	virtual void Update(float delta) override
	{
		camera->Update(delta);
		if (Display::Main->GetInput()->GetKeyState('D') == KEY_JUST_RELEASED)
			displayDifference = !displayDifference;
		if (Display::Main->GetInput()->GetKeyState('F') == KEY_JUST_RELEASED)
			followCursor = !followCursor;
		if (followCursor)
		{
			float cursorX = (float)Display::Main->GetInput()->GetMouseX();
			float cursorY = (float)Display::Main->GetInput()->GetMouseY();
			XMMATRIX viewProjection = camera->GetViewProjection();
			XMVECTOR cursorPos = XMVectorSet(
				(cursorX / Display::Main->GetWidth()) * 2.f - 1.f,
				-((cursorY / Display::Main->GetHeight()) * 2.f - 1.f),
				0.5f, 1.f);
		
			XMVECTOR viewProjectionDet = XMMatrixDeterminant(viewProjection);
			XMMATRIX invViewProjection = XMMatrixInverse(&viewProjectionDet, viewProjection);

			XMVECTOR pointInWorld = XMVector4Transform(cursorPos, invViewProjection);
			pointInWorld = XMVectorScale(pointInWorld, 1.f / XMVectorGetW(pointInWorld));
			XMVECTOR direction = XMVectorSubtract(camera->GetPosition(), pointInWorld);

			XMFLOAT4 c, d;
			XMStoreFloat4(&c, camera->GetPosition());
			XMStoreFloat4(&d, direction);
			float pointX = -c.z * d.x / d.z + c.x;
			float pointY = -c.z * d.y / d.z + c.y;

			pointX = (pointX * meshScaledownFactor + simData.imageWidth) / 2.f;
			pointY = (pointY * meshScaledownFactor + simData.imageHeight) / 2.f;

			if (pointX >= 0 && pointX < simData.imageWidth && pointY >= 0 && pointY < simData.imageHeight)
			{
				simData.heaterLocation.first = (int)pointX;
				simData.heaterLocation.second = (int)pointY;
			}
		}
	}

	ApplicationData GetAppData() const override
	{
		ApplicationData data = initialData;
		data.simData = simData;
		return data;
	}
};

int main(int argc, char** args)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	DisplayData data;
	data.fullscreen = false;
	data.title = "Temperature Simulator";
	data.width = 1280;
	data.height = 720;
	data.adapterIndex = 0;

	ApplicationData appdata;
	appdata.displayData = data;
	appdata.targetUPS = 0;

	SimulatorData simData;
	appdata.simData = simData;

	SimulatorApplication* app = nullptr;
	do
	{
		if (app != nullptr)
			delete app;
		app = new SimulatorApplication(appdata);
		std::cout << "GPU Info: \n" << Display::Main->GetGraphics()->GetVideoCardInfo() << "\n\n";
		app->Run(appdata);
	} while(app->ShouldRestart());
	delete app;

}