#include "Renderer.h"
#include <d3dcompiler.h>
#include "AscendHelpers.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"
#include "Shader/CompiledShaders/Raytracing.hlsl.h"
#include "Shader/CompiledShaders/ComputeRaytracing.hlsl.h"
#include "Shader/CompiledShaders/WorkGraphRaytracing.hlsl.h"

/*
												WARNING
	This whole file needs refactoring, this is currently a spike implemnetation of a D3D12 Renderer
									The code is therefore very messy.
		Most of this code is adapted from https://github.com/microsoft/DirectX-Graphics-Samples
 */

extern "C" {
	__declspec(dllexport) extern const unsigned int D3D12SDKVersion = 613;
}
extern "C" {
	__declspec(dllexport) extern const char* D3D12SDKPath = ".\\";
}

const wchar_t* RenderDevice::c_hitGroupName = L"MyHitGroup";
const wchar_t* RenderDevice::c_raygenShaderName = L"MyRaygenShader";
const wchar_t* RenderDevice::c_closestHitShaderName = L"MyClosestHitShader";
const wchar_t* RenderDevice::c_missShaderName = L"MyMissShader";

RenderDevice::RenderDevice()
{

#pragma region RAY_TRACING
	m_rayGenCB.viewport = { -1.0f, -1.0f, 1.0f, 1.0f };
	float border = 0.1f;
	if (m_width <= m_height)
	{
		m_rayGenCB.stencil =
		{
			-1 + border, -1 + border * m_aspectRatio,
			1.0f - border, 1 - border * m_aspectRatio
		};
	}
	else
	{
		m_rayGenCB.stencil =
		{
			-1 + border / m_aspectRatio, -1 + border,
			 1 - border / m_aspectRatio, 1.0f - border
		};

	}
#pragma endregion

}

RenderDevice::~RenderDevice()
{	
	//Shutdown();
}

bool RenderDevice::Initialize(HWND hwnd)
{
	m_hwnd = hwnd;
	bool hasInitialized = true;

	InitializeDebugDevice();

	CreateCommandQueues();

	CreateDescriptorHeaps();

	CreateFrameResources();

	CreateWorkGraphRootSignature();
	CreateWorkGraph();
	WaitForGPU();
	CreateRaytracingInterfaces();
	BuildGeometry();
	BuildAccelerationStructuresForCompute();

#pragma region COMPUTE
	//LoadComputeAssets();

	//WaitForGPU();
	//CreateRaytracingInterfaces();

	//BuildGeometry();
	//BuildAccelerationStructuresForCompute();
#pragma endregion
#pragma region RAY_TRACING
	//CreateDeviceDependentResources();
	//CreateWindowSizeDependentResources();
#pragma endregion

	return true;
}

void RenderDevice::InitializeDebugDevice()
{
	DWORD dxgiFactoryFlags = 0;

#if DEBUG
	ComPtr<ID3D12Debug1> m_debugController;
	//add debug flags
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&m_debugController))))
	{
		m_debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif //DEBUG

	VERIFYD3D12RESULT(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));

	for (uint32_t id = 0; DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapterByGpuPreference(id, DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&m_adapter)); ++id)
	{
		// Contains description of the graphics hardware adapter (GPU)
		DXGI_ADAPTER_DESC3 desc;
		m_adapter->GetDesc3(&desc);

		// Device that supports D3D12 is found
		if (SUCCEEDED(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&m_device))))
		{
			break;
		}
	}

#if DEBUG
	
	ComPtr<ID3D12InfoQueue1> infoQueue;
	if (SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
	{
		ID3D12InfoQueue* pInfoQueue = nullptr;
		m_device->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
		pInfoQueue->Release();
		m_debugController->Release();
	}
	
#endif //DEBUG

}

void RenderDevice::CreateCommandQueues()
{
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		VERIFYD3D12RESULT(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue)));
	}

	// Compute
	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		VERIFYD3D12RESULT(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_computeCommandQueue)));
	}
}

void RenderDevice::CreateDescriptorHeaps()
{
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = RendererPrivate::MAX_FRAMES;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

		VERIFYD3D12RESULT(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Describe and Create SRV/UAV descriptor heap (used for imgui, can disable to test for compute shader)
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		VERIFYD3D12RESULT(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srvHeap)));
	}
}

