#pragma once

#include "Display.h"
#include "utils/Timer.h"
#include <DirectXMath.h>
#include "core/Simulator.h"

struct ApplicationData
{
	SimulatorData simData;
	DisplayData displayData;
	float targetUPS = 60;
};

class Application
{
public:
	Application(const ApplicationData& data);
	~Application();

	virtual void Stop();
	virtual void Run(ApplicationData& data);
	virtual void RenderUI() = 0;
	virtual void Render(Graphics*) = 0;
	virtual void Update(float delta) = 0;
	virtual void UpdateIfNeeded();
	virtual void OnResize(uint newWidth, uint newHeight) {}
	virtual DirectX::XMFLOAT3 GetClearColor() const { return m_ClearColor; }
	virtual ApplicationData GetAppData() const { return ApplicationData(); }
	virtual void Restart();
	virtual bool ShouldRestart() const { return m_Restart; }
protected:
	bool m_Restart = false;
	Timer m_Timer;
	float m_UpdateInterval, m_TimeSinceLastUpdate;
	DirectX::XMFLOAT3 m_ClearColor;
};