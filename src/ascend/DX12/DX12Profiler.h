#include "DX12.h"

namespace DX12
{
	class Profiler
	{
	public:
		Profiler();
		~Profiler();

		void Init();
		void StartTimestampQuery();
		void EndTimestampQuery();
	private:
		ComPtr<ID3D12QueryHeap> m_timestampQueryHeap;
	};
}