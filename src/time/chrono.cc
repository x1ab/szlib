#include "chrono.hh"

#include <chrono>
#include <limits>

namespace sz::time::impl::realtime {

using NativeRep = std::chrono::steady_clock::rep;
static_assert(std::numeric_limits<NativeRep>::is_integer);
static_assert(std::numeric_limits<NativeRep>::digits <=
              std::numeric_limits<i64>::digits);

seconds read_real_seconds() noexcept
{
	using namespace std::chrono;
	return duration_cast<std::chrono::duration<seconds>>(
		steady_clock::now().time_since_epoch()).count();
}

i64 read_real_ticks() noexcept
{
	return std::chrono::steady_clock::now().time_since_epoch().count();
}

} // namespace sz::time::impl::realtime
