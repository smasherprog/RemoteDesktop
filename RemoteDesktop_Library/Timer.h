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
	long long Elapsed_micro() const{ return (long long)((m_stop.QuadPart - m_start.QuadPart - m_overhead) * 1000000.0) / m_freq.QuadPart; }
	long long Elapsed_milli() const{ return (long long)((m_stop.QuadPart - m_start.QuadPart - m_overhead) * 1000.0) / m_freq.QuadPart; }

	template <typename T, typename Traits>
	friend std::basic_ostream<T, Traits>& operator<<(std::basic_ostream<T, Traits>& out, const Timer& timer){ return out << timer.Elapsed().count(); }
}; 
//On windows the below class will give resolution down to 15 ms. 
class Low_Timer{
	typedef std::chrono::high_resolution_clock high_resolution_clock;
	typedef std::chrono::milliseconds milliseconds;
	high_resolution_clock::time_point _Begin, _End;

public:
	explicit Low_Timer(bool s = false){ if (s) Start(); }
	void Start() { _Begin = high_resolution_clock::now(); }
	void Stop() { _End = high_resolution_clock::now(); }
	long long Elapsed() const{ return std::chrono::duration_cast<milliseconds>(_End - _Begin).count(); }
	template <typename T, typename Traits>
	friend std::basic_ostream<T, Traits>& operator<<(std::basic_ostream<T, Traits>& out, const Low_Timer& timer){ return out << timer.Elapsed().count(); }
};
#endif
