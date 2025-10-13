//----------------------------------------------------------------------------
// Optional wrapper to chrono's steady clock
//
// (Placed into a separate header to isolate the <chrono> tsunami.)
//
//!! Split the chrono deps. out to a .cpp that can be made to be directly
//!! included via a #define!

//----------------------------------------------------------------------------

#pragma once

#include "../time.hh"
#include <chrono>

namespace sz {
namespace time {

//----------------------------------------------------------------------------
// Basic clocks — reference implementations...
//----------------------------------------------------------------------------
namespace impl {

	namespace realtime {

		struct chrono_realtime_policy : analog_synthetic::policy<seconds> {
			static constexpr bool strictly_monotonic = false; // chrono's steady clock is "just monotonic"
		};

		//!! Alas, the existing wrapping_counter::policy is for unsigned types:
		//!!using chrono_tick_type = std::chrono::steady_clock::rep; // decltype(...count())
		//!!struct chrono_realticks_policy : wrapping_counter::policy<chrono_tick_type> { ... };
		//!! ...So, instead:
		struct chrono_realticks_policy
		{
			using underlying_type = std::chrono::steady_clock::rep; // decltype(...count())
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
				auto chrono_time = std::chrono::steady_clock::now().time_since_epoch();

				if constexpr (time::policy::discrete)
				{
					return time(chrono_time.count());
				}
				else // Convert to seconds...
				{
					// Duration type to represent elapsed seconds in terms of our
					// clock::time's underlying type (i.e. sz::seconds, a.k.a. double):
					using TimeSpan = std::chrono::duration<typename time::underlying_type>;
					// Convert the reported time to that duration type:
					auto duration_in_seconds = std::chrono::duration_cast<TimeSpan>(chrono_time);
					// The .count() of this new duration object is our value in seconds:
					return time(duration_in_seconds.count());

	/*!! OLD "guessware":
					using Secs = TimePolicy::underlying_type; // Don't assume `seconds` directly here!
					//!! static_assert(std::is_asme<Secs, seconds>);

					//!! FIX: ACTUALLY VERIFY, IF POSSIBLE!
					//!! Assuming chrono_tick_count to be nanoseconds, which is typical for steady_clock...
					auto s = Secs(chrono_time.count()) / Secs(1'000'000'000.0);
					return time(s);
	!!*/
				}
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
