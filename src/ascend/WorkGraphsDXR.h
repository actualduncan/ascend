#include "DX12/SwapChain.h"
#include "Model.h"

class WorkGraphsDXR
{
public:
	
	WorkGraphsDXR();
	~WorkGraphsDXR();
	

	void Initialize(HWND hwnd, uint32_t width, uint32_t height);
	void Update(float dt);
	void Render();
	void ImGUI();
	void BuildGeometry();

private:
	SwapChain m_swapChain;


	ComPtr<ID3D12Resource> m_indexBuffer;
	ComPtr<ID3D12Resource> m_vertexBuffer;

#pragma region WORK_GRAPHS
	ComPtr<ID3D12RootSignature> m_workGraphRootSignature;
	ComPtr<ID3D12PipelineState> m_workGraphPipelineState;
	ComPtr<ID3D12Resource> m_workGraphOutput;
	ComPtr<ID3D12Resource>    m_workGraphBackingMemory;
	//ComPtr<ID3D12Fence> m_computeFence;

	ComPtr<ID3D12StateObject> m_workGraphStateObject;
	D3D12_SET_PROGRAM_DESC    m_workGraphProgramDesc = {};
	std::uint32_t             m_workGraphEntryPointIndex;
	D3D12_GPU_DESCRIPTOR_HANDLE m_workGraphOutputUavDescriptorHandle;
	UINT m_workGraphOutputUavDescriptorIndex;
	UINT m_workGraphDescriptorSize;

	void CreateWorkGraphRootSignature();
	void CreateWorkGraph();
	void DispatchWorkGraph();
	void CopyWorkGraphOutputToBackBuffer();



#pragma endregion

	ComPtr<ID3D12Device5> m_dxrDevice;
	ComPtr<ID3D12GraphicsCommandList4> m_dxrCommandList;
	ComPtr<ID3D12Resource> m_accelerationStructure;
	std::vector<ComPtr<ID3D12Resource>> m_bottomLevelAccelerationStructures;
	ComPtr<ID3D12Resource> m_topLevelAccelerationStructure;
	void CreateRaytracingInterfaces();
	void BuildAccelerationStructuresForCompute();

	uint32_t m_width;
	uint32_t m_height;

	bool accelStructBuilt = false;

	ComPtr<ID3D12Resource> scratchResource;
	ComPtr<ID3D12Resource> instanceDescs;
	GpuBuffer tlasScratchBuffer;
	std::vector<GpuBuffer> blasScratchBuffers;
	Model* bunny;
};