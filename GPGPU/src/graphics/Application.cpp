#include "Pch.h"
#include "Application.h"

Application::Application(const ApplicationData& data) :
	m_Timer(false)
{
	Display::Init(data.displayData);
	m_TimeSinceLastUpdate = 0;
	if(data.targetUPS > 0)
		m_UpdateInterval = 1.f / data.targetUPS;
	else m_UpdateInterval = 0.f;
}

Application::~Application()
{

}

void Application::Stop()
{
	Display::Main->Close();
}

void Application::Run(ApplicationData& data)
{
	m_Timer.Start();
	Display::Main->Run(this);
	data = GetAppData();
}

void Application::Restart()
{
	m_Restart = true;
	PostQuitMessage(0);
}

void Application::UpdateIfNeeded()
{
	m_Timer.Loop();
	if (m_UpdateInterval == 0)
	{
		Update(m_Timer.GetDelta());
		Display::Main->GetInput()->Update();
		return;
	}
	m_TimeSinceLastUpdate += m_Timer.GetDelta();
	while(m_TimeSinceLastUpdate >= m_UpdateInterval)
	{
		Update(m_UpdateInterval);
		Display::Main->GetInput()->Update();
		m_TimeSinceLastUpdate -= m_UpdateInterval;
	}
}