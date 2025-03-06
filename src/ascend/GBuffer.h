#include "DX12/DX12.h"
struct GBuffer
{
	ComPtr<ID3D12Resource> m_materialId;
	D3D12_CPU_DESCRIPTOR_HANDLE materialHandle;

	ComPtr<ID3D12Resource> m_worldSpacePosition;
	D3D12_CPU_DESCRIPTOR_HANDLE worldSpaceHandle;

	ComPtr<ID3D12Resource> m_normalMap;
	D3D12_CPU_DESCRIPTOR_HANDLE normalMapHandle;

};