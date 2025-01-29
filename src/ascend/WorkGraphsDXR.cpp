#include "WorkGraphsDXR.h"
#include "DX12/DX12.h"
#include "DX12/DX12_Helpers.h"
#include "Shader/CompiledShaders/WorkGraphRaytracing.hlsl.h"

// To be moved to DXR namespace
#pragma region RAY_TRACING

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

#pragma endregion
// DXR

WorkGraphsDXR::WorkGraphsDXR()
	: m_shouldBuildAccelerationStructures(true)
{

}

WorkGraphsDXR::~WorkGraphsDXR()
{

}

void WorkGraphsDXR::Initialize(HWND hwnd, uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
	DX12::Initialize(D3D_FEATURE_LEVEL_12_2);
	m_swapChain.Initialize(hwnd, width, height);

	CreateWorkGraphRootSignature();
	CreateWorkGraph();
	DX12::WaitForGPU();
	CreateRaytracingInterfaces();

	LoadModels();
}

void WorkGraphsDXR::LoadModels()
{
	m_teapot = std::make_unique<Model>("debug/Shader/teapot.obj");
}

void WorkGraphsDXR::Update(float dt)
{
	// imgui frame
	// other stuff
}

void WorkGraphsDXR::Render()
{
	// destroy/create psos if updating shaders

	// update constant buffers

	DX12::StartFrame();
	m_swapChain.StartFrame();

	if (m_shouldBuildAccelerationStructures)
	{
		BuildAccelerationStructuresForCompute();
	}

	DispatchWorkGraph();
	CopyWorkGraphOutputToBackBuffer();

	m_swapChain.EndFrame();
	DX12::EndFrame(m_swapChain.GetD3DObject());
}

void WorkGraphsDXR::ImGUI()
{

}

void WorkGraphsDXR::CreateWorkGraph()
{
	static const wchar_t* WorkGraphProgramName = L"WorkGraph";

	CD3DX12_STATE_OBJECT_DESC stateObjectDesc(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);

	auto rootSignatureSubobject = stateObjectDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	rootSignatureSubobject->SetRootSignature(m_workGraphRootSignature.Get());

	auto workGraphSubobject = stateObjectDesc.CreateSubobject<CD3DX12_WORK_GRAPH_SUBOBJECT>();
	workGraphSubobject->IncludeAllAvailableNodes();
	workGraphSubobject->SetProgramName(WorkGraphProgramName);

	auto rootNodeDispatchGrid = workGraphSubobject->CreateBroadcastingLaunchNodeOverrides(L"EntryFunction");
	rootNodeDispatchGrid->DispatchGrid(ceil(m_width / 8), ceil(m_height / 8), 1);

	// create shader library
	auto lib = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE((void*)g_pWorkGraphRaytracing, ARRAYSIZE(g_pWorkGraphRaytracing));
	lib->SetDXILLibrary(&libdxil);

	VERIFYD3D12RESULT(DX12::Device->CreateStateObject(stateObjectDesc, IID_PPV_ARGS(&m_workGraphStateObject)));

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
		VERIFYD3D12RESULT(DX12::Device->CreateCommittedResource(&heapProperties,
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

void WorkGraphsDXR::CreateWorkGraphRootSignature()
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
	VERIFYD3D12RESULT(DX12::Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_workGraphRootSignature)))

	{
		m_workGraphDescriptorSize = DX12::Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, m_width, m_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		VERIFYD3D12RESULT(DX12::Device->CreateCommittedResource(
			&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&m_workGraphOutput)));


		D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(DX12::UAVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, DX12::UAVDescriptorSize);
		//m_raytracingOutputResourceUAVDescriptorHeapIndex = CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeapCpuBase, descriptorIndexToUse, m_computeDescriptorSize);
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		DX12::Device->CreateUnorderedAccessView(m_workGraphOutput.Get(), nullptr, &UAVDesc, uavDescriptorHandle);
		m_workGraphOutputUavDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(DX12::UAVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	}

}

