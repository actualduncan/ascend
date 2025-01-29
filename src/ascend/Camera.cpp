#include "Camera.h"
#include <algorithm>
#include <iostream>

using namespace DirectX;

float clamp(float n, float lower, float upper)
{
	return n <= lower ? lower : n >= upper ? upper : n;
}

Camera::Camera(HWND window, float aspectRatio, float minZ, float maxZ)
	: m_camPosition(0.0f, 3.7f, -3.5f)
	, m_camRotation(0.0f, 0.0f, 0.0f)
	, m_camLookAt(0.0f, 0.0f, 1.0f)
	, m_camRight(0.0f, 0.0f, 0.0f)
	, m_camRotRate(0.1f)
	, m_movespeed(30.0f)
	, m_window(window)
	, m_cursor()
{

	WindowSizeChanged(aspectRatio, minZ, maxZ);
	
}

Camera::~Camera()
{

}

void Camera::WindowSizeChanged(float aspectRatio, float minZ, float maxZ)
{
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	m_projectionMatrix = XMMatrixPerspectiveFovRH(
		fovAngleY,
		aspectRatio,
		minZ,
		maxZ
	);
}

void Camera::Update(float dt)
{
	GetClientRect(m_window, &m_viewportRect);

	//m_cursor.x = input.mouseX;
	//m_cursor.y = input.mouseY;


 	if (/*input.mouseRButtonDown&&*/ isMouseActive)
	{
		int deltax = m_cursor.x - (m_viewportRect.right / 2);
		int deltay = m_cursor.y - (m_viewportRect.bottom / 2);
		
		Rotate(deltax, deltay, dt);
		m_cursor.x = (m_viewportRect.right / 2);
		m_cursor.y = (m_viewportRect.bottom / 2);

		SetCursorPos(m_cursor.x, m_cursor.y);
	}
	
	if (/*input.mouseRButtonDown && */ !isMouseActive)
	{
		m_cursor.x = (m_viewportRect.right / 2);
		m_cursor.y = (m_viewportRect.bottom / 2);

		SetCursorPos(m_cursor.x, m_cursor.y);
		ShowCursor(false);
		isMouseActive = true;
	}

	if (/*!input.mouseRButtonDown &&*/ isMouseActive)
	{
		ShowCursor(true);
		isMouseActive = false;
	}
	
	Move(XMVECTOR(0.0f/*input.horizontalX*/, 0.0f/*input.vertical*/, 0.0f/*input.horizontalZ*/), dt);

	CalculateView();
}

void Camera::CalculateView()
{
	XMVECTOR positionv;
	positionv = XMLoadFloat3(&m_camPosition);
	float pitch = m_camRotation.x * 0.0174532f;
	float yaw = m_camRotation.y * 0.0174532f;
	float roll = m_camRotation.z * 0.0174532f;

	m_camLookAt = XMVECTOR(0.0, 0.0, 1.0f);
	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(roll, pitch, yaw);

	m_camLookAt = XMVector3TransformCoord(m_camLookAt, rotationMatrix);
	m_camLookAt = m_camLookAt + positionv;

	m_camUp = XMVector3TransformCoord(XMVECTOR(0.0f, 1.0f, 0.0f), rotationMatrix);
	m_camRight = XMVector3TransformCoord(XMVECTOR(1.0f, 0.0f, 0.0f), rotationMatrix);
	m_forward = XMVector3Cross(m_camUp, -m_camRight);

	m_view = XMMatrixLookAtLH(positionv, m_camLookAt, XMVECTOR(0.0f, 1.0f, 0.0f));
}

void Camera::Rotate(int deltaX, int deltaY, float dt)
{
	m_camRotation.x += deltaY / m_camRotRate * dt;
    m_camRotation.y -= deltaX / m_camRotRate * dt;
	m_camRotation.z = 0;

	// clamp x rotation by 90 degrees
	m_camRotation.x = clamp(m_camRotation.x, -90.0f, 90.0f);
}

void Camera::Move(XMVECTOR direction, float dt)
{

	m_camPosition += (m_forward * (direction.m128_f32[2] * m_movespeed)) * dt;
	m_camPosition += (-m_camRight * (direction.m128_f32[0] * m_movespeed)) * dt;
	m_camPosition += (XMVECTOR(0.0f, 1.0f, 0.0f) * (direction.m128_f32[1] * m_movespeed)) * dt;
}


XMMATRIX Camera::GetView()
{
	return m_view;
}

void Camera::SetProjection(XMMATRIX projectionMatrix)
{
	m_projectionMatrix = projectionMatrix;
}

XMMATRIX Camera::GetProjectionMatrix()
{
	return m_projectionMatrix;
}