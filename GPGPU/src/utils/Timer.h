#pragma once

class Timer
{
public:
	Timer(bool start = true);
	~Timer();

	void Start();
	void Loop();
	void Stop();
	float GetDelta() const { return m_Delta * !m_Stopped; }
private:
	__int64 m_Frequency;
	__int64 m_LastLoop;
	float m_Delta;
	bool m_Stopped;
};