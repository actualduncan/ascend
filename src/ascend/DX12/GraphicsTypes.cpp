#include "GraphicsTypes.h"
#include "DX12_Helpers.h"
void Fence::Init(uint64_t initialValue)
{
    VERIFYD3D12RESULT(DX12::Device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&DX12fence)));
    eventHandle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
}


void Fence::Signal(ID3D12CommandQueue* queue, uint64_t fenceValue)
{
    VERIFYD3D12RESULT(queue->Signal(DX12fence.Get(), fenceValue));
}

void Fence::Wait(uint64_t fenceValue)
{
    if (DX12fence->GetCompletedValue() < fenceValue)
    {
        VERIFYD3D12RESULT(DX12fence->SetEventOnCompletion(fenceValue, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
    }
}

bool Fence::Signaled(uint64_t fenceValue)
{
    return DX12fence->GetCompletedValue() >= fenceValue;
}

void Fence::Clear(uint64_t fenceValue)
{
    DX12fence->Signal(fenceValue);
}