void RenderDevice::CreateFrameResources()
{
	{
		// create a command allocator for each frame
		for (UINT n = 0; n < RendererPrivate::MAX_FRAMES; n++)
		{
			VERIFYD3D12RESULT(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n])));
			VERIFYD3D12RESULT(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&m_computeCommandAllocators[n])));
		}
	}

	// Release resources that are tied to the swap chain and update fence values.
	for (UINT n = 0; n < RendererPrivate::MAX_FRAMES; n++)
	{
		m_renderTargets[n].Reset();
		m_fenceValues[n] = m_fenceValues[m_frameIndex];
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	{
		ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
		swapChainDesc.BufferCount = RendererPrivate::MAX_FRAMES;
		swapChainDesc.Width = m_width;
		swapChainDesc.Height = m_height;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc.Stereo = FALSE;
	}

	{
		ComPtr<IDXGISwapChain1> swapChain1;

		VERIFYD3D12RESULT(m_factory->CreateSwapChainForHwnd(m_commandQueue.Get(), m_hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));	// use CreateSwapChainForCoreWindow for Windows Store apps
		VERIFYD3D12RESULT(swapChain1.As(&m_swapChain)); 
	}

	for (UINT n = 0; n < RendererPrivate::MAX_FRAMES; ++n)
	{
		VERIFYD3D12RESULT(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));

		wchar_t name[25] = {};
		swprintf_s(name, L"Render target %u", n);
		m_renderTargets[n]->SetName(name);

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), n, m_rtvDescriptorSize);
		m_device->CreateRenderTargetView(m_renderTargets[n].Get(), &rtvDesc, rtvDescriptor);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();


	//ImGui_ImplDX12_Init(m_device.Get(), RendererPrivate::MAX_FRAMES, DXGI_FORMAT_R8G8B8A8_UNORM,
		//m_srvHeap.Get(),
		//m_srvHeap->GetCPUDescriptorHandleForHeapStart(),
		//m_srvHeap->GetGPUDescriptorHandleForHeapStart());

	VERIFYD3D12RESULT(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_commandList)));

	// compute
#pragma region COMPUTE
	VERIFYD3D12RESULT(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_computeCommandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&m_computeCommandList)));
#pragma endregion
	m_commandList->Close();
	m_computeCommandList->Close();

	// syncro objects (fences)

	VERIFYD3D12RESULT(m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	VERIFYD3D12RESULT(m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_computeFence)));
	++m_fenceValues[m_frameIndex];

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
	{
		VERIFYD3D12RESULT(HRESULT_FROM_WIN32(GetLastError()));
	}
}


void RenderDevice::CreateWorkGraph()
{
	static const wchar_t* WorkGraphProgramName = L"WorkGraph";

	CD3DX12_STATE_OBJECT_DESC stateObjectDesc(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);

	auto rootSignatureSubobject = stateObjectDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	rootSignatureSubobject->SetRootSignature(m_workGraphRootSignature.Get());

	auto workGraphSubobject = stateObjectDesc.CreateSubobject<CD3DX12_WORK_GRAPH_SUBOBJECT>();
	workGraphSubobject->IncludeAllAvailableNodes();
	workGraphSubobject->SetProgramName(WorkGraphProgramName);

	auto rootNodeDispatchGrid = workGraphSubobject->CreateBroadcastingLaunchNodeOverrides(L"EntryFunction");
	rootNodeDispatchGrid->DispatchGrid(m_width, m_height, 1);

	// create shader library
	auto lib = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE((void*)g_pWorkGraphRaytracing, ARRAYSIZE(g_pWorkGraphRaytracing));
	lib->SetDXILLibrary(&libdxil);

	VERIFYD3D12RESULT(m_device->CreateStateObject(stateObjectDesc, IID_PPV_ARGS(&m_workGraphStateObject)));

	ComPtr<ID3D12StateObjectProperties1> stateObjectProperties;
	ComPtr<ID3D12WorkGraphProperties>    workGraphProperties;

	VERIFYD3D12RESULT(m_workGraphStateObject->QueryInterface(IID_PPV_ARGS(&stateObjectProperties)));
	VERIFYD3D12RESULT(m_workGraphStateObject->QueryInterface(IID_PPV_ARGS(&workGraphProperties)));

	// Get the index of our work graph inside the state object (state object can contain multiple work graphs)
	const auto workGraphIndex = workGraphProperties->GetWorkGraphIndex(WorkGraphProgramName);

	D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS memoryRequirements = {};
	workGraphProperties->GetWorkGraphMemoryRequirements(workGraphIndex, &memoryRequirements);

	// Work graphs can also request no backing memory (i.e., MaxSizeInBytes = 0)
	if (memoryRequirements.MaxSizeInBytes > 0) {
		CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
		CD3DX12_RESOURCE_DESC   resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(memoryRequirements.MaxSizeInBytes,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		VERIFYD3D12RESULT(m_device->CreateCommittedResource(&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			NULL,
			IID_PPV_ARGS(&m_workGraphBackingMemory)));
	}

	// Prepare work graph desc
    // See https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html#d3d12_set_program_desc
	m_workGraphProgramDesc.Type = D3D12_PROGRAM_TYPE_WORK_GRAPH;
	m_workGraphProgramDesc.WorkGraph.ProgramIdentifier = stateObjectProperties->GetProgramIdentifier(WorkGraphProgramName);
	// Set flag to initialize backing memory.
	// We'll clear this flag once we've run the work graph for the first time.
	m_workGraphProgramDesc.WorkGraph.Flags = D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE;
	// Set backing memory
	if (m_workGraphBackingMemory) {
		m_workGraphProgramDesc.WorkGraph.BackingMemory.StartAddress = m_workGraphBackingMemory->GetGPUVirtualAddress();
		m_workGraphProgramDesc.WorkGraph.BackingMemory.SizeInBytes = m_workGraphBackingMemory->GetDesc().Width;
	}

	// All tutorial work graphs must declare a node named "Entry" with an empty record (i.e., no input record).
	// The D3D12_DISPATCH_GRAPH_DESC uses entrypoint indices instead of string-based node IDs to reference the enty node.
	// GetEntrypointIndex allows us to translate from a node ID (i.e., node name and node array index)
	// to an entrypoint index.
	// See https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html#getentrypointindex
	m_workGraphEntryPointIndex = workGraphProperties->GetEntrypointIndex(workGraphIndex, { L"EntryFunction", 0 });

	// Check if entrypoint was found.
	if (m_workGraphEntryPointIndex == 0xFFFFFFFFU) {
		throw std::runtime_error("work graph does not contain an entry node with [NodeId(\"Entry\", 0)].");
	}

}

void RenderDevice::CreateWorkGraphRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE UAVDescriptor;
	UAVDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
	CD3DX12_ROOT_PARAMETER rootParameters[2];
	rootParameters[0].InitAsDescriptorTable(1, &UAVDescriptor);
	rootParameters[1].InitAsShaderResourceView(0);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc(ARRAYSIZE(rootParameters), rootParameters);

	VERIFYD3D12RESULT(D3DX12SerializeVersionedRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error));
	VERIFYD3D12RESULT(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_workGraphRootSignature)))

	{
		m_workGraphDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, m_width, m_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		VERIFYD3D12RESULT(m_device->CreateCommittedResource(
			&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&m_workGraphOutput)));


		D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 0, m_workGraphDescriptorSize);
		//m_raytracingOutputResourceUAVDescriptorHeapIndex = CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeapCpuBase, descriptorIndexToUse, m_computeDescriptorSize);
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		m_device->CreateUnorderedAccessView(m_workGraphOutput.Get(), nullptr, &UAVDesc, uavDescriptorHandle);
		m_workGraphOutputUavDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_srvHeap->GetGPUDescriptorHandleForHeapStart());
	}

}

