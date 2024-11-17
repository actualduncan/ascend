#include <iostream>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12/d3dx12.h> // include d3d
#include <wrl.h>
#include <DirectXMath.h>
#include "Shader/RaytracingHlslCompat.h"
using namespace Microsoft::WRL;
using namespace DirectX;

class RayTracingRHI
{
public:
	RayTracingRHI();
	~RayTracingRHI();
	
	void Initialize();
private:
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

};