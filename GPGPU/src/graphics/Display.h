#pragma once

#include "Pch.h"
#include <Windows.h>
#include "Graphics.h"
#include "Input.h"
#include <DirectXMath.h>

struct DisplayData {
	uint width, height;
	std::string title;
	bool fullscreen;
	bool vsync;
	int adapterIndex;
};

class Application;

class Display {
	friend class Graphics;
private:
	Display();
	void InitDisplay(const DisplayData&);
public:
	static Display* Main;
	static void Init(const DisplayData& data);
	static void Destroy();
public:
	~Display();

	void Run(Application*);

	void Close();
	bool IsRunning() const { return m_Running; }
	uint GetWidth() const { return m_Width; }
	uint GetHeight() const { return m_Height; }

	Graphics* GetGraphics() const { return m_Graphics; }
	Input* GetInput() const { return m_Input; }
	Application* GetApplication() const { return m_Application; }
private:
	void ProcessFrame();
	static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
	LRESULT CALLBACK DisplayWndProc(HWND, UINT, WPARAM, LPARAM);
private:
	uint m_Width, m_Height;
	std::string m_Title;
	HINSTANCE m_Instance;
	HWND m_Window;
	bool m_Running;
	bool m_Fullscreen;
	bool m_VSync;

	Graphics* m_Graphics = nullptr;
	Input* m_Input = nullptr;
	Application* m_Application = nullptr;
};