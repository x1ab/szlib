//#define DEBUG
//#define CHRONO
#define IOS

#include "time.hh"
#ifdef CHRONO
# include "time/chrono.hh"
#endif

#include <cstdint>
#include <cassert>


#ifdef IOS
#	include <iostream>
		using std::cerr, std::cout, std::endl;
#	define DBG cout <<std::boolalpha<<
#	define ERR cerr <<std::boolalpha<<
#else
	struct NullStream { template <typename T> auto operator<<(const T&) {return *this;} };
#	define DBG NullStream() <<
#	define ERR NullStream() <<
#endif


//----------------------------------------------------------------------------
using namespace sz::time;

tick_clock<> g_source_ticks;
tick_clock  g_driven_tick_clock(g_source_ticks);

model_clock<> g_source_analog;
model_clock g_driven_analog_clock(g_source_analog);

//----------------------------------------------------------------------------
int main()
{
	//====================================================================
	// Tick counter...

//	sz::tick_clock<int> ticks_i; // Should fail to compile!

	tick_clock<uint64_t> ticks;

#ifndef _MSC_VER // error C2641: cannot deduce template arguments
	tick_clock   ticks_default; // unsigned
#else
	tick_clock<> ticks_default; // unsigned
#endif

	DBG "Tick clock now(): " << ticks.now().value() << "\n";
	DBG "Tick clock now(): " << ticks.now().value() << "\n";
	ticks.advance(10);
	DBG "Tick clock now() after advance(10): " << ticks.now().value() << "\n";
	DBG "Tick clock is strictly_monotonic (i.e. now always advances) ? "
		<< decltype(ticks)::time::policy::strictly_monotonic << "\n";
//!! Well, but... this is still accepted!...:
	ticks.advance(-10);
	DBG "Tick clock now() after advance(-10): " << ticks.now().value() << "\n";

#ifdef DEBUG
	ticks = ticks + 10;
#else
	// Shouldn't compile in non-DEBUG!
	//ticks = ticks + 10;
#endif

	auto t0 = ticks.now();
	auto t1 = ticks.now();

//	t1 > t0; // Should NOT compile with wrapping time!

	auto delta = t1 - t0;
//	assert(ticks.span(t0, t1) == t1 - t0);
	DBG "Delta ticks: " << delta << "\n"; // 11

	// Wrap simulation
#ifdef DEBUG
	ticks = ~0ull - 1; // u64 is ull in MSVC and GCC, but is that just a coincidence?...
#else
	ERR "- Warning: The wraparound test requires compiling with DEBUG!\n";
#endif
	t0 = ticks.now(); // -1
	t1 = ticks.now(); // 0
	auto delta_wrap = t1 - t0; // 1
//	assert(ticks.span(t0, t1) == t1 - t0);
	DBG "Delta (of "<< t0.value() <<", "<< t1.value() <<") with wrap: "
		<< delta_wrap << "\n"; // 1

	//====================================================================
	// "Analog" sim timer...

#ifndef _MSC_VER // error C2641: cannot deduce template arguments
	model_clock   wclock_default;
#else
	model_clock<> wclock_default;
#endif
	model_clock<float> wclock;

	// Implicit conv. SHOULD compile with wrapping time!
	DBG "Auto-advance sim clock now: " << wclock.now() << "\n";
	DBG "Auto-advance sim clock now: " << wclock.now() << "\n";

	//!!assert(wclock.now() <= wclock.now()); //!! Well, super nifty, but UB! And GCC does crash on it  (unlike MSVC and Clang)!
	{ auto _t1 = wclock.now(), _t2 = wclock.now(); assert(_t1 <= _t2); }
	if constexpr (model_clock<float>::time::policy::strictly_monotonic) {
		{ auto _t1 = wclock.now(), _t2 = wclock.now(); assert(_t1 < _t2); }
	} else {
		DBG "- Note: The tested analog clock is not defined to be strictly monotonic.\n";
	}


#ifdef CHRONO
	//====================================================================
	// Realtime (chrono wrappers)...

	DBG "Real time relative to an unspecified base...\n";

	// Seconds...
	real_seconds_clock realsecs;
	auto t0_realsecs = realsecs.now();
	DBG "\tseconds since "<< t0_realsecs <<": "<< realsecs.since(t0_realsecs) <<"\n";

	// Ticks...
	real_tick_clock realticks;
	auto t0_realticks = realticks.now();
	// long long raw_ticks = t0_realticks; // Fails to compile (explicit conversion)
	DBG "\tticks since "<< t0_realticks <<": "<< realticks.since(t0_realticks) <<"\n";

/*
	seconds secs = t0_realsecs; // Compiles! Implicit conversion is allowed by the policy.
	auto realticks_tval = t0_realticks.value(); // OK
	DBG "\tanchor (ticks): " << realticks_tval << "\n";
	DBG "\tanchor (s):     " << secs << "\n";
*/
#endif

	//====================================================================
	// Chaining...

#ifdef ____not_yet____CHRONO
	real_tick_clock source_s_clock;
	tick_clock<	real_tick_clock::time::underlying_type,
			decltype(source_s_clock),
			impl::Scale_x2>
		slave_wclock_f_decl(source_s_clock, impl::Scale_x2{});
#else
	model_clock<double> source_s_clock;
	model_clock<	model_clock<double>::time::underlying_type,
			decltype(source_s_clock),
			impl::Scale_x2>
		slave_wclock_f_decl(source_s_clock, impl::Scale_x2{});
#endif

	auto                                          slave_wclock_auto(source_s_clock);
	model_clock                                   slave_wclock(source_s_clock);
	model_clock<double, decltype(source_s_clock)> slave_wclock_d_decl(source_s_clock);
//!! CAN'T YET...:
////	model_clock<decltype(source_s_clock)> slave_wclock_decl(source_s_clock);
////	model_clock<float>              slave_wclock_f(source_s_clock);

	DBG "Linked clock now(): "<< slave_wclock_f_decl.now() <<"\n";
	DBG "Linked clock now(): "<< slave_wclock_f_decl.now() <<"\n";
	DBG " - from source now(): "<< source_s_clock.now() <<"\n";


#ifdef CHRONO
	real_tick_clock            source_tick_clock;
#else
	tick_clock<unsigned short> source_tick_clock;
#endif

	auto                                              slave_tclock_auto(source_tick_clock);
	tick_clock                                        slave_tclock(source_tick_clock);
	tick_clock<unsigned, decltype(source_tick_clock)> slave_tclock_u_decl(source_tick_clock);
//!! CAN'T YET...:
////	tick_clock<decltype(source_tick_clock)>            slave_tclock_decl(source_tick_clock);
////	tick_clock<unsigned>                               slave_tclock_u(source_tick_clock);
}
