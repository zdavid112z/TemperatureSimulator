#include "Pch.h"
#include "Camera.h"
#include "Display.h"

using namespace DirectX;

Camera::Camera(float fov, float aspect, float znear, float zfar,
	float zoomSpeed, float panSpeed, float rotateSpeed) :
	m_Fov(fov), m_Aspect(aspect), m_ZNear(znear), m_ZFar(zfar),
	m_Grabbed(false), m_ZoomSpeed(zoomSpeed), m_PanSpeed(panSpeed),
	m_RotateSpeed(rotateSpeed), m_AngleThreshold(0.01f)
{
	UpdateProjection(aspect);
	ResetView();
}

void Camera::ResetView()
{
	m_TargetPosition = XMVectorSet(0, 0, 0, 1);
	m_LocalForward = XMVectorSet(0, 0, 1, 0);
	m_LocalUp = XMVectorSet(0, 1, 0, 0);
	m_LocalRight = XMVectorSet(1, 0, 0, 0);
	m_Distance = 5.f;
	m_Yaw = 0;
	m_Pitch = 0;
	UpdateView();
}

void Camera::Update(float delta)
{
	Input* input = Display::Main->GetInput();

	if (GetAsyncKeyState(VK_LMENU))
	{
		if (!m_Grabbed)
		{
			m_OldMouseX = (float)input->GetMouseX();
			m_OldMouseY = (float)input->GetMouseY();
			m_Grabbed = true;
		}
		float dx = m_OldMouseX - input->GetMouseX();
		float dy = m_OldMouseY - input->GetMouseY();
		dy = -dy;

		m_OldMouseX = (float)input->GetMouseX();
		m_OldMouseY = (float)input->GetMouseY();

		if (input->GetMouseButtonDown(VK_LBUTTON))
		{
			Rotate(dx * m_RotateSpeed, dy * m_RotateSpeed);
		}
		if (input->GetMouseButtonDown(VK_RBUTTON))
		{
			Zoom(dy * m_ZoomSpeed);
		}
		if (input->GetMouseButtonDown(VK_MBUTTON))
		{
			Pan(dx * m_PanSpeed, dy * m_PanSpeed);
		}

		UpdateView();
	}
}

void Camera::UpdateView()
{
	m_View = XMMatrixLookAtLH(GetPosition(), GetTargetPosition(), GetUp());
}

void Camera::Pan(float dx, float dy)
{
	m_TargetPosition = XMVectorAdd(m_TargetPosition, XMVectorScale(GetRight(), dx));
	m_TargetPosition = XMVectorAdd(m_TargetPosition, XMVectorScale(GetUp(), dy));
}

void Camera::Zoom(float amount)
{
	m_Distance -= amount;
	if (m_Distance < 0.01f)
		m_Distance = 0.01f;
}

void Camera::Rotate(float dx, float dy)
{
	m_Yaw += dx;
	m_Pitch -= dy;
}

void Camera::UpdateProjection(float aspect)
{
	m_Aspect = aspect;
	m_Projection = XMMatrixPerspectiveFovLH(m_Fov, m_Aspect, m_ZNear, m_ZFar);
}

DirectX::XMMATRIX Camera::GetViewProjection() const
{
	return XMMatrixMultiply(m_View, m_Projection);
}

DirectX::XMVECTOR Camera::GetTargetPosition() const
{
	return m_TargetPosition;
}

DirectX::XMVECTOR Camera::GetPosition() const
{
	return XMVectorSubtract(m_TargetPosition, 
		XMVectorScale(GetForward(), m_Distance));
}

DirectX::XMVECTOR Camera::GetRotation() const
{
	return XMQuaternionRotationRollPitchYaw(m_Pitch, m_Yaw, 0);
}

DirectX::XMVECTOR Camera::GetForward() const
{
	return XMVector3Rotate(m_LocalForward, XMQuaternionConjugate(GetRotation()));
}

DirectX::XMVECTOR Camera::GetRight() const
{
	return XMVector3Rotate(m_LocalRight, XMQuaternionConjugate(GetRotation()));
}

DirectX::XMVECTOR Camera::GetUp() const
{
	return XMVector3Rotate(m_LocalUp, XMQuaternionConjugate(GetRotation()));
}