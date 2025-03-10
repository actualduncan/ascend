#pragma once
#include "PCH.h"
#include <map>
namespace DX12
{
	class Profiler
	{
	public:
		Profiler();
		~Profiler();

		void Init(DXGI_ADAPTER_DESC1 desc);
		void StartTimestampQuery(std::string name);
		void EndTimestampQuery(std::string name);
		void CollectTimingData();
		double GetFrametime(std::string name);

		void StartMemoryQuery(std::string name);
		void EndMemoryQuery(std::string name);
		double GetMemoryUsage(std::string name);
	private:
		ComPtr<ID3D12QueryHeap> m_timestampQueryHeap;
		ComPtr<ID3D12Resource> m_resultBuffer;
		ComPtr<IDXGIAdapter3> m_adapter;
		uint64_t m_timestampFrequency;
		std::map<std::string, uint32_t> m_timestampMarkers;
		std::map<std::string, uint32_t> m_memoryMarkers;
		UINT64* data;
		int profileHeaps = 0;

		SIZE_T systemMem;
		SIZE_T videoMem;
		wchar_t* adapterinfo;
	};

}