void RenderDevice::DispatchWorkGraph()
{
	m_commandList->SetComputeRootSignature(m_workGraphRootSignature.Get());
	//D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	m_commandList->SetDescriptorHeaps(1, m_srvHeap.GetAddressOf());
	m_commandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, m_workGraphOutputUavDescriptorHandle);
	m_commandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, m_topLevelAccelerationStructure->GetGPUVirtualAddress());


	


	D3D12_DISPATCH_GRAPH_DESC dispatchDesc = {};
	dispatchDesc.Mode = D3D12_DISPATCH_MODE_NODE_CPU_INPUT;
	dispatchDesc.NodeCPUInput = {};
	dispatchDesc.NodeCPUInput.EntrypointIndex = m_workGraphEntryPointIndex;
	// Launch graph with one record
	dispatchDesc.NodeCPUInput.NumRecords = 1;
	// Record does not contain any data
	dispatchDesc.NodeCPUInput.RecordStrideInBytes = 0;
	dispatchDesc.NodeCPUInput.pRecords = nullptr;

	// Set program and dispatch the work graphs.
	// See
	// https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html#setprogram
	// https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html#dispatchgraph

	m_commandList->SetProgram(&m_workGraphProgramDesc);
	m_commandList->DispatchGraph(&dispatchDesc);

	// Clear backing memory initialization flag, as the graph has run at least once now
	// See https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html#d3d12_set_work_graph_flags
	m_workGraphProgramDesc.WorkGraph.Flags &= ~D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE;
}

void RenderDevice::CopyWorkGraphOutputToBackBuffer()
{
	D3D12_RESOURCE_BARRIER preCopyBarriers[2];
	preCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	preCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_workGraphOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_commandList->ResourceBarrier(ARRAYSIZE(preCopyBarriers), preCopyBarriers);

	m_commandList->CopyResource(m_renderTargets[m_frameIndex].Get(), m_workGraphOutput.Get());

	D3D12_RESOURCE_BARRIER postCopyBarriers[2];
	postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_workGraphOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	m_commandList->ResourceBarrier(ARRAYSIZE(postCopyBarriers), postCopyBarriers);
}

