#include "Camera.h"
#include <algorithm>
#include <iostream>

using namespace DirectX;

float clamp(float n, float lower, float upper)
{
	return n <= lower ? lower : n >= upper ? upper : n;
}

Camera::Camera(HWND window, float aspectRatio, float minZ, float maxZ)
	: m_rotRate(0.1f)
	, m_moveSpeed(3.0f)
	, m_window(window)
	, m_cursor()
{

	m_position = XMFLOAT3(0.0f,0.0f, 1.0f);
	m_rotation = XMFLOAT3(0,0,0);
	m_up = XMFLOAT3(0, 0, 0);
	m_right = XMFLOAT3(0, 0, 0);
	m_forward = XMFLOAT3(0, 0, 0);
	m_projectionMatrix = XMMatrixIdentity();

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

void Camera::Update(float dt, InputCommands input)
{
	GetClientRect(m_window, &m_viewportRect);

	m_cursor.x = input.mouseX;
	m_cursor.y = input.mouseY;


 	if (input.mouseRButtonDown&& isMouseActive)
	{
		int deltax = m_cursor.x - (m_viewportRect.right / 2);
		int deltay = m_cursor.y - (m_viewportRect.bottom / 2);
		
		Rotate(deltax, deltay, dt);
		m_cursor.x = (m_viewportRect.right / 2);
		m_cursor.y = (m_viewportRect.bottom / 2);

		SetCursorPos(m_cursor.x, m_cursor.y);
	}
	
	if (input.mouseRButtonDown && !isMouseActive)
	{
		m_cursor.x = (m_viewportRect.right / 2);
		m_cursor.y = (m_viewportRect.bottom / 2);

		SetCursorPos(m_cursor.x, m_cursor.y);
		ShowCursor(false);
		isMouseActive = true;
	}

	if (!input.mouseRButtonDown && isMouseActive)
	{
		ShowCursor(true);
		isMouseActive = false;
	}

	Move(XMFLOAT3(input.horizontalX, input.vertical, input.horizontalZ), dt);

	CalculateView();
}

void Camera::CalculateView()
{
	XMVECTOR up, right, positionv, lookAt;
	float yaw, pitch, roll;
	XMMATRIX rotationMatrix;

	// Setup the vectors
	up = XMVectorSet(0.0f, 1.0, 0.0, 0.0);
	positionv = XMLoadFloat3(&m_position);
	lookAt = XMVectorSet(0.0, 0.0, 1.0f, 0.0);

	// Set the yaw (Y axis), pitch (X axis), and roll (Z axis) rotations in radians.
	pitch = m_rotation.x * 0.0174532f;
	yaw = m_rotation.y * 0.0174532f;
	roll = m_rotation.z * 0.0174532f;

	// Create the m_rotation matrix from the yaw, pitch, and roll values.
	rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

	// Transform the lookAt and up vector by the m_rotation matrix so the view is correctly rotated at the origin.
	lookAt = XMVector3TransformCoord(lookAt, rotationMatrix);
	up = XMVector3TransformCoord(up, rotationMatrix);
	right = XMVector3TransformCoord(XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f), rotationMatrix);

	XMStoreFloat3(&m_forward, XMVector3Cross(up, right));
	XMStoreFloat3(&m_up, up);
	XMStoreFloat3(&m_right, right);

	// Translate the rotated camera m_position to the location of the viewer.
	lookAt = positionv + lookAt;


	// Finally create the view matrix from the three updated vectors.
	m_view = XMMatrixLookAtLH(positionv, lookAt, up);
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

XMFLOAT3 Camera::GetPosition()
{
	return m_position;
}

void Camera::Rotate(int deltaX, int deltaY, float dt)
{
	m_rotation.x -= deltaY / m_rotRate * dt;
	m_rotation.y -= deltaX / m_rotRate * dt;
	m_rotation.z = 0;

	// clamp x rotation by 90 degrees
	m_rotation.x = clamp(m_rotation.x, -90.0f, 90.0f);
}

void Camera::Move(XMFLOAT3 direction, float dt)
{
	XMVECTOR directionv = XMVector3Normalize(XMLoadFloat3(&direction));
	XMVECTOR positionv = XMLoadFloat3(&m_position);

	positionv += (XMLoadFloat3(&m_forward) * (XMVectorGetZ(directionv) * m_moveSpeed)) * dt;
	positionv += (XMLoadFloat3(&m_right) * (XMVectorGetX(directionv) * m_moveSpeed)) * dt;
	positionv += (XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f) * (XMVectorGetY(directionv) * m_moveSpeed)) * dt;

	XMStoreFloat3(&m_position, positionv);
}