void WorkGraphsDXR::DispatchWorkGraph()
{
	DX12::GraphicsCmdList->SetComputeRootSignature(m_workGraphRootSignature.Get());
	//D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	//DX12::GraphicsCmdList->SetDescriptorHeaps(1, m_srvHeap.GetAddressOf());
	DX12::GraphicsCmdList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, m_workGraphOutputUavDescriptorHandle);
	DX12::GraphicsCmdList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, m_topLevelAccelerationStructure->GetGPUVirtualAddress());


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

	DX12::GraphicsCmdList->SetProgram(&m_workGraphProgramDesc);
	DX12::GraphicsCmdList->DispatchGraph(&dispatchDesc);

	// Clear backing memory initialization flag, as the graph has run at least once now
	// See https://microsoft.github.io/DirectX-Specs/d3d/WorkGraphs.html#d3d12_set_work_graph_flags
	m_workGraphProgramDesc.WorkGraph.Flags &= ~D3D12_SET_WORK_GRAPH_FLAG_INITIALIZE;
}

void WorkGraphsDXR::CopyWorkGraphOutputToBackBuffer()
{
	DX12::TransitionResource(DX12::GraphicsCmdList.Get(), m_swapChain.BackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST, 0);
	DX12::TransitionResource(DX12::GraphicsCmdList.Get(), m_workGraphOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE, 0);

	DX12::GraphicsCmdList->CopyResource(m_swapChain.BackBuffer(), m_workGraphOutput.Get());

	DX12::TransitionResource(DX12::GraphicsCmdList.Get(), m_swapChain.BackBuffer(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET, 0);
	DX12::TransitionResource(DX12::GraphicsCmdList.Get(), m_workGraphOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0);

}

void WorkGraphsDXR::CreateRaytracingInterfaces()
{
	VERIFYD3D12RESULT(DX12::Device->QueryInterface(IID_PPV_ARGS(&m_dxrDevice)));
	VERIFYD3D12RESULT(DX12::GraphicsCmdList->QueryInterface(IID_PPV_ARGS(&m_dxrCommandList)));
}

void WorkGraphsDXR::BuildAccelerationStructuresForCompute()
{
	const uint64_t modelMeshCount = m_teapot->GetModelMeshVector().size();
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs(modelMeshCount);
	for (int i = 0; i < modelMeshCount; ++i)
	{
		geometryDescs[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometryDescs[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geometryDescs[i].Triangles.VertexCount = m_teapot->GetModelMeshVector()[i].numVertices;;
		geometryDescs[i].Triangles.VertexBuffer.StartAddress = m_teapot->GetVertexBuffer().BufferLocation + m_teapot->GetModelMeshVector()[i].vertexOffset;
		geometryDescs[i].Triangles.VertexBuffer.StrideInBytes = m_teapot->GetVertexBuffer().StrideInBytes;
		geometryDescs[i].Triangles.Transform3x4 = 0;
		geometryDescs[i].Triangles.IndexBuffer = m_teapot->GetIndexBuffer().BufferLocation + m_teapot->GetModelMeshVector()[i].indexOffset;
		geometryDescs[i].Triangles.IndexCount = m_teapot->GetModelMeshVector()[i].numIndices;
		geometryDescs[i].Triangles.IndexFormat = m_teapot->GetIndexBuffer().Format;

		geometryDescs[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	}

	// Build TLAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelAccelerationStructureDesc = {};
	topLevelAccelerationStructureDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	topLevelAccelerationStructureDesc.Inputs.NumDescs = modelMeshCount;
	topLevelAccelerationStructureDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	topLevelAccelerationStructureDesc.Inputs.pGeometryDescs = nullptr;
	topLevelAccelerationStructureDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo;
	m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelAccelerationStructureDesc.Inputs, &topLevelPrebuildInfo);

	const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlag = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
	UINT64 scratchBufferSizeNeeded = topLevelPrebuildInfo.ScratchDataSizeInBytes;


	tlasScratchBuffer.Create(L"Acceleration Structure Scratch Buffer", (UINT)scratchBufferSizeNeeded, 1);

	D3D12_HEAP_PROPERTIES defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto tlasBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	DX12::Device->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&tlasBufferDesc,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
		nullptr,
		IID_PPV_ARGS(&m_topLevelAccelerationStructure));

	topLevelAccelerationStructureDesc.DestAccelerationStructureData = m_topLevelAccelerationStructure->GetGPUVirtualAddress();
	topLevelAccelerationStructureDesc.ScratchAccelerationStructureData = tlasScratchBuffer.GetGpuVirtualAddress();

	// Build BLAS
	m_bottomLevelAccelerationStructures.resize(modelMeshCount);
	blasScratchBuffers.resize(modelMeshCount);
	std::vector<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC> blasDescs(modelMeshCount);
	std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDesc(modelMeshCount);

	for (UINT i = 0; i < modelMeshCount; i++)
	{
		blasDescs[i].Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		blasDescs[i].Inputs.NumDescs = 1;
		blasDescs[i].Inputs.pGeometryDescs = &geometryDescs[i];
		blasDescs[i].Inputs.Flags = buildFlag;
		blasDescs[i].Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo;

		m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&blasDescs[i].Inputs, &bottomLevelPrebuildInfo);

		D3D12_RESOURCE_DESC blasBufferDesc = {};

		blasBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		VERIFYD3D12RESULT(DX12::Device->CreateCommittedResource(
			&defaultHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&blasBufferDesc,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nullptr,
			IID_PPV_ARGS(&m_bottomLevelAccelerationStructures[i])));

		blasDescs[i].DestAccelerationStructureData = m_bottomLevelAccelerationStructures[i]->GetGPUVirtualAddress();

		blasScratchBuffers[i].Create(L"BLAS build scratch buffer", (UINT)bottomLevelPrebuildInfo.ScratchDataSizeInBytes, 1);
		blasDescs[i].ScratchAccelerationStructureData = blasScratchBuffers[i].GetGpuVirtualAddress();

		// Identity matrix
		ZeroMemory(instanceDesc[i].Transform, sizeof(instanceDesc[i].Transform));
		instanceDesc[i].Transform[0][0] = 1.0f;
		instanceDesc[i].Transform[1][1] = 1.0f;
		instanceDesc[i].Transform[2][2] = 1.0f;

		instanceDesc[i].AccelerationStructure = m_bottomLevelAccelerationStructures[i]->GetGPUVirtualAddress();
		instanceDesc[i].Flags = 0;
		instanceDesc[i].InstanceID = 0;
		instanceDesc[i].InstanceMask = 1;
		instanceDesc[i].InstanceContributionToHitGroupIndex = i;

	}

	AllocateUploadBuffer(DX12::Device.Get(), instanceDesc.data(), instanceDesc.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC), &instanceDescs, L"InstanceDescs");

	topLevelAccelerationStructureDesc.Inputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
	topLevelAccelerationStructureDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

	auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
	for (UINT i = 0; i < blasDescs.size(); i++)
	{
		m_dxrCommandList->BuildRaytracingAccelerationStructure(&blasDescs[i], 0, nullptr);

		// If each BLAS build reuses the scratch buffer, you would need a UAV barrier between each. But without
		// barriers, the driver may be able to batch these BLAS builds together. This maximizes GPU utilization
		// and should execute more quickly.
	}

	m_dxrCommandList->ResourceBarrier(1, &uavBarrier);
	m_dxrCommandList->BuildRaytracingAccelerationStructure(&topLevelAccelerationStructureDesc, 0, nullptr);
	m_shouldBuildAccelerationStructures = false;
}