#pragma region COMPUTE
void RenderDevice::LoadComputeAssets()
{
	{
		CD3DX12_DESCRIPTOR_RANGE UAVDescriptor;
		UAVDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		CD3DX12_ROOT_PARAMETER rootParameters[2];
		rootParameters[0].InitAsDescriptorTable(1, &UAVDescriptor);
		rootParameters[1].InitAsShaderResourceView(0);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc(ARRAYSIZE(rootParameters), rootParameters);

		VERIFYD3D12RESULT(D3DX12SerializeVersionedRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error));
		VERIFYD3D12RESULT(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_computeRootSignature)))
	}
	// create pso
	{
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
		ComPtr<ID3DBlob> computeShader;
		ComPtr<ID3DBlob> error;
		//VERIFYD3D12RESULT(D3DCompileFromFile(L"C:/Users/Duncan/Desktop/_dev/ascend/ascend/build/src/Debug/Shader/ComputeRaytracing.hlsl", nullptr, nullptr, "CSMain", "cs_5_1", compileFlags, 0, &computeShader, &error));
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc {};
		desc.pRootSignature = m_computeRootSignature.Get();
		desc.CS = CD3DX12_SHADER_BYTECODE(CD3DX12_SHADER_BYTECODE((void*)g_pComputeRaytracing, ARRAYSIZE(g_pComputeRaytracing)));

		VERIFYD3D12RESULT(m_device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_computePipelineState)));
		NAME_D3D12_OBJECT(m_computePipelineState);
	}



	{
		m_computeDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, m_width, m_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		VERIFYD3D12RESULT(m_device->CreateCommittedResource(
			&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&m_computeOutput)));
	

		D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 0, m_computeDescriptorSize);
		//m_raytracingOutputResourceUAVDescriptorHeapIndex = CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeapCpuBase, descriptorIndexToUse, m_computeDescriptorSize);
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		m_device->CreateUnorderedAccessView(m_computeOutput.Get(), nullptr, &UAVDesc, uavDescriptorHandle);
		m_computeOutputUavDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_srvHeap->GetGPUDescriptorHandleForHeapStart());

	}
}

void RenderDevice::DoCompute()
{
	m_computeCommandList->SetPipelineState(m_computePipelineState.Get());
	//D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	m_computeCommandList->SetDescriptorHeaps(1, m_srvHeap.GetAddressOf());
	m_computeCommandList->SetComputeRootSignature(m_computeRootSignature.Get());
	m_computeCommandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, m_computeOutputUavDescriptorHandle);
	m_computeCommandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, m_topLevelAccelerationStructure->GetGPUVirtualAddress());
	m_computeCommandList->Dispatch(m_width, m_height, 1);
	//m_commandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, m_topLevelAccelerationStructure->GetGPUVirtualAddress());
}

void RenderDevice::CopyComputeOutputToBackBuffer()
{
	D3D12_RESOURCE_BARRIER preCopyBarriers[2];
	preCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	preCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_computeOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_commandList->ResourceBarrier(ARRAYSIZE(preCopyBarriers), preCopyBarriers);

	m_commandList->CopyResource(m_renderTargets[m_frameIndex].Get(), m_computeOutput.Get());

	D3D12_RESOURCE_BARRIER postCopyBarriers[2];
	postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_computeOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	m_commandList->ResourceBarrier(ARRAYSIZE(postCopyBarriers), postCopyBarriers);

}

void RenderDevice::BuildAccelerationStructuresForCompute()
{
	// Reset the command list for the acceleration structure construction.
	m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr);

	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles.IndexBuffer = m_indexBuffer->GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexCount = static_cast<UINT>(m_indexBuffer->GetDesc().Width) / sizeof(Index);
	geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
	geometryDesc.Triangles.Transform3x4 = 0;
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.VertexCount = static_cast<UINT>(m_vertexBuffer->GetDesc().Width) / sizeof(Vertex);
	geometryDesc.Triangles.VertexBuffer.StartAddress = m_vertexBuffer->GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

	// Mark the geometry as opaque. 
	// PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
	// Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	// Get required sizes for an acceleration structure.
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = buildFlags;
	topLevelInputs.NumDescs = 1;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);


	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = topLevelInputs;
	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.pGeometryDescs = &geometryDesc;
	m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);


	ComPtr<ID3D12Resource> scratchResource;
	AllocateUAVBuffer(m_device.Get(), max(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes), &scratchResource, D3D12_RESOURCE_STATE_COMMON, L"ScratchResource");

	// Allocate resources for acceleration structures.
	// Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
	// Default heap is OK since the application doesn’t need CPU read/write access to them. 
	// The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
	// and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
	//  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
	//  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
	{
		D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

		AllocateUAVBuffer(m_device.Get(), bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_bottomLevelAccelerationStructure, initialResourceState, L"BottomLevelAccelerationStructure");
		AllocateUAVBuffer(m_device.Get(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_topLevelAccelerationStructure, initialResourceState, L"TopLevelAccelerationStructure");
	}

	// Create an instance desc for the bottom-level acceleration structure.
	ComPtr<ID3D12Resource> instanceDescs;
	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
	instanceDesc.InstanceMask = 1;
	instanceDesc.AccelerationStructure = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
	AllocateUploadBuffer(m_device.Get(), &instanceDesc, sizeof(instanceDesc), &instanceDescs, L"InstanceDescs");

	// Bottom Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	{
		bottomLevelBuildDesc.Inputs = bottomLevelInputs;
		bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
		bottomLevelBuildDesc.DestAccelerationStructureData = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
	}

	// Top Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	{
		topLevelInputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
		topLevelBuildDesc.Inputs = topLevelInputs;
		topLevelBuildDesc.DestAccelerationStructureData = m_topLevelAccelerationStructure->GetGPUVirtualAddress();
		topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
	}

	auto BuildAccelerationStructure = [&](auto* raytracingCommandList)
		{
			CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(m_bottomLevelAccelerationStructure.Get());
			raytracingCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
			m_commandList->ResourceBarrier(1, &barrier);
			raytracingCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
		};

	// Build acceleration structure.
	BuildAccelerationStructure(m_dxrCommandList.Get());

	// Kick off acceleration structure construction.
	VERIFYD3D12RESULT(m_commandList->Close());
	ID3D12CommandList* commandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);

	// Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
	WaitForGPU();
}

