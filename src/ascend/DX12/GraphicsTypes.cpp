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

//                                      Constant Buffer
//----------------------------------------------------------------------------------------------

void ConstantBuffer::Create(const std::wstring& name, size_t bufferSize)
{
    m_bufferSize = bufferSize;

    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_bufferSize);

    VERIFYD3D12RESULT(DX12::Device->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_resource)));
    m_resource->SetName(name.c_str());
    // Describe and create a constant buffer view.
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = m_resource->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = m_bufferSize;

    DescriptorAllocation alloc = DX12::UAVDescriptorHeap.AllocateDescriptor();
    resourceIdx = alloc.Index;

    DX12::Device->CreateConstantBufferView(&cbvDesc, alloc.Handle);
    m_gpuVirtualAddress = m_resource->GetGPUVirtualAddress();

    BufferStartPtr = Map();
}

void ConstantBuffer::UpdateContents(void* data, size_t size)
{
    memcpy(BufferStartPtr, data, size);
}

void DepthStencilBuffer::Create(const std::wstring& name, DXGI_FORMAT format, uint32_t width, uint32_t height)
{
    m_format = format;
    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = width;
    depthStencilDesc.Height = height;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = m_format;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = m_format;
    optClear.DepthStencil.Depth = 0.0f;
    optClear.DepthStencil.Stencil = 0;

    // window size properties therefore should be in swapchain info? Or create depth buffer struct
    auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    VERIFYD3D12RESULT(DX12::Device->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &depthStencilDesc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(&m_resource)));
    m_resource->SetName(name.c_str());

    DescriptorAllocation alloc = DX12::DSVDescriptorHeap.AllocateDescriptor();
    resourceIdx = alloc.Index;
    DX12::Device->CreateDepthStencilView(m_resource.Get(), nullptr, alloc.Handle);

    DX12::TransitionResource(DX12::GraphicsCmdList.Get(), m_resource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE, 0);
}

void DescriptorHeap::Init(uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE heapType, bool shaderVisible)
{
    NumDescriptors = numDescriptors;
    HeapType = heapType;
    ShaderVisible = shaderVisible;

    if(heapType == D3D12_DESCRIPTOR_HEAP_TYPE_RTV || heapType == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
        ShaderVisible = false;

    NumHeaps = ShaderVisible ? DX12::RenderLatency : 1;

    DeadList = std::vector<uint32_t>(NumDescriptors);
    for(uint32_t i = 0; i < NumDescriptors; ++i)
    {
        DeadList[i] = uint32_t(i);
    }

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = NumDescriptors;
    heapDesc.Type = HeapType;
    heapDesc.Flags = ShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    VERIFYD3D12RESULT(DX12::Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&Heap)));
    CPUStart = Heap->GetCPUDescriptorHandleForHeapStart();
    if (ShaderVisible)
        GPUStart = Heap->GetGPUDescriptorHandleForHeapStart();
    DescriptorSize = DX12::Device->GetDescriptorHandleIncrementSize(HeapType);

}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::CPUHandleFromIndex(uint32_t descriptorIdx, uint32_t heapIdx) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = CPUStart;
    handle.ptr += descriptorIdx * DescriptorSize;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GPUHandleFromIndex(uint32_t descriptorIdx, uint32_t heapIdx) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = GPUStart;
    handle.ptr += descriptorIdx * DescriptorSize;
    return handle;
}

DescriptorAllocation DescriptorHeap::AllocateDescriptor()
{
    // DXR Path tracer makes use of SRWLockExclusive??
    // maybe uses a render thread for commands?? an allocations?? - pretty feasible that it does this to be fair.
    uint32_t idx = DeadList[AllocatedDescriptors];
    ++AllocatedDescriptors;

    DescriptorAllocation alloc;
    alloc.Index = idx;


    alloc.Handle = CPUStart;
    alloc.Handle.ptr += idx * DescriptorSize;
    alloc.GPUHandle = GPUStart;
    alloc.GPUHandle.ptr += idx * DescriptorSize;
    return alloc;
}