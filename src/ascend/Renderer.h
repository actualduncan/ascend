#pragma once
#include <iostream>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12/d3dx12.h> // include d3d
#include <wrl.h>
#include <DirectXMath.h>
#include "WindowsApplication.h"

using namespace Microsoft::WRL;
using namespace DirectX;

namespace RendererPrivate
{
	constexpr uint8_t MAX_FRAMES = 3;

}


class Renderer
{
public:
	Renderer();
	~Renderer();
	bool Initialize();
	void InitPipeline();
	void LoadAssets();
	void WaitForGPU();
	void GetDevice();
	void Update();
	void PopulateCommandLists();
	void Shutdown();
private:
	ComPtr<ID3D12Device> m_device;
	ComPtr<IDXGIAdapter4> m_adapter;

	ComPtr<IDXGISwapChain4> m_swapChain;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	ComPtr<ID3D12Resource2> m_renderTargets[RendererPrivate::MAX_FRAMES];
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	ComPtr<ID3D12Fence> m_fence;
	ComPtr<ID3D12Resource> m_vertexBuffer;
	
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	UINT m_rtvDescriptorSize;
	UINT m_srvDescriptorSize;
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	UINT m_frameIndex;
	UINT64 m_fenceValues[RendererPrivate::MAX_FRAMES];
	HANDLE m_fenceEvent;
};

