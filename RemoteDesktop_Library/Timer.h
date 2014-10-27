#ifndef CMYTIME_H
#define CMYTIME_H
#include <chrono>

class Timer{
	typedef std::chrono::high_resolution_clock high_resolution_clock;
	typedef std::chrono::milliseconds milliseconds;
	high_resolution_clock::time_point _Begin, _End;

public:
	explicit Timer(bool s = false){ if (s) Start(); }
	void Start() { _Begin = high_resolution_clock::now(); }
	void Stop() { _End = high_resolution_clock::now(); }
	long long Elapsed() const{ return std::chrono::duration_cast<milliseconds>(_End - _Begin).count(); }
	template <typename T, typename Traits>
	friend std::basic_ostream<T, Traits>& operator<<(std::basic_ostream<T, Traits>& out, const Timer& timer){ return out << timer.Elapsed().count(); }
};
#endif
