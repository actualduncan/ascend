#include "WorkGraphsDXR.h"
#include "DX12/DX12.h"
#include "DX12/DX12_Helpers.h"

#include <d3dcompiler.h>
#include "Shader/CompiledShaders/WorkGraphRaytracing.hlsl.h"
#include "Shader/CompiledShaders/ComputeRaytracing.hlsl.h"
#include "Shader/CompiledShaders/RasterPS.hlsl.h"
#include "Shader/CompiledShaders/RasterVS.hlsl.h"
// To be moved to DXR namespace
#pragma region RAY_TRACING

namespace GlobalRootSignatureParams {

	enum Value {
		OutputViewSlot = 0,
		AccelerationStructureSlot,
		ConstantBufferSlot,
		Count
	};
}

namespace LocalRootSignatureParams {
	enum Value {
		ViewportConstantSlot = 0,
		ConstantBufferSlot,
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
	m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
	m_scissorRect = CD3DX12_RECT(0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height));

	D3D12_RESOURCE_DESC depthStencilDesc = {};
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = width;
	depthStencilDesc.Height = height;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear = {};
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	VERIFYD3D12RESULT(DX12::Device->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(&m_depthStencilBuffer)));
	DX12::Device->CreateDepthStencilView(m_depthStencilBuffer.Get(), nullptr, DX12::DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	DX12::TransitionResource(DX12::GraphicsCmdList.Get(), m_depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE, 0);
	// Create the constant buffer.
	{
		const UINT constantBufferSize = sizeof(RayTraceConstants);    // CB size is required to be 256-byte aligned.
		auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

		VERIFYD3D12RESULT(DX12::Device->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_constantBuffer)));
		m_constantBuffer->SetName(L"Constant Buffer");
		// Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = constantBufferSize;
		DX12::Device->CreateConstantBufferView(&cbvDesc, DX12::UAVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		
		// Map and initialize the constant buffer. We don't unmap this until the
		// app closes. Keeping things mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		VERIFYD3D12RESULT(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
		memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
	}

	if (true)
	{
		LoadRasterAssets();
		//CreateWorkGraphRootSignature();
		//CreateWorkGraph();
	}
	else
	{
		LoadComputeAssets();
	}

	DX12::WaitForGPU();
	CreateRaytracingInterfaces();
	float aspectRatio = float(width) / float(height);
	m_camera = std::make_unique<Camera>(hwnd, aspectRatio);
	LoadModels();
}

std::vector<std::unique_ptr<Texture>> textures;
D3D12_GPU_DESCRIPTOR_HANDLE texhandle;
void WorkGraphsDXR::LoadModels()
{
	m_sponza = std::make_unique<Model>("debug/res/sponza.obj");
	textures.push_back(std::make_unique<Texture>(0, L"debug/res/textures/sponza_column_b_bump.dds"));
	textures.push_back(std::make_unique<Texture>(1, L"debug/res/textures/sponza_thorn_diff.dds"));
	textures.push_back(std::make_unique<Texture>(3, L"debug/res/textures/vase_plant.dds"));
	textures.push_back(std::make_unique<Texture>(2, L"debug/res/textures/vase_round.dds"));
	textures.push_back(std::make_unique<Texture>(4, L"debug/res/textures/background.dds"));
	textures.push_back(std::make_unique<Texture>(5, L"debug/res/textures/spnza_bricks_a_diff.dds"));
	textures.push_back(std::make_unique<Texture>(6, L"debug/res/textures/sponza_arch_diff.dds"));
	textures.push_back(std::make_unique<Texture>(7, L"debug/res/textures/sponza_ceiling_a_diff.dds"));
	textures.push_back(std::make_unique<Texture>(8, L"debug/res/textures/sponza_column_a_diff.dds"));
	textures.push_back(std::make_unique<Texture>(9, L"debug/res/textures/sponza_floor_a_diff.dds"));
	textures.push_back(std::make_unique<Texture>(10, L"debug/res/textures/sponza_column_c_diff.dds"));
	textures.push_back(std::make_unique<Texture>(11, L"debug/res/textures/sponza_details_diff.dds"));
	textures.push_back(std::make_unique<Texture>(12, L"debug/res/textures/sponza_column_b_diff.dds"));
	textures.push_back(std::make_unique<Texture>(13, L"debug/res/textures/sponza_column_b_bump.dds"));
	textures.push_back(std::make_unique<Texture>(14, L"debug/res/textures/sponza_flagpole_diff.dds"));
	textures.push_back(std::make_unique<Texture>(15, L"debug/res/textures/sponza_fabric_green_diff.dds"));
	textures.push_back(std::make_unique<Texture>(16, L"debug/res/textures/sponza_fabric_blue_diff.dds"));
	textures.push_back(std::make_unique<Texture>(17, L"debug/res/textures/sponza_fabric_diff.dds"));
	textures.push_back(std::make_unique<Texture>(18, L"debug/res/textures/sponza_curtain_blue_diff.dds"));
	textures.push_back(std::make_unique<Texture>(19, L"debug/res/textures/sponza_curtain_diff.dds"));
	textures.push_back(std::make_unique<Texture>(20, L"debug/res/textures/sponza_curtain_green_diff.dds"));
	textures.push_back(std::make_unique<Texture>(21, L"debug/res/textures/chain_texture.dds"));
	textures.push_back(std::make_unique<Texture>(22, L"debug/res/textures/vase_hanging.dds"));
	textures.push_back(std::make_unique<Texture>(23, L"debug/res/textures/vase_dif.dds"));
	textures.push_back(std::make_unique<Texture>(24, L"debug/res/textures/lion.dds"));
	textures.push_back(std::make_unique<Texture>(25, L"debug/res/textures/sponza_roof_diff.dds"));


}

