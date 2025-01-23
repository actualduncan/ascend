#include "DX12.h"

// to fill with graphics types 
struct Fence
{
	ComPtr<ID3D12Fence> DX12fence;
	HANDLE eventHandle;

	void Init(uint64_t initialValue);
	void Signal(ID3D12CommandQueue* commandQueue, uint64_t fenceValue);
	void Wait(uint64_t fenceValue);
	bool Signaled(uint64_t fenceValue);
	void Clear(uint64_t fenceValue);
};