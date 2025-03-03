#include "Textures.h"
#include "DDSTextureLoader12.h"
#include "DX12_Helpers.h"

void LoadTextureFromFile(Texture& texture, const wchar_t* filePath)
{
   // indexing issues will arise, 
   // need to interface with uav descriptor heap to actually output a valid index rather than setting to input index + magic number!!!
    std::unique_ptr<uint8_t[]> ddsData;
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    VERIFYD3D12RESULT(LoadDDSTextureFromFile(DX12::Device.Get(), filePath, texture.Resource.ReleaseAndGetAddressOf(), ddsData, subresources));

    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Resource.Get(), 0,
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
            IID_PPV_ARGS(texture.UploadHeap.GetAddressOf())));

    UpdateSubresources(DX12::GraphicsCmdList.Get(), texture.Resource.Get(), texture.UploadHeap.Get(),
        0, 0, static_cast<UINT>(subresources.size()), subresources.data());

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Resource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    DX12::GraphicsCmdList->ResourceBarrier(1, &barrier);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = texture.Resource->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = texture.Resource->GetDesc().MipLevels;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    DescriptorAllocation alloc = DX12::UAVDescriptorHeap.AllocateDescriptor();
    texture.ResourceIdx = alloc.Index;
    DX12::Device->CreateShaderResourceView(texture.Resource.Get(), &srvDesc, alloc.Handle);
   
    
}