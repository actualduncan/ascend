#include "DX12/DX12.h"
struct GBuffer
{
	ComPtr<ID3D12Resource> m_materialId;
	ComPtr<ID3D12Resource> m_worldSpacePosition;
	ComPtr<ID3D12Resource> m_normalMap;
};