#include "Pch.h"
#include "Display.h"
#include "Graphics.h"
#include "Application.h"
#include <windowsx.h>
#include "vendor/imgui/imgui_nonewmacro.h"
#include "vendor/imgui/imgui_impl_dx11.h"
#include "vendor/imgui/imgui_impl_win32.h"

Display* Display::Main = nullptr;

void Display::Init(const DisplayData& data)
{
	Main = new Display();
	Main->InitDisplay(data);
}

void Display::Destroy()
{
	delete Main;
}

Display::Display()
{

}

void Display::InitDisplay(const DisplayData& data)
{
	m_Title = data.title;
	m_Fullscreen = data.fullscreen;
	m_VSync = data.vsync;

	WNDCLASSEX wc;
	DEVMODE dmScreenSettings;
	int posX, posY;

	// Get the instance of this application.
	m_Instance = GetModuleHandle(NULL);

	// Setup the windows class with default settings.
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_Instance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = data.title.c_str();
	wc.cbSize = sizeof(WNDCLASSEX);

	// Register the window class.
	RegisterClassEx(&wc);

	// Determine the resolution of the clients desktop screen.
	m_Width = GetSystemMetrics(SM_CXSCREEN);
	m_Height = GetSystemMetrics(SM_CYSCREEN);

	// Setup the screen settings depending on whether it is running in full screen or in windowed mode.
	if (data.fullscreen)
	{
		// If full screen set the screen to maximum size of the users desktop and 32bit.
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = m_Width;
		dmScreenSettings.dmPelsHeight = m_Height;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// Change the display settings to full screen.
		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

		// Set the position of the window to the top left corner.
		posX = posY = 0;
	}
	else
	{
		// If windowed then set it to 800x600 resolution.
		m_Width = data.width;
		m_Height = data.height;

		// Place the window in the middle of the screen.
		posX = (GetSystemMetrics(SM_CXSCREEN) - m_Width) / 2;
		posY = (GetSystemMetrics(SM_CYSCREEN) - m_Height) / 2;
	}

	// Create the window with the screen settings and get the handle to it.
	m_Window = CreateWindowEx(WS_EX_APPWINDOW, data.title.c_str(), data.title.c_str(),
		WS_OVERLAPPEDWINDOW,
		posX, posY, m_Width, m_Height, NULL, NULL, m_Instance, NULL);

	m_Graphics = new Graphics(this, data.adapterIndex);
	m_Input = new Input();

	// Bring the window up on the screen and set it as main focus.
	ShowWindow(m_Window, SW_SHOW);
	SetForegroundWindow(m_Window);
	SetFocus(m_Window);
	UpdateWindow(m_Window);

	// Hide the mouse cursor.
	//ShowCursor(false);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(m_Window);
	ImGui_ImplDX11_Init(m_Graphics->GetDevice(), m_Graphics->GetDeviceContext());

	m_Running = true;
}

Display::~Display()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	delete m_Input;
	delete m_Graphics;

	if (m_Fullscreen)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	DestroyWindow(m_Window);
	UnregisterClass(m_Title.c_str(), m_Instance);
}

void Display::Run(Application* app)
{
	m_Application = app;

	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));
	while (m_Running)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message != WM_QUIT)
		{
			ProcessFrame();
		}
		else m_Running = false;
	}
}

void Display::Close()
{
	m_Running = false;
}

void Display::ProcessFrame()
{
	m_Application->UpdateIfNeeded();

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	m_Application->RenderUI();

	ImGui::Render();
	m_Graphics->BeginScene(m_Application->GetClearColor());

	m_Application->Render(m_Graphics);

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	m_Graphics->Present();
}

LRESULT CALLBACK Display::WndProc(HWND hWnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	return Display::Main->DisplayWndProc(hWnd, umsg, wparam, lparam);
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK Display::DisplayWndProc(HWND hWnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, umsg, wparam, lparam))
		return true;

	switch (umsg)
	{
	case WM_SIZE:
		if (m_Graphics->GetDevice() != NULL && wparam != SIZE_MINIMIZED)
		{
			m_Width = (UINT)LOWORD(lparam);
			m_Height = (UINT)HIWORD(lparam);
			m_Graphics->Resize(m_Width, m_Height);
			if(m_Application != nullptr)
				m_Application->OnResize(m_Width, m_Height);
		}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	case WM_KEYDOWN:
		m_Input->m_Keys[(unsigned int)wparam] = KEY_JUST_PRESSED;
		return 0;
	case WM_KEYUP:
		m_Input->m_Keys[(unsigned int)wparam] = KEY_JUST_RELEASED;
		return 0;
	case WM_MOUSEMOVE:
		m_Input->m_MouseX = GET_X_LPARAM(lparam);
		m_Input->m_MouseY = GET_Y_LPARAM(lparam);
		return 0;
	case WM_LBUTTONDOWN:
		m_Input->m_MouseButtons[VK_LBUTTON] = KEY_JUST_PRESSED;
		return 0;
	case WM_LBUTTONUP:
		m_Input->m_MouseButtons[VK_LBUTTON] = KEY_JUST_RELEASED;
		return 0;
	case WM_MBUTTONDOWN:
		m_Input->m_MouseButtons[VK_MBUTTON] = KEY_JUST_PRESSED;
		return 0;
	case WM_MBUTTONUP:
		m_Input->m_MouseButtons[VK_MBUTTON] = KEY_JUST_RELEASED;
		return 0;
	case WM_RBUTTONDOWN:
		m_Input->m_MouseButtons[VK_RBUTTON] = KEY_JUST_PRESSED;
		return 0;
	case WM_RBUTTONUP:
		m_Input->m_MouseButtons[VK_RBUTTON] = KEY_JUST_RELEASED;
		return 0;
	default:
		return DefWindowProc(hWnd, umsg, wparam, lparam);
	}
}