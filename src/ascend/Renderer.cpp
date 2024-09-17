#include "Renderer.h"
#include <d3dcompiler.h>
#include "AscendHelpers.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"
/*
												WARNING
	This whole file needs refactoring, this is currently a spike implemnetation of a D3D12 Renderer
									The code is therefore very messy.
		Most of this code is adapted from https://github.com/microsoft/DirectX-Graphics-Samples
 */


RenderDevice::RenderDevice()
{
}

RenderDevice::~RenderDevice()
{	
	
	//Shutdown();
}

bool RenderDevice::Initialize(HWND hwnd)
{

	bool hasInitialized = true;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	{
		ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
		swapChainDesc.BufferCount = RendererPrivate::MAX_FRAMES;
		swapChainDesc.Width = 0;
		swapChainDesc.Height = 0;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc.Stereo = FALSE;
	}

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


	ComPtr<IDXGIFactory7> m_factory;
	VERIFYD3D12RESULT(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));

	for (uint32_t id = 0; DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapterByGpuPreference(id, DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&m_adapter)); ++id)
	{
		// Contains description of the graphics hardware adapter (GPU)
		DXGI_ADAPTER_DESC3 desc;
		m_adapter->GetDesc3(&desc);

		// Device that supports D3D12 is found
		if (SUCCEEDED(D3D12CreateDevice(m_adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device))))
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

	{
		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 1;
		VERIFYD3D12RESULT(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue)));
	}


	//ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));


	{
		ComPtr<IDXGISwapChain1> swapChain1;

		// TODO: Add fullscreen support
		VERIFYD3D12RESULT(m_factory->CreateSwapChainForHwnd(m_commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));	// use CreateSwapChainForCoreWindow for Windows Store apps
		VERIFYD3D12RESULT(m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
		VERIFYD3D12RESULT(swapChain1.As(&m_swapChain)); // seems like you can only create a swapchain1. Therefore to get currentBackbuffer

		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
		VERIFYD3D12RESULT(swapChain1->QueryInterface(IID_PPV_ARGS(&m_swapChain)));
		//swapChain1->Release();
		//m_swapChain->SetMaximumFrameLatency(RendererPrivate::MAX_FRAMES);
		//m_SwapChainWaitableObject = m_swapChain->GetFrameLatencyWaitableObject();
		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	}

	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = RendererPrivate::MAX_FRAMES;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		VERIFYD3D12RESULT(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV and a command allocator for each frame.
		for (UINT n = 0; n < RendererPrivate::MAX_FRAMES; n++)
		{
			VERIFYD3D12RESULT(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);

			VERIFYD3D12RESULT(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n])));
		}
	}

	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		VERIFYD3D12RESULT(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srvHeap)));
	}

	ImGui_ImplDX12_Init(m_device.Get(), RendererPrivate::MAX_FRAMES, DXGI_FORMAT_R8G8B8A8_UNORM,
		m_srvHeap.Get(),
		m_srvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_srvHeap->GetGPUDescriptorHandleForHeapStart());


	// syncro objects (fences)

	VERIFYD3D12RESULT(m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	++m_fenceValues[m_frameIndex];

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
	{
		VERIFYD3D12RESULT(HRESULT_FROM_WIN32(GetLastError()));
	}
	

	/*
	for (UINT i = 0; i < RendererPrivate::MAX_FRAMES; i++)
	{
		ID3D12Resource* pBackBuffer = nullptr;
		m_swapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
		m_device->CreateRenderTargetView(pBackBuffer, nullptr, m_renderTargetDescriptors[i]);
		m_renderTargets[i] = pBackBuffer;
	}
	*/

	VERIFYD3D12RESULT(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
	m_commandList->Close();

	WaitForGPU();
	return true;
}
void RenderDevice::OnRender()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::ShowDemoWindow();



	ImGui::Render();
	
	PopulateCommandLists();

	ImGui::UpdatePlatformWindows();
	ImGui::RenderPlatformWindowsDefault(nullptr, (void*)m_commandList.Get());
	VERIFYD3D12RESULT(m_swapChain->Present(1, 0));

	MoveToNextFrame();
	// (Your code calls ExecuteCommandLists, swapchain's Present(), etc.)
}
void RenderDevice::PopulateCommandLists()
{
	// Command list allocators can only be reset when the associated 
// command lists have finished execution on the GPU; apps should use 
// fences to determine GPU execution progress.
	//VERIFYD3D12RESULT(m_commandAllocators[0]->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.

	// Set necessary state.
	//m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	//m_commandList->RSSetViewports(1, &m_viewport);
	//m_commandList->RSSetScissorRects(1, &m_scissorRect);
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	// = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	// Indicate that the back buffer will be used as a render target.
	VERIFYD3D12RESULT(m_commandList->Reset(m_commandAllocators[0].Get(), nullptr));
	m_commandList->ResourceBarrier(1, &barrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	m_commandList->SetDescriptorHeaps(1, m_srvHeap.GetAddressOf());
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList.Get());
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	m_commandList->ResourceBarrier(1, &barrier);
	m_commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//VERIFYD3D12RESULT(m_commandList->Close());
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