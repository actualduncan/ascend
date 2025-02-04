#include "DX12/SwapChain.h"
#include "Model.h"
#include "Camera.h"
#include "InputCommands.h"
struct RayTraceConstants
{
	//XMFLOAT4X4 InvViewProjection;
	XMMATRIX InvViewProjection;
	XMFLOAT4 CameraPosWS;
	XMFLOAT2 yes[22];
};

class WorkGraphsDXR
{
public:
	
	WorkGraphsDXR();
	~WorkGraphsDXR();
	

	void Initialize(HWND hwnd, uint32_t width, uint32_t height);
	void LoadModels();
	void Update(float dt, InputCommands* inputCommands);
	void Render();
	void ImGUI();

private:
	SwapChain m_swapChain;
	RayTraceConstants m_constantBufferData;
	ComPtr<ID3D12Resource> m_indexBuffer;
	ComPtr<ID3D12Resource> m_vertexBuffer;
	ComPtr<ID3D12Resource> m_constantBuffer;
	UINT8* m_pCbvDataBegin = nullptr;
	std::unique_ptr<Model> m_teapot;
	std::unique_ptr<Camera> m_camera;
	InputCommands m_input;

#pragma region WORK_GRAPHS
	ComPtr<ID3D12RootSignature> m_workGraphRootSignature;
	ComPtr<ID3D12PipelineState> m_workGraphPipelineState;
	ComPtr<ID3D12Resource> m_workGraphOutput;
	ComPtr<ID3D12Resource>    m_workGraphBackingMemory;

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

	bool m_shouldBuildAccelerationStructures;

	ComPtr<ID3D12Resource> scratchResource;
	ComPtr<ID3D12Resource> instanceDescs;
	GpuBuffer tlasScratchBuffer;
	std::vector<GpuBuffer> blasScratchBuffers;

	// compute shennanigans
	void LoadComputeAssets();
	void DoCompute();
	void CopyComputeOutputToBackBuffer();
	ComPtr<ID3D12RootSignature> m_computeRootSignature;
	ComPtr<ID3D12PipelineState> m_computePipelineState;
	ComPtr<ID3D12Resource> m_computeOutput;
	D3D12_GPU_DESCRIPTOR_HANDLE m_computeOutputUavDescriptorHandle;
};