#pragma endregion

#pragma region RAY_TRACING
void RenderDevice::CreateDeviceDependentResources()
{
	// Initialize raytracing pipeline.

// Create raytracing interfaces: raytracing device and commandlist.
	CreateRaytracingInterfaces();

	// Create root signatures for the shaders.
	CreateRaytracingRootSignatures();

	// Create a raytracing pipeline state object which defines the binding of shaders, state and resources to be used during raytracing.
	CreateRaytracingPSO();

	// Create a heap for descriptors.
	CreateRaytracingDescriptorHeap();

	// Build geometry to be used in the sample.
	BuildGeometry();

	// Build raytracing acceleration structures from the generated geometry.
	BuildAccelerationStructures();

	// Build shader tables, which define shaders and their local root arguments.
	BuildShaderTables();


	// Create an output 2D texture to store the raytracing result to.
	CreateRaytracingOutputResource();
}

void RenderDevice::CreateWindowSizeDependentResources()
{
	CreateRaytracingOutputResource();

	// For simplicity, we will rebuild the shader tables.
	BuildShaderTables();
}

void RenderDevice::CreateRaytracingInterfaces()
{
	VERIFYD3D12RESULT(m_device->QueryInterface(IID_PPV_ARGS(&m_dxrDevice)));
	VERIFYD3D12RESULT(m_commandList->QueryInterface(IID_PPV_ARGS(&m_dxrCommandList)));
}

void RenderDevice::CreateRaytracingRootSignatures()
{
	{
		CD3DX12_DESCRIPTOR_RANGE UAVDescriptor;
		UAVDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		CD3DX12_ROOT_PARAMETER rootParameters[2];
		rootParameters[0].InitAsDescriptorTable(1, &UAVDescriptor);
		rootParameters[1].InitAsShaderResourceView(0);
		CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);

		ComPtr<ID3DBlob> blob;
		ComPtr<ID3DBlob> error;
		VERIFYD3D12RESULT(D3D12SerializeRootSignature(&globalRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error));
		VERIFYD3D12RESULT(m_device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&m_raytracingGlobalRootSignature)));
	}

	{
		CD3DX12_ROOT_PARAMETER rootParameters[1];
		rootParameters[0].InitAsConstants(SizeOfInUint32(m_rayGenCB), 0, 0);
		CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
		localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
		ComPtr<ID3DBlob> blob;
		ComPtr<ID3DBlob> error;
		VERIFYD3D12RESULT(D3D12SerializeRootSignature(&localRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error));
		VERIFYD3D12RESULT(m_device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&m_raytracingLocalRootSignature)));
	}

}

