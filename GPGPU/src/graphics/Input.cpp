#include "Pch.h"
#include "Input.h"

Input::Input()
{
	ZeroMemory(m_Keys, sizeof(m_Keys));
	ZeroMemory(m_MouseButtons, sizeof(m_MouseButtons));
	m_MouseX = m_MouseY = 0;
}

Input::~Input()
{

}

void Input::Update()
{
	for (int i = 0; i < sizeof(m_Keys) / sizeof(m_Keys[0]); i++)
		m_Keys[i] &= 2;
	for (int i = 0; i < sizeof(m_MouseButtons) / sizeof(m_MouseButtons[0]); i++)
		m_MouseButtons[i] &= 2;
}
