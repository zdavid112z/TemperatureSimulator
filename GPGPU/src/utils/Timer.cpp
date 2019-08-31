#include "Pch.h"
#include "Timer.h"
#include <windows.h>

Timer::Timer(bool start)
{
	if(start)
		Start();
	else m_Stopped = true;
}

Timer::~Timer()
{

}

void Timer::Start()
{
	m_Delta = 0;
	m_Stopped = false;

	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
		std::cout << "QueryPerformanceFrequency failed!\n";

	m_Frequency = li.QuadPart;

	QueryPerformanceCounter(&li);
	m_LastLoop = li.QuadPart;
}

void Timer::Loop()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	m_Delta = float(li.QuadPart - m_LastLoop) / m_Frequency;
	m_LastLoop = li.QuadPart;
}

void Timer::Stop()
{
	m_Stopped = true;
}