void RenderDevice::CreateRaytracingPSO()
{
	CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };


	// DXIL library
	// This contains the shaders and their entrypoints for the state object.
	// Since shaders are not considered a subobject, they need to be passed in via DXIL library subobjects.
	auto lib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing, ARRAYSIZE(g_pRaytracing));
	lib->SetDXILLibrary(&libdxil);
	// Define which shader exports to surface from the library.
	// If no shader exports are defined for a DXIL library subobject, all shaders will be surfaced.
	// In this sample, this could be omitted for convenience since the sample uses all shaders in the library. 
	{
		lib->DefineExport(c_raygenShaderName);
		lib->DefineExport(c_closestHitShaderName);
		lib->DefineExport(c_missShaderName);
	}

	// Triangle hit group
	// A hit group specifies closest hit, any hit and intersection shaders to be executed when a ray intersects the geometry's triangle/AABB.
	// In this sample, we only use triangle geometry with a closest hit shader, so others are not set.
	auto hitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	hitGroup->SetClosestHitShaderImport(c_closestHitShaderName);
	hitGroup->SetHitGroupExport(c_hitGroupName);
	hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

	// Shader config
	// Defines the maximum sizes in bytes for the ray payload and attribute structure.
	auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	UINT payloadSize = 4 * sizeof(float);   // float4 color
	UINT attributeSize = 2 * sizeof(float); // float2 barycentrics
	shaderConfig->Config(payloadSize, attributeSize);

	// Local root signature and shader association
	CreateLocalRootSignatureSubobjects(&raytracingPipeline);
	// This is a root signature that enables a shader to have unique arguments that come from shader tables.

	// Global root signature
	// This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
	auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	globalRootSignature->SetRootSignature(m_raytracingGlobalRootSignature.Get());

	// Pipeline config
	// Defines the maximum TraceRay() recursion depth.
	auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	// PERFOMANCE TIP: Set max recursion depth as low as needed 
	// as drivers may apply optimization strategies for low recursion depths. 
	UINT maxRecursionDepth = 1; // ~ primary rays only. 
	pipelineConfig->Config(maxRecursionDepth);

	VERIFYD3D12RESULT(m_dxrDevice->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&m_dxrStateObject)));
}

void RenderDevice::CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline)
{
	// Hit group and miss shaders in this sample are not using a local root signature and thus one is not associated with them.

	// Local root signature to be used in a ray gen shader.
	{
		auto localRootSignature = raytracingPipeline->CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
		localRootSignature->SetRootSignature(m_raytracingLocalRootSignature.Get());
		// Shader association
		auto rootSignatureAssociation = raytracingPipeline->CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
		rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
		rootSignatureAssociation->AddExport(c_raygenShaderName);
	}
}

void RenderDevice::CreateRaytracingDescriptorHeap()
{

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	// Allocate a heap for a single descriptor:
	// 1 - raytracing output texture UAV
	descriptorHeapDesc.NumDescriptors = 1;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NodeMask = 0;
	m_device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_descriptorHeap));
	NAME_D3D12_OBJECT(m_descriptorHeap);

	m_descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void RenderDevice::BuildAccelerationStructures()
{

	// Reset the command list for the acceleration structure construction.
	m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr);

	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles.IndexBuffer = m_indexBuffer->GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexCount = static_cast<UINT>(m_indexBuffer->GetDesc().Width) / sizeof(Index);
	geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
	geometryDesc.Triangles.Transform3x4 = 0;
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.VertexCount = static_cast<UINT>(m_vertexBuffer->GetDesc().Width) / sizeof(Vertex);
	geometryDesc.Triangles.VertexBuffer.StartAddress = m_vertexBuffer->GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);

	// Mark the geometry as opaque. 
	// PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
	// Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	// Get required sizes for an acceleration structure.
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
	topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	topLevelInputs.Flags = buildFlags;
	topLevelInputs.NumDescs = 1;
	topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
	m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);


	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = topLevelInputs;
	bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	bottomLevelInputs.pGeometryDescs = &geometryDesc;
	m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);


	ComPtr<ID3D12Resource> scratchResource;
	AllocateUAVBuffer(m_device.Get(), max(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes), &scratchResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"ScratchResource");

	// Allocate resources for acceleration structures.
	// Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
	// Default heap is OK since the application doesn’t need CPU read/write access to them. 
	// The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
	// and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
	//  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
	//  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
	{
		D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

		AllocateUAVBuffer(m_device.Get(), bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_bottomLevelAccelerationStructure, initialResourceState, L"BottomLevelAccelerationStructure");
		AllocateUAVBuffer(m_device.Get(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_topLevelAccelerationStructure, initialResourceState, L"TopLevelAccelerationStructure");
	}

	// Create an instance desc for the bottom-level acceleration structure.
	ComPtr<ID3D12Resource> instanceDescs;
	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
	instanceDesc.InstanceMask = 1;
	instanceDesc.AccelerationStructure = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
	AllocateUploadBuffer(m_device.Get(), &instanceDesc, sizeof(instanceDesc), &instanceDescs, L"InstanceDescs");

	// Bottom Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
	{
		bottomLevelBuildDesc.Inputs = bottomLevelInputs;
		bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
		bottomLevelBuildDesc.DestAccelerationStructureData = m_bottomLevelAccelerationStructure->GetGPUVirtualAddress();
	}

	// Top Level Acceleration Structure desc
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
	{
		topLevelInputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
		topLevelBuildDesc.Inputs = topLevelInputs;
		topLevelBuildDesc.DestAccelerationStructureData = m_topLevelAccelerationStructure->GetGPUVirtualAddress();
		topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
	}

	auto BuildAccelerationStructure = [&](auto* raytracingCommandList)
		{
			CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(m_bottomLevelAccelerationStructure.Get());
			raytracingCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
			m_commandList->ResourceBarrier(1, &barrier);
			raytracingCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
		};

	// Build acceleration structure.
	BuildAccelerationStructure(m_dxrCommandList.Get());

	// Kick off acceleration structure construction.
	VERIFYD3D12RESULT(m_commandList->Close());
	ID3D12CommandList* commandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(ARRAYSIZE(commandLists), commandLists);

	// Wait for GPU to finish as the locally created temporary GPU resources will get released once we go out of scope.
	WaitForGPU();
}

