#pragma once

#include <Windows.h>
#include "Pch.h"

class Display;

enum KeyState : byte
{
	KEY_RELEASED = 0,
	KEY_JUST_RELEASED = 1,
	KEY_PRESSED = 2,
	KEY_JUST_PRESSED = 3
};

class Input {
	friend class Display;
public:
	Input();
	~Input();

	void Update();
	bool GetKeyDown(uint key) const { return (m_Keys[key] & 2) != 0; }
	bool GetKeyUp(uint key) const { return (m_Keys[key] & 2) == 0; }
	KeyState GetKeyState(uint key) const { return (KeyState)m_Keys[key]; }

	bool GetMouseButtonDown(uint mb) const { return (m_MouseButtons[mb] & 2) != 0; }
	bool GetMouseButtonUp(uint mb) const { return (m_MouseButtons[mb] & 2) != 0; }
	KeyState GetMouseButtonState(uint mb) const { return (KeyState)m_MouseButtons[mb]; }

	int GetMouseX() const { return m_MouseX; }
	int GetMouseY() const { return m_MouseY; }
private:
	byte m_Keys[256];
	byte m_MouseButtons[16];
	int m_MouseX, m_MouseY;
};