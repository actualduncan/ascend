#pragma once
#include <iostream>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12/d3dx12.h> // include d3d
#include <wrl.h>
#include <DirectXMath.h>
#include "DirectXRaytracingHelper.h"
//#include "WindowsApplication.h"

using namespace Microsoft::WRL;
using namespace DirectX;

namespace RendererPrivate
{
	constexpr uint8_t MAX_FRAMES = 3;

	struct Viewport
	{
		float left;
		float top;
		float right;
		float bottom;
	};

	struct RayGenConstantBuffer
	{
		Viewport viewport;
		Viewport stencil;
	};
}

struct FrameContext
{
	ComPtr<ID3D12CommandAllocator> CommandAllocator;
	UINT64                         FenceValue;
};

class RenderDevice
{
public:
	RenderDevice();
	~RenderDevice();
	bool Initialize(HWND hwnd);
	void WaitForGPU();
	void MoveToNextFrame();
	void PopulateCommandLists();
	void OnRender();
	void BuildGeometry();
private:

	
	FrameContext m_frameContext[RendererPrivate::MAX_FRAMES];

	// Raytracing
	void CreateRaytracingInterfaces();
	void CreateRootSignatures();
	void CreateRaytracingPSO();
	void CreateDescriptorHeap();
	void BuildAccelerationStructures();
	void BuildShaderTables();
	void CreateRaytracingOutputResource();

	ComPtr<ID3D12Device5> m_dxrDevice;
	ComPtr<ID3D12GraphicsCommandList4> m_dxrCommandList;
	ComPtr<ID3D12RootSignature> m_raytracingRootSignature;
	RendererPrivate::RayGenConstantBuffer m_rayGenCB;
	// others
	ComPtr<ID3D12Device> m_device;
	ComPtr<IDXGIAdapter4> m_adapter;

	ComPtr<IDXGISwapChain4> m_swapChain;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	ComPtr<ID3D12Resource> m_renderTargets[RendererPrivate::MAX_FRAMES];
	D3D12_CPU_DESCRIPTOR_HANDLE  m_renderTargetDescriptors[RendererPrivate::MAX_FRAMES] = {};
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12CommandAllocator> m_commandAllocators[RendererPrivate::MAX_FRAMES];
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	ComPtr<ID3D12Fence> m_fence;
	ComPtr<ID3D12Resource> m_vertexBuffer;
	

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	UINT m_rtvDescriptorSize;
	UINT m_srvDescriptorSize;
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	HANDLE m_SwapChainWaitableObject;
	UINT m_frameIndex;
	UINT64 m_fenceValues[RendererPrivate::MAX_FRAMES];
	HANDLE m_fenceEvent;
};

