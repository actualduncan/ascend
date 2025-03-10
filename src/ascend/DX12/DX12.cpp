#include "DX12.h"
#include "DX12_Helpers.h"
#include "GraphicsTypes.h"
#include "PCH.h"

extern "C" {
	__declspec(dllexport) extern const unsigned int D3D12SDKVersion = 613;
}
extern "C" {
	__declspec(dllexport) extern const char* D3D12SDKPath = ".\\";
}

namespace DX12
{

	D3D_FEATURE_LEVEL MaxSupportedFeatureLevel = D3D_FEATURE_LEVEL_12_0;
	// initialize based on d3d12 version
	ComPtr<IDXGIFactory4> Factory = nullptr;
	ComPtr<IDXGIAdapter1> Adapter = nullptr;
	ComPtr<ID3D12Device9> Device = nullptr;
	ComPtr<ID3D12CommandList> CmdList = nullptr;
	ComPtr<ID3D12CommandQueue> GraphicsQueue = nullptr;
	ComPtr<ID3D12GraphicsCommandList10> GraphicsCmdList = nullptr;

	UINT64 CurrentCPUFrame;
	UINT64 CurrentGPUFrame;
	UINT64 CurrentFrameIdx;

	static const uint64_t NumCmdAllocators = RenderLatency;
	static ComPtr<ID3D12CommandAllocator> GraphicsCmdAllocators[NumCmdAllocators] = { };

	static Fence FrameFence;

	UINT RTVDescriptorSize = 0;
	UINT UAVDescriptorSize = 0;
	UINT DSVDescriptorSize = 0;

	DescriptorHeap RTVDescriptorHeap;
	DescriptorHeap UAVDescriptorHeap;
	DescriptorHeap DSVDescriptorHeap;
	
	Profiler GPUProfiler;

	void Initialize(D3D_FEATURE_LEVEL minFeatureLevel)
	{
		// check feature levels

		VERIFYD3D12RESULT(CreateDXGIFactory1(IID_PPV_ARGS(&Factory)));

		VERIFYD3D12RESULT(Factory->EnumAdapters1(0, &Adapter));

		DXGI_ADAPTER_DESC1 desc = {};
		Adapter.Get()->GetDesc1(&desc);

		
#if DEBUG
		ComPtr<ID3D12Debug1> debugController;

		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
#endif //DEBUG

		VERIFYD3D12RESULT(D3D12CreateDevice(Adapter.Get(), minFeatureLevel, IID_PPV_ARGS(&Device)));


		CD3DX12FeatureSupport features;
		features.Init(Device.Get());

		MaxSupportedFeatureLevel = features.MaxSupportedFeatureLevel();

		// Check D3D12 Feature Level support
		if (MaxSupportedFeatureLevel < minFeatureLevel)
		{
			throw std::exception("Feature level Not supported");
		}

		// Check Shader Model Support
		if (features.HighestShaderModel() < D3D_SHADER_MODEL_6_6)
		{
			throw std::exception("The device does not support the minimum shader model required to run (Shader Model 6.6)");
		}

		// Check DXR Support
		if (features.RaytracingTier() < D3D12_RAYTRACING_TIER_1_1)
		{
			throw std::exception("The device does not support DXR 1.1, which is required to run.");
		}

		// Initialize Command Lists and Allocators
		for (int i = 0; i < NumCmdAllocators; ++i)
		{
			VERIFYD3D12RESULT(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&GraphicsCmdAllocators[i])));
			GraphicsCmdAllocators[i]->SetName(L"Graphics Command Allocator");
		}

		VERIFYD3D12RESULT(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, GraphicsCmdAllocators[0].Get(), nullptr, IID_PPV_ARGS(&GraphicsCmdList)));
		VERIFYD3D12RESULT(GraphicsCmdList->Close());
		GraphicsCmdList->SetName(L"Main Graphics Command List");

		// Initialize Graphics & Compute Queues
		D3D12_COMMAND_QUEUE_DESC gQueueDesc = {};
		gQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		gQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
		VERIFYD3D12RESULT(Device->CreateCommandQueue(&gQueueDesc, IID_PPV_ARGS(&GraphicsQueue)));
		GraphicsQueue.Get()->SetName(L"Main Graphics Queue");

		CurrentFrameIdx = CurrentCPUFrame % NumCmdAllocators;

		VERIFYD3D12RESULT(GraphicsCmdAllocators[CurrentFrameIdx]->Reset());
		VERIFYD3D12RESULT(GraphicsCmdList->Reset(GraphicsCmdAllocators[CurrentFrameIdx].Get(), nullptr));


		FrameFence.Init(0);

		// Create Descriptors
		RTVDescriptorHeap.Init(10, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
		UAVDescriptorHeap.Init(60, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
		DSVDescriptorHeap.Init(RenderLatency, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, false);

		RTVDescriptorSize = RTVDescriptorHeap.DescriptorSize;

		GPUProfiler.Init(desc);
	}

	void StartFrame()
	{
		GraphicsCmdList->SetDescriptorHeaps(1, UAVDescriptorHeap.Heap.GetAddressOf());
	}

	void EndFrame(IDXGISwapChain4* swapChain)
	{
		
		// Close cmd list now that 
		VERIFYD3D12RESULT(GraphicsCmdList->Close());

		ID3D12CommandList* graphicsCommandLists[] = { 
			GraphicsCmdList.Get()
		};

		GraphicsQueue->ExecuteCommandLists(ARRAYSIZE(graphicsCommandLists), graphicsCommandLists);


		if (swapChain)
		{
			VERIFYD3D12RESULT(swapChain->Present(1, 0));
		}

		++CurrentCPUFrame;

		//fence here
		FrameFence.Signal(GraphicsQueue.Get(), CurrentCPUFrame);
		//change frame index

		// Wait for the GPU to catch up before we stomp an executing command buffer
		const uint64_t gpuLag = DX12::CurrentCPUFrame - DX12::CurrentGPUFrame;
	
		if (gpuLag >= DX12::RenderLatency)
		{
			// Make sure that the previous frame is finished
			FrameFence.Wait(DX12::CurrentGPUFrame + 1);
			++DX12::CurrentGPUFrame;
		}

		//reset command allocators and command lists for next frame.
		CurrentFrameIdx = DX12::CurrentCPUFrame % NumCmdAllocators;


		VERIFYD3D12RESULT(GraphicsCmdAllocators[CurrentFrameIdx]->Reset());
		VERIFYD3D12RESULT(GraphicsCmdList->Reset(GraphicsCmdAllocators[CurrentFrameIdx].Get(), nullptr));
	}

	void WaitForGPU()
	{
		if (CurrentCPUFrame > CurrentGPUFrame)
		{
			FrameFence.Wait(CurrentCPUFrame);
			CurrentGPUFrame = CurrentCPUFrame;
		}
	}

	// maybe move to another file
	D3D12_RESOURCE_BARRIER MakeTransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t subResource)
	{
		D3D12_RESOURCE_BARRIER barrier = { };
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = resource;
		barrier.Transition.StateBefore = before;
		barrier.Transition.StateAfter = after;
		barrier.Transition.Subresource = subResource;
		return barrier;
	}

	void TransitionResource(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after, uint32_t subResource)
	{
		D3D12_RESOURCE_BARRIER barrier = MakeTransitionBarrier(resource, before, after, subResource);
		cmdList->ResourceBarrier(1, &barrier);
	}

	void Shutdown()
	{

	}
}