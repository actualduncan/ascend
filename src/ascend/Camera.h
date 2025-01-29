#pragma once

#include "DX12/PCH.h"



class Camera
{
public:
	Camera(HWND window, float aspectRatio, float minZ = 0.01f, float maxZ = 1000.0f);
	~Camera();

	void SetProjection(XMMATRIX projectionMatrix);
	void WindowSizeChanged(float aspectRatio, float minZ = 0.01f, float maxZ = 1000.0f);
	void Update(float deltaTime);

	void Rotate(int deltaX, int deltaY, float dt);
	void Move(XMVECTOR direction, float dt);

	XMMATRIX GetView();
	XMMATRIX GetProjectionMatrix();

	bool intersected = false;
private:

	void CalculateView();

	//camera
	XMVECTOR		m_camPosition;
	XMFLOAT3		m_camRotation	;
	XMVECTOR		m_camLookAt;
	XMVECTOR		m_camUp;
	XMVECTOR		m_camRight;
	XMVECTOR		m_forward;
	XMMATRIX        m_rotationMatrix;
	XMMATRIX	    m_view;

	XMMATRIX        m_projectionMatrix;

	RECT m_viewportRect;

	float m_camRotRate;
	float m_movespeed;

	POINT m_cursor;
	HWND m_window;

	bool isMouseActive = false;
};

