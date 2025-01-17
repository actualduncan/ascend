#include <d3dx12/d3dx12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>// include d3d
#include <wrl.h>
#include <DirectXMath.h>

using namespace Microsoft::WRL;
using namespace DirectX;

namespace DX12 
{

	const uint64_t RenderLatency = 2;
	// for syncro
	extern UINT64 CurrentCPUFrame;  
	extern UINT64 CurrentGPUFrame;  
	extern UINT64 CurrFrameIdx;     

	void Initialize(D3D_FEATURE_LEVEL featureLevel);

	void StartFrame();

	void EndFrame();
}