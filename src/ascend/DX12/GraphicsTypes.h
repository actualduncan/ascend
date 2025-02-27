#pragma once
#include "DX12.h"

//                                          Fence
//----------------------------------------------------------------------------------------------
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

//                                         Buffer
//----------------------------------------------------------------------------------------------
class Buffer
{
public:
	Buffer();
	virtual ~Buffer();


	void* Map();
	void Unmap(size_t begin = 0, size_t end = -1);
	size_t GetBufferSize() const { return m_bufferSize; }
	D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return m_gpuVirtualAddress; }

protected:

	ComPtr<ID3D12Resource> m_resource;
	D3D12_RESOURCE_STATES m_usageState;
	D3D12_RESOURCE_STATES m_transitioningState;

	D3D12_GPU_VIRTUAL_ADDRESS m_gpuVirtualAddress;
	size_t m_bufferSize;
};

//                                      Upload Buffer
//----------------------------------------------------------------------------------------------
class UploadBuffer : public Buffer
{
public:
	UploadBuffer();

	void Create(const std::wstring& name, size_t BufferSize);
	
};

//                                      GPU Buffer
//----------------------------------------------------------------------------------------------
class GpuBuffer : public Buffer
{
public:
	GpuBuffer();

	void Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, const UploadBuffer& srcData, uint32_t srcOffset = 0);
	void Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize);

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV() const { return m_uav; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const { return m_srv; }

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t Offset, uint32_t Size, bool b32Bit = false) const;



private:
	D3D12_CPU_DESCRIPTOR_HANDLE m_uav;
	D3D12_CPU_DESCRIPTOR_HANDLE m_srv;

	uint32_t m_elementCount;
	uint32_t m_elementSize;
	D3D12_RESOURCE_FLAGS m_resourceFlags;
};


inline D3D12_VERTEX_BUFFER_VIEW GpuBuffer::VertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const
{
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	vertexBufferView.BufferLocation = m_gpuVirtualAddress + Offset;
	vertexBufferView.SizeInBytes = Size;
	vertexBufferView.StrideInBytes = Stride;
	return vertexBufferView;
}

inline D3D12_INDEX_BUFFER_VIEW GpuBuffer::IndexBufferView(size_t Offset, uint32_t Size, bool b32Bit) const
{
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
	indexBufferView.BufferLocation = m_gpuVirtualAddress + Offset;
	indexBufferView.Format = b32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
	indexBufferView.SizeInBytes = Size;
	return indexBufferView;
}

//                                      ConstantBuffer
//----------------------------------------------------------------------------------------------
class ConstantBuffer : public Buffer
{
public:
	ConstantBuffer() {};

	void Create(const std::wstring& name, size_t BufferSize);
	void UpdateContents(void* data, size_t size);

private:
	void* BufferStartPtr = nullptr;
};

class DepthStencilBuffer : public Buffer
{
public:
	
	DepthStencilBuffer() {};

	void Create(const std::wstring& name, DXGI_FORMAT format, uint32_t width, uint32_t height);
	DXGI_FORMAT GetFormat() { return m_format; }
private:
	DXGI_FORMAT m_format;
};

//                                      Texture
//----------------------------------------------------------------------------------------------
struct Texture
{
	Texture(int index, std::wstring filename);
	std::string Name;
	std::wstring Filename;

	ComPtr<ID3D12Resource> Resource = nullptr;
	ComPtr<ID3D12Resource> UploadHeap = nullptr;
};