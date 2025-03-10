#include "DX12Profiler.h"
#include "DX12_Helpers.h"
#include "DX12.h"
#define TO_MS 1000.f
namespace DX12
{
	Profiler::Profiler()
	{

	}

	Profiler::~Profiler()
	{

	}

	void Profiler::Init(DXGI_ADAPTER_DESC1 desc)
	{
		systemMem = desc.DedicatedSystemMemory;
		videoMem = desc.DedicatedVideoMemory;
		adapterinfo = desc.Description;

		// for memory usage queries
		VERIFYD3D12RESULT(Factory->EnumAdapterByLuid(desc.AdapterLuid, IID_PPV_ARGS(&m_adapter)));

		D3D12_QUERY_HEAP_DESC heapDesc = {};
		heapDesc.Count = 6;
		heapDesc.NodeMask = NULL;
		heapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;

		VERIFYD3D12RESULT(DX12::Device->CreateQueryHeap(&heapDesc, IID_PPV_ARGS(&m_timestampQueryHeap)));
		m_timestampQueryHeap->SetName(L"Timestamp Query Heap");
		auto readback = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

		auto timestamp = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT64) * heapDesc.Count);
		VERIFYD3D12RESULT(DX12::Device->CreateCommittedResource(
			&readback, D3D12_HEAP_FLAG_NONE, &timestamp, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_resultBuffer)));
		DX12::GraphicsQueue->GetTimestampFrequency(&m_timestampFrequency);

	}

	void Profiler::StartTimestampQuery(std::string name)
	{
		std::string begin = name + "begin";

		if (!m_timestampMarkers.contains(begin))
		{
			std::string end = name + "end";
			m_timestampMarkers.emplace(begin, profileHeaps);
			m_timestampMarkers.emplace(end, profileHeaps + 1);
			profileHeaps += 2;
		}

		DX12::GraphicsCmdList->EndQuery(m_timestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, m_timestampMarkers[begin]);
	}


	void Profiler::EndTimestampQuery(std::string name)
	{
		std::string end = name + "end";
		if (!m_timestampMarkers.contains(end))
		{
			return;
		}

		DX12::GraphicsCmdList->EndQuery(m_timestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, m_timestampMarkers[end]);
	}

	double Profiler::GetFrametime(std::string name)
	{
		std::string begin = name + "begin";
		std::string end = name + "end";
		if (!m_timestampMarkers.contains(begin))
		{
			return 0.0;
		}

		return ((((double)data[m_timestampMarkers[end]]) / (double)m_timestampFrequency) - (((double)data[m_timestampMarkers[begin]]) / (double)m_timestampFrequency)) * TO_MS;
	}

	void Profiler::CollectTimingData()
	{

		DX12::GraphicsCmdList->ResolveQueryData(m_timestampQueryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, 6, m_resultBuffer.Get(), 0);

		D3D12_RANGE readbackRange{ 0, sizeof(UINT64) * 6 };
		data = {};
		m_resultBuffer->Map(0, &readbackRange, reinterpret_cast<void**>(&data));

		D3D12_RANGE emptyRange{ 0, 0 };
		m_resultBuffer->Unmap
		(
			0,
			&emptyRange
		);

	}

	void Profiler::StartMemoryQuery(std::string name)
	{
		std::string begin = name + "begin";
		if (!m_memoryMarkers.contains(begin))
		{
			std::string end = name + "end";
			m_memoryMarkers.emplace(begin, profileHeaps);
			m_memoryMarkers.emplace(end, profileHeaps + 1);
		}
		// query current memory usage
		DXGI_QUERY_VIDEO_MEMORY_INFO mInfo = {};
		m_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &mInfo);
		m_memoryMarkers[begin] = mInfo.CurrentUsage;
	}

	void Profiler::EndMemoryQuery(std::string name)
	{
		std::string end = name + "end";
		if (!m_memoryMarkers.contains(end))
		{
			return;
		}
		// query current memory usage
		DXGI_QUERY_VIDEO_MEMORY_INFO mInfo = {};
		m_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &mInfo);
		m_memoryMarkers[end] = mInfo.CurrentUsage;
	}

	double Profiler::GetMemoryUsage(std::string name)
	{
		std::string begin = name + "begin";
		std::string end = name + "end";
		if (!m_memoryMarkers.contains(begin))
		{
			return 0.0;
		}

		return ((double)m_memoryMarkers[end] - (double)m_memoryMarkers[begin]) / 1024 / 1024;
	}

	double Profiler::GetCurrentTotalMemoryUsage()
	{
		DXGI_QUERY_VIDEO_MEMORY_INFO mInfo = {};
		m_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &mInfo);
		return (double)mInfo.CurrentUsage / 1024 / 1024;
	}
}