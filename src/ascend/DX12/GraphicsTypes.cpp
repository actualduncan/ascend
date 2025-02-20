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
    DX12::Device->CreateConstantBufferView(&cbvDesc, DX12::UAVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    m_gpuVirtualAddress = m_resource->GetGPUVirtualAddress();

    BufferStartPtr = Map();
}

void ConstantBuffer::UpdateContents(void* data, size_t size)
{
    memcpy(BufferStartPtr, data, size);
}

void DepthStencilBuffer::Create(const std::wstring& name, uint32_t width, uint32_t height)
{
    D3D12_RESOURCE_DESC depthStencilDesc = {};
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = width;
    depthStencilDesc.Height = height;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear = {};
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optClear.DepthStencil.Depth = 1.0f;
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
    DX12::Device->CreateDepthStencilView(m_resource.Get(), nullptr, DX12::DSVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    DX12::TransitionResource(DX12::GraphicsCmdList.Get(), m_resource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE, 0);
}
#include "DDSTextureLoader12.h"
Texture::Texture(int index, std::wstring filename)
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(DX12::UAVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), index + 2, DX12::UAVDescriptorSize);
    std::unique_ptr<uint8_t[]> ddsData;
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    VERIFYD3D12RESULT(LoadDDSTextureFromFile(DX12::Device.Get(), filename.c_str(), Resource.ReleaseAndGetAddressOf(), ddsData, subresources));

    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(Resource.Get(), 0,
        static_cast<UINT>(subresources.size()));

    // Create the GPU upload buffer.
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);

    auto desc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);


    VERIFYD3D12RESULT(
        DX12::Device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(UploadHeap.GetAddressOf())));

    UpdateSubresources(DX12::GraphicsCmdList.Get(), Resource.Get(), UploadHeap.Get(),
        0, 0, static_cast<UINT>(subresources.size()), subresources.data());

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(Resource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    DX12::GraphicsCmdList->ResourceBarrier(1, &barrier);



    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = Resource->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = Resource->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    DX12::Device->CreateShaderResourceView(Resource.Get(), &srvDesc, hDescriptor);
}