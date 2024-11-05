#pragma once
#include <iostream>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12/d3dx12.h> // include d3d
#include <wrl.h>
#include <DirectXMath.h>
#include "Shader/RaytracingHlslCompat.h"
//#include "WindowsApplication.h"

using namespace Microsoft::WRL;
using namespace DirectX;

namespace RendererPrivate
{
	constexpr uint8_t MAX_FRAMES = 3;

}
namespace GlobalRootSignatureParams {
	enum Value {
		OutputViewSlot = 0,
		AccelerationStructureSlot,
		Count
	};
}

namespace LocalRootSignatureParams {
	enum Value {
		ViewportConstantSlot = 0,
		Count
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

	// Viewport dimensions.
	UINT m_width = 1280;
	UINT m_height = 800;
	float m_aspectRatio;

	// Raytracing
	void CreateRaytracingInterfaces();
	void CreateRootSignatures();
	void CreateRaytracingPSO();
	void CreateDescriptorHeap();
	void BuildAccelerationStructures();
	void BuildShaderTables();
	void CreateRaytracingOutputResource();
	void CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline);
	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();
	void DoRaytracing();
	void CopyRaytracingOutputToBackbuffer();
	UINT AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, UINT descriptorIndexToUse);
	ComPtr<ID3D12Device5> m_dxrDevice;
	ComPtr<ID3D12GraphicsCommandList4> m_dxrCommandList;
	ComPtr<ID3D12StateObject> m_dxrStateObject;

	// Root signatures
	ComPtr<ID3D12RootSignature> m_raytracingGlobalRootSignature;
	ComPtr<ID3D12RootSignature> m_raytracingLocalRootSignature;
	RayGenConstantBuffer m_rayGenCB;

	// descriptors
	ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
	UINT m_descriptorsAllocated = 0;
	UINT m_descriptorSize;

	typedef UINT16 Index;
	struct Vertex { float v1, v2, v3; };
	ComPtr<ID3D12Resource> m_indexBuffer;
	ComPtr<ID3D12Resource> m_vertexBuffer;

	// Acceleration structure
	ComPtr<ID3D12Resource> m_accelerationStructure;
	ComPtr<ID3D12Resource> m_bottomLevelAccelerationStructure;
	ComPtr<ID3D12Resource> m_topLevelAccelerationStructure;

	// Shader table
	static const wchar_t* c_hitGroupName;
	static const wchar_t* c_raygenShaderName;
	static const wchar_t* c_closestHitShaderName;
	static const wchar_t* c_missShaderName;
	ComPtr<ID3D12Resource> m_missShaderTable;
	ComPtr<ID3D12Resource> m_hitGroupShaderTable;
	ComPtr<ID3D12Resource> m_rayGenShaderTable;

	// Raytracing output
	ComPtr<ID3D12Resource> m_raytracingOutput;
	D3D12_GPU_DESCRIPTOR_HANDLE m_raytracingOutputResourceUAVGpuDescriptor;
	UINT m_raytracingOutputResourceUAVDescriptorHeapIndex = UINT_MAX;

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
	//ComPtr<ID3D12Resource> m_vertexBuffer;
	

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	UINT m_rtvDescriptorSize;
	UINT m_srvDescriptorSize;
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	HANDLE m_SwapChainWaitableObject;
	UINT m_frameIndex = 0;
	UINT64 m_fenceValues[RendererPrivate::MAX_FRAMES];
	HANDLE m_fenceEvent;
};

