#include "DX12Profiler.h"
#include "DX12_Helpers.h"
namespace DX12
{
	Profiler::Profiler()
	{

	}

	Profiler::~Profiler()
	{

	}

	void Profiler::Init()
	{
		D3D12_QUERY_HEAP_DESC heapDesc = {};
		heapDesc.Count = 1;
		heapDesc.NodeMask = NULL;
		heapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;

		VERIFYD3D12RESULT(DX12::Device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&m_timestampQueryHeap)));
		m_timestampQueryHeap->SetName(L"Timestamp Query Heap");
	}

	void Profiler::StartTimestampQuery()
	{
		DX12::GraphicsCmdList->BeginQuery(m_timestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
	}


	void Profiler::EndTimestampQuery()
	{
		DX12::GraphicsCmdList->EndQuery(m_timestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
	}
}