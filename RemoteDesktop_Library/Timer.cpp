#include "stdafx.h"
#include "Timer.h"

LARGE_INTEGER Timer::m_freq = (QueryPerformanceFrequency(&Timer::m_freq), Timer::m_freq);
LONGLONG Timer::m_overhead = Timer::GetOverhead();