#ifndef CMYTIME_H
#define CMYTIME_H
#include <chrono>

class Timer{
	LARGE_INTEGER m_start;
	LARGE_INTEGER m_stop;
	static LARGE_INTEGER m_freq;
	static LONGLONG m_overhead;
	static LONGLONG GetOverhead(){
		Timer t;
		t.Start();
		t.Stop();
		return t.m_stop.QuadPart - t.m_start.QuadPart;
	}
public:
	explicit Timer(bool s = false){ if (s) Start(); }
	void Start() { QueryPerformanceCounter(&m_start); }
	void Stop() { QueryPerformanceCounter(&m_stop); }
	long long Elapsed_micro() const{ return ((m_stop.QuadPart - m_start.QuadPart - m_overhead) * 1000000.0) / m_freq.QuadPart; }
	long long Elapsed_milli() const{ return ((m_stop.QuadPart - m_start.QuadPart - m_overhead) * 1000.0) / m_freq.QuadPart; }

	template <typename T, typename Traits>
	friend std::basic_ostream<T, Traits>& operator<<(std::basic_ostream<T, Traits>& out, const Timer& timer){ return out << timer.Elapsed().count(); }
};
#endif
