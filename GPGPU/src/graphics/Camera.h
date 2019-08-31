#pragma once

#include "Pch.h"
#include <DirectXMath.h>

class Camera
{
public:
	Camera(float fov, float aspect, float znear, float zfar, float zoomSpeed = 1, float panSpeed = 1, float rotateSpeed = 1);

	void Update(float delta);
	void UpdateProjection(float aspect);
	void UpdateView();
	void ResetView();

	DirectX::XMMATRIX GetViewProjection() const;
	DirectX::XMMATRIX GetProjection() const { return m_Projection; }
	DirectX::XMMATRIX GetView() const { return m_View; }

	DirectX::XMVECTOR GetPosition() const;
	DirectX::XMVECTOR GetRotation() const;
	DirectX::XMVECTOR GetTargetPosition() const;

	DirectX::XMVECTOR GetForward() const;
	DirectX::XMVECTOR GetRight() const;
	DirectX::XMVECTOR GetUp() const;
private:
	void Pan(float dx, float dy);
	void Zoom(float amount);
	void Rotate(float dx, float dy);
private:
	float m_Fov, m_Aspect, m_ZNear, m_ZFar;
	DirectX::XMMATRIX m_Projection, m_View;
	DirectX::XMVECTOR m_TargetPosition;
	DirectX::XMVECTOR m_LocalForward;
	DirectX::XMVECTOR m_LocalRight;
	DirectX::XMVECTOR m_LocalUp;
	float m_Pitch, m_Yaw, m_AngleThreshold;
	float m_Distance;

	float m_ZoomSpeed, m_PanSpeed, m_RotateSpeed;
	bool m_Grabbed;
	float m_OldMouseX, m_OldMouseY;
};