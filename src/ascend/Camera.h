#pragma once

#include "DX12/PCH.h"
#include "InputCommands.h"
using namespace DirectX;

class Camera
{
public:
	Camera(HWND window, float aspectRatio, float minZ = 0.01f, float maxZ = 1000.0f);
	~Camera();

	void SetProjection(XMMATRIX projectionMatrix);
	
	void WindowSizeChanged(float aspectRatio, float minZ = 0.01f, float maxZ = 1000.0f);
	void Update(float deltaTime, InputCommands input);

	void Rotate(int deltaX, int deltaY, float dt);
	void Move(XMFLOAT3 direction, float dt);

	XMMATRIX GetView();
	XMMATRIX GetProjectionMatrix();
	XMFLOAT3 GetPosition();

private:

	void CalculateView();

	XMFLOAT3 m_position;
	XMFLOAT3 m_rotation;

	XMFLOAT3 m_up;
	XMFLOAT3 m_right;
	XMFLOAT3 m_forward;

	XMMATRIX m_view;
	XMMATRIX m_projectionMatrix;

	RECT m_viewportRect;

	float m_rotRate;
	float m_moveSpeed;

	POINT m_cursor;
	HWND m_window;

	bool isMouseActive = false;
};

