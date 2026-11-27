// Default: embed the implementation for the single-TU test runner.
// For separate compilation, define SZ_TIME_TEST_SEPARATE and link chrono.cc.
#ifndef SZ_TIME_TEST_SEPARATE
#define SZ_TIME_IMPL
#endif
#include "time/chrono.hh"

#include <chrono>
#include <cstdio>

int main()
{
	using namespace sz::time;
	using Clock = std::chrono::steady_clock;

	real_seconds_clock realsecs;
	auto s0 = std::chrono::duration<seconds>(Clock::now().time_since_epoch()).count();
	auto s = realsecs.now();
	auto s1 = std::chrono::duration<seconds>(Clock::now().time_since_epoch()).count();
	if (!(s0 <= s.value() && s.value() <= s1 && realsecs.since(s) >= 0))
		return 1;

	real_tick_clock realticks;
	auto t0 = Clock::now().time_since_epoch().count();
	auto t = realticks.now();
	auto t1 = Clock::now().time_since_epoch().count();
	if (!(t0 <= t.value() && t.value() <= t1 && realticks.since(t) >= 0))
		return 1;

	std::puts("Real clocks: ok");
}
