//----------------------------------------------------------------------------
// Optional wrapper to chrono's steady clock
//
// Compile chrono.cc separately, or define SZ_TIME_IMPL in exactly one TU
// before including this header. Other TUs need only this header.

//----------------------------------------------------------------------------

#pragma once

#include "../time.hh"
#include "../lang/atom/int.hh"

namespace sz {
namespace time {

//----------------------------------------------------------------------------
// Basic clocks — reference implementations...
//----------------------------------------------------------------------------
namespace impl {

	namespace realtime {

		seconds read_real_seconds() noexcept;
		atom::unint::i64 read_real_ticks() noexcept;

		struct chrono_realtime_policy : analog_synthetic::policy<seconds> {
			static constexpr bool strictly_monotonic = false; // chrono's steady clock is "just monotonic"
		};

		//!! Alas, the existing wrapping_counter::policy is for unsigned types:
		struct chrono_realticks_policy
		{
			using underlying_type = atom::unint::i64; // Native ticks, not necessarily nanoseconds
			static constexpr bool discrete = true;
			static constexpr bool controlled_wrap = false;
			static constexpr bool strictly_monotonic = false; // chrono's steady clock is "just monotonic"
		};

		// This clock is sourced from std::chrono.
		// (That still means "self-sourcing" in terms of not depending on another sz::clock.)
		template <typename TimePolicy, typename UpstreamClockT = detail::StandaloneClock>
		class chrono_dual_clock : public generic_clock<chrono_dual_clock<TimePolicy, UpstreamClockT>,
		                                               TimePolicy, UpstreamClockT>
		{
		public:
			using time = time<TimePolicy>;

			void advance() noexcept {}
			// No-op: there's no internal state to advance,
			// read() will always get the latest time instead.

			time read() noexcept
			{
				if constexpr (time::policy::discrete)
					return time(read_real_ticks());
				else
					return time(read_real_seconds());
			}
		};


	} // namespace realtime

} // namespace impl

//----------------------------------------------------------------------------
// Exported vocabulary aliases for the chrono steady-clock wrapper
//----------------------------------------------------------------------------
using real_tick_clock    = impl::realtime::chrono_dual_clock<impl::realtime::chrono_realticks_policy>;
using real_seconds_clock = impl::realtime::chrono_dual_clock<impl::realtime::chrono_realtime_policy>;
	static_assert(clock_c<real_tick_clock>);
	static_assert(clock_c<real_seconds_clock>);

} // namespace time
} // namespace sz

#ifdef SZ_TIME_IMPL
#include "chrono.cc"
#endif
