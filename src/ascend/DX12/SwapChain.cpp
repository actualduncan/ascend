#include "SwapChain.h"
#include "DX12.h"
#include "DX12_Helpers.h"

SwapChain::SwapChain()
{

}

SwapChain::~SwapChain()
{

}

void SwapChain::Initialize(HWND hwnd, uint32_t width, uint32_t height)
{
	// first output
	DX12::Adapter->EnumOutputs(0, &m_output);

	DXGI_SWAP_CHAIN_DESC1 desc = { };
	desc.BufferCount = BACK_BUFFER_NUM;
	desc.Width = m_width;
	desc.Height = m_height;
	desc.Format = m_format;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	desc.Scaling = DXGI_SCALING_STRETCH;
	desc.Stereo = FALSE;

	ComPtr<IDXGISwapChain1> tempSwapChain = nullptr;

	VERIFYD3D12RESULT(DX12::Factory->CreateSwapChainForHwnd(DX12::Device.Get(), hwnd, &desc, nullptr, m_output.Get(), &tempSwapChain));
	VERIFYD3D12RESULT(tempSwapChain.As(&m_swapChain));

	m_backBufferIdx = m_swapChain->GetCurrentBackBufferIndex();
	m_waitableObject = m_swapChain->GetFrameLatencyWaitableObject();

	// Create RTVs

	for (int i = 0; i < BACK_BUFFER_NUM; ++i)
	{

		VERIFYD3D12RESULT(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBufferRT[i])));

		wchar_t name[25] = {};
		swprintf_s(name, L"SwapChain Render target %u", i);
		m_backBufferRT[i]->SetName(name);

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Format = m_format;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(DX12::RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), i, DX12::RTVDescriptorSize);
		DX12::Device->CreateRenderTargetView(m_backBufferRT[i].Get(), &rtvDesc, rtvDescriptor);
	}

	m_backBufferIdx = m_swapChain->GetCurrentBackBufferIndex();
}

void SwapChain::StartFrame()
{
	m_backBufferIdx = m_swapChain->GetCurrentBackBufferIndex();
	DX12::TransitionResource(DX12::GraphicsCmdList.Get(), m_backBufferRT[m_backBufferIdx].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, 0);
}

void SwapChain::EndFrame()
{
	DX12::TransitionResource(DX12::GraphicsCmdList.Get(), m_backBufferRT[m_backBufferIdx].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT, 0);
}