void WorkGraphsDXR::Update(float dt, InputCommands* inputCommands)
{
	// get this frame's input

	m_input = *inputCommands;
	m_camera->Update(dt, *inputCommands);
	XMMATRIX viewProj = m_camera->GetView() * m_camera->GetProjectionMatrix();
	XMVECTOR det = XMMatrixDeterminant(viewProj);
	m_constantBufferData.InvViewProjection = XMMatrixTranspose(viewProj);//XMMatrixTranspose(XMMatrixInverse(nullptr, viewProj));
	m_constantBufferData.CameraPosWS = XMFLOAT4(m_camera->GetPosition().x, m_camera->GetPosition().y, m_camera->GetPosition().z, 1.0f);
	memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

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
		//BuildAccelerationStructuresForCompute();
	}

	if (true)
	{
		DoRaster();
		//DispatchWorkGraph();
		//CopyWorkGraphOutputToBackBuffer();
	}
	else
	{
		DoCompute();
		CopyComputeOutputToBackBuffer();
	}


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
	rootNodeDispatchGrid->DispatchGrid(m_width, m_height, 1);

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
	CD3DX12_ROOT_PARAMETER rootParameters[3];
	rootParameters[0].InitAsDescriptorTable(1, &UAVDescriptor);
	rootParameters[1].InitAsShaderResourceView(0);
	rootParameters[2].InitAsConstantBufferView(0);
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
	DX12::GraphicsCmdList->SetComputeRootConstantBufferView(GlobalRootSignatureParams::ConstantBufferSlot, m_constantBuffer->GetGPUVirtualAddress());

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
	const uint64_t modelMeshCount = m_sponza->GetModelMeshVector().size();
	std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs(modelMeshCount);
	for (int i = 0; i < modelMeshCount; ++i)
	{
		geometryDescs[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometryDescs[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geometryDescs[i].Triangles.VertexCount = m_sponza->GetModelMeshVector()[i].numVertices;;
		geometryDescs[i].Triangles.VertexBuffer.StartAddress = m_sponza->GetVertexBuffer().BufferLocation + m_sponza->GetModelMeshVector()[i].vertexOffset;
		geometryDescs[i].Triangles.VertexBuffer.StrideInBytes = m_sponza->GetVertexBuffer().StrideInBytes;
		geometryDescs[i].Triangles.Transform3x4 = 0;
		geometryDescs[i].Triangles.IndexBuffer = m_sponza->GetIndexBuffer().BufferLocation + m_sponza->GetModelMeshVector()[i].indexOffset;
		geometryDescs[i].Triangles.IndexCount = m_sponza->GetModelMeshVector()[i].numIndices;
		geometryDescs[i].Triangles.IndexFormat = m_sponza->GetIndexBuffer().Format;

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

void WorkGraphsDXR::LoadComputeAssets()
{
	{
		CD3DX12_DESCRIPTOR_RANGE UAVDescriptor;
		UAVDescriptor.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
		CD3DX12_ROOT_PARAMETER rootParameters[3];
		rootParameters[0].InitAsDescriptorTable(1, &UAVDescriptor);
		rootParameters[1].InitAsShaderResourceView(0);
		rootParameters[2].InitAsConstantBufferView(0);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc(ARRAYSIZE(rootParameters), rootParameters);

		VERIFYD3D12RESULT(D3DX12SerializeVersionedRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error));
		VERIFYD3D12RESULT(DX12::Device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_computeRootSignature)));
	}
	// create pso
	{
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
		ComPtr<ID3DBlob> computeShader;
		ComPtr<ID3DBlob> error;
		//VERIFYD3D12RESULT(D3DCompileFromFile(L"C:/Users/Duncan/Desktop/_dev/ascend/ascend/build/src/Debug/Shader/ComputeRaytracing.hlsl", nullptr, nullptr, "CSMain", "cs_5_1", compileFlags, 0, &computeShader, &error));
		D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
		desc.pRootSignature = m_computeRootSignature.Get();
		desc.CS = CD3DX12_SHADER_BYTECODE(CD3DX12_SHADER_BYTECODE((void*)g_pComputeRaytracing, ARRAYSIZE(g_pComputeRaytracing)));

		VERIFYD3D12RESULT(DX12::Device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_computePipelineState)));
		NAME_D3D12_OBJECT(m_computePipelineState);
	}



	{
		auto uavDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, m_width, m_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

		auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		VERIFYD3D12RESULT(DX12::Device->CreateCommittedResource(
			&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &uavDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&m_computeOutput)));


		D3D12_CPU_DESCRIPTOR_HANDLE uavDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(DX12::UAVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, DX12::UAVDescriptorSize);
		//m_raytracingOutputResourceUAVDescriptorHeapIndex = CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeapCpuBase, descriptorIndexToUse, m_computeDescriptorSize);
		D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		DX12::Device->CreateUnorderedAccessView(m_computeOutput.Get(), nullptr, &UAVDesc, uavDescriptorHandle);
		m_computeOutputUavDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(DX12::UAVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	}
}

