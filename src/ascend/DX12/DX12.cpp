#include "DX12.h"
#include "DX12_Helpers.h"
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
	ComPtr<ID3D12CommandQueue> ComputeQueue = nullptr;
	ComPtr<ID3D12GraphicsCommandList4> GraphicsCmdList = nullptr;
	ComPtr<ID3D12GraphicsCommandList4> ComputeCmdList = nullptr;

	UINT64 CurrentCPUFrame;
	UINT64 CurrentGPUFrame;
	UINT64 CurrentFrameIdx;

	static const uint64_t NumCmdAllocators = RenderLatency;
	static ComPtr<ID3D12CommandAllocator> GraphicsCmdAllocators[NumCmdAllocators] = { };
	static ComPtr<ID3D12CommandAllocator> ComputeCmdAllocators[NumCmdAllocators] = { };

	ComPtr<ID3D12Fence> FrameFence = nullptr;

	UINT RTVDescriptorSize = 0;
	UINT UAVDescriptorSize = 0;

	ComPtr<ID3D12DescriptorHeap> RTVDescriptorHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> UAVDescriptorHeap = nullptr;


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

		// Check Feature Level Support
		D3D12_FEATURE_DATA_FEATURE_LEVELS featureLevelData = {};
		featureLevelData.pFeatureLevelsRequested = &minFeatureLevel;
		featureLevelData.NumFeatureLevels = 1;
		Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureLevelData, 1);
		MaxSupportedFeatureLevel = featureLevelData.MaxSupportedFeatureLevel;
		if (MaxSupportedFeatureLevel < minFeatureLevel)
		{
			throw std::exception("Feature level Not supported");
		}

		// Check Shader Model Support
		D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_6 };
		VERIFYD3D12RESULT(Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)));
		if (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_6)
		{
			throw std::exception("The device does not support the minimum shader model required to run (Shader Model 6.6)");
		}

		// Check DXR Support
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 opts5 = { };
		VERIFYD3D12RESULT(Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &opts5, sizeof(opts5)));
		if (opts5.RaytracingTier < D3D12_RAYTRACING_TIER_1_1)
		{
			throw std::exception("The device does not support DXR 1.1, which is required to run.");
		}


		// Initialize Command Lists and Allocators
		for (int i = 0; i < NumCmdAllocators; ++i)
		{
			VERIFYD3D12RESULT(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&GraphicsCmdAllocators[i])));
			VERIFYD3D12RESULT(Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&ComputeCmdAllocators[i])));
		}

		VERIFYD3D12RESULT(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, GraphicsCmdAllocators[0].Get(), nullptr, IID_PPV_ARGS(&GraphicsCmdList)));
		VERIFYD3D12RESULT(GraphicsCmdList->Close());
		GraphicsCmdList->SetName(L"Main Graphics Command List");

		VERIFYD3D12RESULT(Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, ComputeCmdAllocators[0].Get(), nullptr, IID_PPV_ARGS(&ComputeCmdList)));
		VERIFYD3D12RESULT(ComputeCmdList->Close());
		ComputeCmdList->SetName(L"Main Compute Command List");

		// Initialize Graphics & Compute Queues
		D3D12_COMMAND_QUEUE_DESC gQueueDesc = {};
		gQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		gQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		VERIFYD3D12RESULT(Device->CreateCommandQueue(&gQueueDesc, IID_PPV_ARGS(&GraphicsQueue)));
		GraphicsQueue.Get()->SetName(L"Main Graphics Queue");

		D3D12_COMMAND_QUEUE_DESC cQueueDesc = {};
		cQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		cQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		VERIFYD3D12RESULT(Device->CreateCommandQueue(&cQueueDesc, IID_PPV_ARGS(&ComputeQueue)));
		ComputeQueue.Get()->SetName(L"Main Compute Queue");

		CurrentFrameIdx = CurrentCPUFrame % NumCmdAllocators;

		VERIFYD3D12RESULT(GraphicsCmdAllocators[CurrentFrameIdx]->Reset());
		VERIFYD3D12RESULT(ComputeCmdAllocators[CurrentFrameIdx]->Reset());
		VERIFYD3D12RESULT(GraphicsCmdList->Reset(GraphicsCmdAllocators[CurrentFrameIdx].Get(), nullptr));
		VERIFYD3D12RESULT(ComputeCmdList->Reset(GraphicsCmdAllocators[CurrentFrameIdx].Get(), nullptr));


		// Create Descriptors
		D3D12_DESCRIPTOR_HEAP_DESC RTVDescriptorHeapDesc = { };
		RTVDescriptorHeapDesc.NumDescriptors = RenderLatency;
		RTVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		
		VERIFYD3D12RESULT(Device->CreateDescriptorHeap(&RTVDescriptorHeapDesc, IID_PPV_ARGS(&RTVDescriptorHeap)));
		RTVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		D3D12_DESCRIPTOR_HEAP_DESC UAVDescriptorHeapDesc = { };
		RTVDescriptorHeapDesc.NumDescriptors = RenderLatency;
		RTVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

		VERIFYD3D12RESULT(Device->CreateDescriptorHeap(&RTVDescriptorHeapDesc, IID_PPV_ARGS(&UAVDescriptorHeap)));
		UAVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		/*

		CreateFrameResources();

		CreateWorkGraphRootSignature();
		CreateWorkGraph();
		WaitForGPU();
		CreateRaytracingInterfaces();
		BuildGeometry();
		BuildAccelerationStructuresForCompute();
		*/
		
	}

	void StartFrame()
	{

	}

	void EndFrame()
	{

	}
}