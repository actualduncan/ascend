#include <d3dx12/d3dx12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>// include d3d
#include <wrl.h>
#include <DirectXMath.h>

using namespace Microsoft::WRL;
using namespace DirectX;

namespace DX12 
{

	const uint64_t RenderLatency = 2;

	extern ComPtr<IDXGIFactory4> Factory;
	extern ComPtr<IDXGIAdapter1> Adapter;
	extern ComPtr<ID3D12Device9> Device;
	extern ComPtr<ID3D12CommandQueue> GraphicsQueue;
	extern ComPtr<ID3D12CommandQueue> ComputeQueue;
	extern ComPtr<ID3D12GraphicsCommandList4> GraphicsCmdList;
	extern ComPtr<ID3D12GraphicsCommandList4> ComputeCmdList;

	extern UINT RTVDescriptorSize;
	extern UINT UAVDescriptorSize;

	extern ComPtr<ID3D12DescriptorHeap> RTVDescriptorHeap;
	extern ComPtr<ID3D12DescriptorHeap> UAVDescriptorHeap;
	// for syncro
	extern UINT64 CurrentCPUFrame;  
	extern UINT64 CurrentGPUFrame;  
	extern UINT64 CurrFrameIdx;     


	void Initialize(D3D_FEATURE_LEVEL featureLevel);

	void StartFrame();

	void EndFrame();

	D3D12_RESOURCE_BARRIER MakeTransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t subResource);

	void TransitionResource(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t subResource);
}