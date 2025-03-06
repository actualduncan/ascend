#include "imgui/imgui.h"
#include "DX12/DX12.h"
class IMGUI_Helper
{
public:
	IMGUI_Helper(HWND hwnd);
	~IMGUI_Helper();

	void InitWin32DX12();
	void StartFrame();
	void EndFrame();
	void CreatePSO();
private:
	ImGuiStyle m_imguiStyle;
	HWND m_hwnd;
	ComPtr<ID3D12PipelineState> m_pipelineStateObject;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	D3D12_GPU_DESCRIPTOR_HANDLE handle;
};