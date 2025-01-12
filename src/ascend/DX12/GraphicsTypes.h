#include "DX12.h"

// to fill with graphics types 
struct DX12Buffer
{
	ComPtr<ID3D12Resource> buffer;
};

struct DX12Fence
{
	ComPtr<ID3D12Fence> fence;
	
};