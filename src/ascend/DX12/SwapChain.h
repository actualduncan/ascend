#include "PCH.h"
#include "DX12.h"


class SwapChain
{
public:
	SwapChain();
	~SwapChain();

	void Initialize(HWND hwnd, uint32_t width, uint32_t height);

	IDXGISwapChain4* GetD3DObject() { return m_swapChain.Get(); }
	ID3D12Resource* BackBuffer() const { return m_backBufferRT[m_backBufferIdx].Get(); };

	void StartFrame();
	void EndFrame();

	void SetFormat(DXGI_FORMAT format) { m_format = format; };
	void SetWidth(uint32_t width) { m_width = width; };
	void SetHeight(uint32_t height) { m_height = height; };
	void SetFullScreen(bool enabled) { m_isFullScreen = enabled; };
private:
	static const uint64_t BACK_BUFFER_NUM = 2;

	// actual Swap Chain
	ComPtr<IDXGISwapChain4> m_swapChain = nullptr;

	// Back Buffer Resources
	uint32_t m_backBufferIdx = 0;
	ComPtr<ID3D12Resource> m_backBufferRT[BACK_BUFFER_NUM];

	ComPtr<IDXGIOutput> m_output;

	DXGI_FORMAT m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
	
	uint32_t m_width = 1280;
	uint32_t m_height = 720;

	HANDLE m_waitableObject = INVALID_HANDLE_VALUE;

	bool m_isFullScreen;
};