void RenderDevice::BuildShaderTables()
{
	void* rayGenShaderIdentifier;
	void* missShaderIdentifier;
	void* hitGroupShaderIdentifier;

	auto GetShaderIdentifiers = [&](auto* stateObjectProperties)
		{
			rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(c_raygenShaderName);
			missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(c_missShaderName);
			hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(c_hitGroupName);
		};

	// Get shader identifiers.
	UINT shaderIdentifierSize;
	{
		ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
		VERIFYD3D12RESULT(m_dxrStateObject.As(&stateObjectProperties));
		GetShaderIdentifiers(stateObjectProperties.Get());
		shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	}

	// Ray gen shader table
	{
		struct RootArguments {
			RayGenConstantBuffer cb;
		} rootArguments;
		rootArguments.cb = m_rayGenCB;

		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize + sizeof(rootArguments);
		ShaderTable rayGenShaderTable(m_device.Get(), numShaderRecords, shaderRecordSize, L"RayGenShaderTable");
		rayGenShaderTable.push_back(ShaderRecord(rayGenShaderIdentifier, shaderIdentifierSize, &rootArguments, sizeof(rootArguments)));
		m_rayGenShaderTable = rayGenShaderTable.GetResource();
	}

	// Miss shader table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable missShaderTable(m_device.Get(), numShaderRecords, shaderRecordSize, L"MissShaderTable");
		missShaderTable.push_back(ShaderRecord(missShaderIdentifier, shaderIdentifierSize));
		m_missShaderTable = missShaderTable.GetResource();
	}

	// Hit group shader table
	{
		UINT numShaderRecords = 1;
		UINT shaderRecordSize = shaderIdentifierSize;
		ShaderTable hitGroupShaderTable(m_device.Get(), numShaderRecords, shaderRecordSize, L"HitGroupShaderTable");
		hitGroupShaderTable.push_back(ShaderRecord(hitGroupShaderIdentifier, shaderIdentifierSize));
		m_hitGroupShaderTable = hitGroupShaderTable.GetResource();
	}
}

void RenderDevice::CreateRaytracingOutputResource()
{
	// Create the output resource. The dimensions and format should match the swap-chain.
	auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, m_width, m_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	VERIFYD3D12RESULT(m_device->CreateCommittedResource(
		&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&m_raytracingOutput)));
	NAME_D3D12_OBJECT(m_raytracingOutput);

	D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptorHandle;
	m_raytracingOutputResourceUAVDescriptorHeapIndex = AllocateDescriptor(&uavDescriptorHandle, m_raytracingOutputResourceUAVDescriptorHeapIndex);
	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	m_device->CreateUnorderedAccessView(m_raytracingOutput.Get(), nullptr, &UAVDesc, uavDescriptorHandle);
	m_raytracingOutputResourceUAVGpuDescriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_descriptorHeap->GetGPUDescriptorHandleForHeapStart(), m_raytracingOutputResourceUAVDescriptorHeapIndex, m_descriptorSize);
}

void RenderDevice::DoRaytracing()
{
	auto DispatchRays = [&](auto* commandList, auto* stateObject, auto* dispatchDesc)
		{
			// Since each shader table has only one shader record, the stride is same as the size.
			dispatchDesc->HitGroupTable.StartAddress = m_hitGroupShaderTable->GetGPUVirtualAddress();
			dispatchDesc->HitGroupTable.SizeInBytes = m_hitGroupShaderTable->GetDesc().Width;
			dispatchDesc->HitGroupTable.StrideInBytes = dispatchDesc->HitGroupTable.SizeInBytes;
			dispatchDesc->MissShaderTable.StartAddress = m_missShaderTable->GetGPUVirtualAddress();
			dispatchDesc->MissShaderTable.SizeInBytes = m_missShaderTable->GetDesc().Width;
			dispatchDesc->MissShaderTable.StrideInBytes = dispatchDesc->MissShaderTable.SizeInBytes;
			dispatchDesc->RayGenerationShaderRecord.StartAddress = m_rayGenShaderTable->GetGPUVirtualAddress();
			dispatchDesc->RayGenerationShaderRecord.SizeInBytes = m_rayGenShaderTable->GetDesc().Width;
			dispatchDesc->Width = m_width;
			dispatchDesc->Height = m_height;
			dispatchDesc->Depth = 1;
			commandList->SetPipelineState1(stateObject);
			commandList->DispatchRays(dispatchDesc);
		};

	m_commandList->SetComputeRootSignature(m_raytracingGlobalRootSignature.Get());


	// Bind the heaps, acceleration structure and dispatch rays.    
	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	m_commandList->SetDescriptorHeaps(1, m_descriptorHeap.GetAddressOf());
	m_commandList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, m_raytracingOutputResourceUAVGpuDescriptor);
	m_commandList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, m_topLevelAccelerationStructure->GetGPUVirtualAddress());
	DispatchRays(m_dxrCommandList.Get(), m_dxrStateObject.Get(), &dispatchDesc);
}

