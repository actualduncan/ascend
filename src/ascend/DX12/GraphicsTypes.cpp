#include "GraphicsTypes.h"
#include "DX12_Helpers.h"
#include <algorithm>

//                                          Fence
//----------------------------------------------------------------------------------------------
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

//                                         Buffer
//----------------------------------------------------------------------------------------------
Buffer::Buffer()
{

}
Buffer::~Buffer()
{

}
void* Buffer::Map()
{
    void* Memory;
    CD3DX12_RANGE range(0, m_bufferSize);
    m_resource->Map(0, &range, &Memory);
    return Memory;
}

void Buffer::Unmap(size_t begin, size_t end)
{
    CD3DX12_RANGE range(begin, std::min(end, m_bufferSize));
    m_resource->Unmap(0, &range);
}

//                                      Upload Buffer
//----------------------------------------------------------------------------------------------
UploadBuffer::UploadBuffer()
{

}

void UploadBuffer::Create(const std::wstring& name, size_t bufferSize)
{
    m_bufferSize = bufferSize;

    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = m_bufferSize;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    VERIFYD3D12RESULT(DX12::Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_resource)));

    m_gpuVirtualAddress = m_resource->GetGPUVirtualAddress();
    m_resource->SetName(name.c_str());
}

//                                      GPU Buffer
//----------------------------------------------------------------------------------------------
GpuBuffer::GpuBuffer()
{
    m_resourceFlags = D3D12_RESOURCE_FLAG_NONE;
}

void GpuBuffer::Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, const UploadBuffer& srcData, uint32_t srcOffset)
{
    m_elementCount = NumElements;
    m_elementSize = ElementSize;
    m_bufferSize = NumElements * ElementSize;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Alignment = 0;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Flags = m_resourceFlags;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.Height = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Width = (UINT64)m_bufferSize;
    m_usageState = D3D12_RESOURCE_STATE_COMMON;

    D3D12_HEAP_PROPERTIES heapProperties;
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    VERIFYD3D12RESULT(DX12::Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, m_usageState, nullptr, IID_PPV_ARGS(&m_resource)));

    m_gpuVirtualAddress = m_resource->GetGPUVirtualAddress();

    m_resource->SetName(name.c_str());

}

void GpuBuffer::Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize)
{

    m_elementCount = NumElements;
    m_elementSize = ElementSize;
    m_bufferSize = NumElements * ElementSize;

    D3D12_RESOURCE_DESC resourceDesc = {};
    resourceDesc.Alignment = 0;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Flags = m_resourceFlags;
    resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
    resourceDesc.Height = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.SampleDesc.Quality = 0;
    resourceDesc.Width = (UINT64)m_bufferSize;
    m_usageState = D3D12_RESOURCE_STATE_COMMON;

    D3D12_HEAP_PROPERTIES heapProperties;
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    VERIFYD3D12RESULT(DX12::Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
        &resourceDesc, m_usageState, nullptr, IID_PPV_ARGS(&m_resource)));

    m_gpuVirtualAddress = m_resource->GetGPUVirtualAddress();

    m_resource->SetName(name.c_str());



}