void WorkGraphsDXR::DoCompute()
{
	DX12::GraphicsCmdList->SetPipelineState(m_computePipelineState.Get());
	//D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	DX12::GraphicsCmdList->SetDescriptorHeaps(1, DX12::UAVDescriptorHeap.GetAddressOf());
	DX12::GraphicsCmdList->SetComputeRootSignature(m_computeRootSignature.Get());
	DX12::GraphicsCmdList->SetComputeRootDescriptorTable(GlobalRootSignatureParams::OutputViewSlot, m_computeOutputUavDescriptorHandle);
	DX12::GraphicsCmdList->SetComputeRootShaderResourceView(GlobalRootSignatureParams::AccelerationStructureSlot, m_topLevelAccelerationStructure->GetGPUVirtualAddress());
	DX12::GraphicsCmdList->SetComputeRootConstantBufferView(GlobalRootSignatureParams::ConstantBufferSlot, m_constantBuffer->GetGPUVirtualAddress());
	DX12::GraphicsCmdList->Dispatch(m_width, m_height, 1);
}

void WorkGraphsDXR::CopyComputeOutputToBackBuffer()
{
	DX12::TransitionResource(DX12::GraphicsCmdList.Get(), m_swapChain.BackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST, 0);
	DX12::TransitionResource(DX12::GraphicsCmdList.Get(), m_computeOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE, 0);

	DX12::GraphicsCmdList->CopyResource(m_swapChain.BackBuffer(), m_computeOutput.Get());

	DX12::TransitionResource(DX12::GraphicsCmdList.Get(), m_swapChain.BackBuffer(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET, 0);
	DX12::TransitionResource(DX12::GraphicsCmdList.Get(), m_computeOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0);
}

void WorkGraphsDXR::LoadRasterAssets()
{
	 // Create the root signature.
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		CD3DX12_ROOT_PARAMETER1 rootParameters[2];
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[1].InitAsConstantBufferView(0);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
		VERIFYD3D12RESULT(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signature, &error));
		VERIFYD3D12RESULT(DX12::Device ->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rasterRootSignature)));
	}

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif
		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rasterRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE((void*)g_pRasterVS, ARRAYSIZE(g_pRasterVS));
		psoDesc.PS = CD3DX12_SHADER_BYTECODE((void*)g_pRasterPS, ARRAYSIZE(g_pRasterPS));
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; 
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		VERIFYD3D12RESULT(DX12::Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_rasterPipelineState)));
	}
}
void WorkGraphsDXR::DoRaster()
{

	DX12::GraphicsCmdList->SetPipelineState(m_rasterPipelineState.Get());
	DX12::GraphicsCmdList->SetGraphicsRootSignature(m_rasterRootSignature.Get());
	//DX12::GraphicsCmdList->SetGraphicsRootDescriptorTable(0, DX12::UAVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	DX12::GraphicsCmdList->SetGraphicsRootConstantBufferView(1, m_constantBuffer->GetGPUVirtualAddress());

	DX12::GraphicsCmdList->RSSetViewports(1, &m_viewport);
	DX12::GraphicsCmdList->RSSetScissorRects(1, &m_scissorRect);


	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(DX12::RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_swapChain.GetD3DObject()->GetCurrentBackBufferIndex(), DX12::RTVDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(DX12::DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 0, DX12::DSVDescriptorSize);
	DX12::GraphicsCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	DX12::GraphicsCmdList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	DX12::GraphicsCmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	const uint64_t modelMeshCount = m_sponza->GetModelMeshVector().size();
	for (int i = 0; i < modelMeshCount; ++i)
	{
			
		auto mesh = m_sponza->GetModelMeshVector()[i];

		CD3DX12_GPU_DESCRIPTOR_HANDLE hDescriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(DX12::UAVDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), mesh.materialIdx, DX12::UAVDescriptorSize);
		DX12::GraphicsCmdList->SetGraphicsRootDescriptorTable(0, hDescriptor);
		
		D3D12_VERTEX_BUFFER_VIEW vbView = mesh.GetVertexBufferView();
		D3D12_INDEX_BUFFER_VIEW ibView = mesh.GetIndexBufferView();

		DX12::GraphicsCmdList->IASetVertexBuffers(0, 1, &vbView);
		DX12::GraphicsCmdList->IASetIndexBuffer(&ibView);
		DX12::GraphicsCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		DX12::GraphicsCmdList->DrawIndexedInstanced(mesh.numIndices, 1, 0, 0, 0);
	}
}