void RenderDevice::CopyRaytracingOutputToBackbuffer()
{
	D3D12_RESOURCE_BARRIER preCopyBarriers[2];
	preCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	preCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_raytracingOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_commandList->ResourceBarrier(ARRAYSIZE(preCopyBarriers), preCopyBarriers);

	m_commandList->CopyResource(m_renderTargets[m_frameIndex].Get(), m_raytracingOutput.Get());

	D3D12_RESOURCE_BARRIER postCopyBarriers[2];
	postCopyBarriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
	postCopyBarriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_raytracingOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	m_commandList->ResourceBarrier(ARRAYSIZE(postCopyBarriers), postCopyBarriers);
}
#pragma endregion

void RenderDevice::BuildGeometry()
{
	Index indices[] =
	{
		0, 1, 2
	};

	float depthValue = 1.0;
	float offset = 0.7f;
	Vertex vertices[] =
	{
		// The sample raytraces in screen space coordinates.
		// Since DirectX screen space coordinates are right handed (i.e. Y axis points down).
		// Define the vertices in counter clockwise order ~ clockwise in left handed.
		{ 0, -offset, depthValue },
		{ -offset, offset, depthValue },
		{ offset, offset, depthValue }
	};

	AllocateUploadBuffer(m_device.Get(), vertices, sizeof(vertices), &m_vertexBuffer);
	AllocateUploadBuffer(m_device.Get(), indices, sizeof(indices), &m_indexBuffer);
}

void RenderDevice::OnRender()
{
	//ImGui_ImplDX12_NewFrame();
	//ImGui_ImplWin32_NewFrame();
	//ImGui::NewFrame();
	//ImGui::ShowDemoWindow();

	m_commandAllocators[m_frameIndex]->Reset();
	VERIFYD3D12RESULT(m_computeCommandAllocators[m_frameIndex]->Reset());
	m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), nullptr);
	VERIFYD3D12RESULT(m_computeCommandList->Reset(m_computeCommandAllocators[m_frameIndex].Get(), nullptr));

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.

	{
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commandList->ResourceBarrier(1, &barrier);
	}



#pragma region RAY_TRACING
	//DoRaytracing();
	//CopyRaytracingOutputToBackbuffer();
#pragma endregion
	DispatchWorkGraph();
	CopyWorkGraphOutputToBackBuffer();
	//ImGui::Render();
	PopulateCommandLists();


	//ImGui::UpdatePlatformWindows();
	//ImGui::RenderPlatformWindowsDefault(nullptr, (void*)m_commandList.Get());

	m_commandList->Close();
	m_computeCommandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	{

		ID3D12CommandList* ppCommandLists[] = { m_computeCommandList.Get() };
		m_computeCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		m_computeCommandQueue->Signal(m_computeFence.Get(), m_fenceValues[m_frameIndex]);

		// Execute the rendering work only when the compute work is complete.
		m_commandQueue->Wait(m_computeFence.Get(), m_fenceValues[m_frameIndex]);
	}

	
	
	VERIFYD3D12RESULT(m_swapChain->Present(1, 0));
	
	MoveToNextFrame();
}

void RenderDevice::PopulateCommandLists()
{

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	//const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	//m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);


	m_commandList->SetDescriptorHeaps(1, m_srvHeap.GetAddressOf());

	//ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	m_commandList->ResourceBarrier(1, &barrier);

}

void RenderDevice::WaitForGPU()
{
	// Schedule Signal command
	VERIFYD3D12RESULT(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));

	// wait until fence has been processed by the gpu
	VERIFYD3D12RESULT(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

	++m_fenceValues[m_frameIndex];
}

void RenderDevice::MoveToNextFrame()
{
	const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
	VERIFYD3D12RESULT(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

	// Update the frame index.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();


	if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
	{
		VERIFYD3D12RESULT(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}

	// Set the fence value for the next frame.
	m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}

UINT RenderDevice::AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, UINT descriptorIndexToUse)
{
	auto descriptorHeapCpuBase = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	if (descriptorIndexToUse >= m_descriptorHeap->GetDesc().NumDescriptors)
	{
		descriptorIndexToUse = m_descriptorsAllocated++;
	}
	*cpuDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeapCpuBase, descriptorIndexToUse, m_descriptorSize);
	return descriptorIndexToUse;
}
