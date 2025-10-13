// 0.30.1

/****************************************************************************
  Uniform discrete/continuous time/clock abstraction with polled, chainable
  clocks and policy-based time characteristics

  Unlike std::chrono's focus of being a stateless observer of absolute time,
  this design is biased toward timed event systems, simulations, etc., where
  the notion of time is decoupled from wall clocks, to facilitate arbitrary
  time manipulations (warping, reversal, scaling, etc.), deterministic
  (repeatable) synthetic timelines, linking virtual clocks, etc., while
  still allowing to model (measure) real (wall-clock) time, too.

  In this model a clock is the master time source for a "time domain" (or
  timeline) that's defined by the very existene of that clock instance.
  (Note: this also implies that `now()` is not a static free function, but
  always belongs to a clock *instance*. *Within* a time domain, there could,
  of course, still be a static "local" now() helper/wrapper, defined by
  client code; but that's out of scope here, at this (low) level of the
  abstraction.)

  A clock::time type defined by a clock is a uniform ordering scalar in that
  time domain.

  Time domains can be composed into hierarchies: clocks may depend on other
  clocks (as external time sources), defining local, dependent (downstream)
  time domains with different time types (and corresponding rules/policies).

!! REFINE!...

  ----------------------------------------------------------------------------
  Semantics of the `controlled_wrap` policy (for tick counters):

  GUARANTEE:  `.since(earlier)` (and `operator-`) returns the correct time
              span as long as `earlier` was NO MORE than than 1 cycle (i.e.
              the full range of the underlying type) ago.
              This works correctly across 1 wraparound boundary, making the
              whole range of the (unsigned) underlying type is available.

  CONSTRAINT: You MUST NOT measure intervals longer than one full cycle
              (e.g., for uint32_t ticks, intervals must be < 2^32 ticks).

              The arguments to operator-) MUST be in the correct temporal
	      order. (Otherwise the result is considered invalid.)

              The ordering comparison ops. (<, >, <=, >=) are DISABLED,
              because they can't guarantee correct results across wraparound
              boundaries. Use .since() to get a duration value instead,
              which can then be compared freely.

  Example with uint8_t (wraps at 256):
    time t0(255), t1(1);  // Simulate a wrapped value in t1...
    auto elapsed = t1.since(t0);  // Returns 2.
    // t1 > t0  // DISABLED - would give wrong answer!

  This model trades direct ordering and explicit wraparound checks
  for correct interval measurement over the full range of an unsigned
  type and maximal hot-loop performance, in accordance with the indended
  use case of high-frequency measurements across time spans that are
  known to be short enogh to fit the range of the chosen time type.

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
TODO:
- Split strictly_monotonic into strictly_monotonic_read (i.e. now() always
  advances), and a decoupled trait to allow negative x for advance(x)
 ****************************************************************************/

#pragma once

//#include <type_traits> // No! Keep this core API header <std>-less!

namespace sz {
namespace time {

//----------------------------------------------------------------------------
// Legacy "lingua-franca" wall-clock time type
using seconds = double;

//----------------------------------------------------------------------------
// Requirements for underlying types used for "time" types

template<typename T>
concept discrete_time_type_c = requires(T t)
// Note: Doesn't enforce wrapping semantics, only unsignedness!
//       E.g. a saturating T++ that stops at T::MAX would pass silently!
{
	requires ( T(T(0) - T(1))) > T(0); // unsigned
		//! The reasons for complex form above:
		//! - `T(0) - T(1)` instead of just `T(0) - 1` doesn't force conversion from non-T.
		//! - `T(0) - T(1)` doesn't force a custom unsigned T to accept negative literals.
		//! - The outer `T(...)` cast is to work around the wicked C landmine of promoting
		//!   small unsigned builtin types to `int`(!!!), instead of `unsigned`!... :-o
		//!   (So, `unsigned short` doesn't inexplicably fail the concept check any more... ;) )
		//! - The outer `T(...)` cast also shepherds the result of the subtraction back to T,
		//!   in case it returned some non-T diff type... The conversion back to T there
		//!   is a bit of an arbitrary requirement here, but hopefully acceptable.
		//! Note: the comparison above also requires an op> for T...

	t - t;                      // can subtract
	++t;                        // can increment
};

template<typename T>
concept analog_synthetic_time_type_c = requires(T t)
{
	t - t;                      // can subtract
};


//----------------------------------------------------------------------------
// Requirements for a "clock" type
template<typename C>
concept clock_c = requires(C clock) {
	typename C::time;
	clock.now();
	//!! Would need std::...:
	//!!{ clock.now() } -> std::same_as<typename C::time>;
};


//----------------------------------------------------------------------------
// Generic time types: time (as time point), duration (as time span)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Time span: difference between two time points
//
//!! Can't create it from time itself, of even TimePolicy (instead of NumT directly),
//!! because duration<> is already used inside TimePolicy! :-/
//
//!! NOT TRUE YET:
//!! Auto-converts to built-in numeric types, but won't accidentally materialize
//!! (by auto-converting) *from* numbers.
template <typename NumT>
using duration = decltype(NumT(0) - NumT(0));
/*!!??
template<typename NumT>
class duration
{
	//!! Can't create it from TimePolicy (instead of NumT directly), because duration<> is already used inside TimePolicy! :-/
	//!!using NumT = TimePolicy::underlying_type;
	using DiffT = decltype(NumT(0) - NumT(0));
	DiffT value_;
public:
	explicit constexpr duration(NumT t = 0) noexcept : value_(t) {} // Constexpr is crucial here for duration literals!
	operator DiffT() const noexcept { return value_; }
	//!! ...
};
??!!*/

//----------------------------------------------------------------------------
// The actual `time` type, parametrized by a policy struct
template<typename TimePolicy>
class time
{
	using SelfT = time<TimePolicy>;
public:
	using policy = TimePolicy;
	using underlying_type = policy::underlying_type;

	explicit time(underlying_type v = 0) noexcept : value_(v) {}
	explicit(policy::controlled_wrap) operator underlying_type() const noexcept { return value_; }

	SelfT& operator = (underlying_type t) { value_ = t; return *this; }

	underlying_type value() const                  { return value_; }
	underlying_type& value_ref()                   { return value_; }
	const underlying_type& const_value_ref() const { return (const underlying_type&) value_; }

	auto since(SelfT earlier) const noexcept
	{
		if constexpr (policy::controlled_wrap) {
			// Implicit modulo-wrap for built-in unsigned types.
			// The same is expected from user-defined ones on subtraction...
			// (See more at discrete_time_type_c!)
			using T = underlying_type;
			return T(T(value_) - T(earlier.value_)); // Again, see why at discrete_time_type_c!
		} else {
			// Otherwise (most likely with signed types) the result type is "unspecified" (auto):
			return value_ - earlier.value_;
		}
	}
	auto operator - (SelfT earlier_rhs) const noexcept { return since(earlier_rhs); }

	bool operator == (SelfT o) const { return value_ == o.value_; }
	bool operator != (SelfT o) const { return value_ != o.value_; }

	// Disable ordering comparisons for time types not known to wrap safely:
	//!!C++: No `delete(flag)`, similarly to `explicit(flag)`... :-/ (I wager never will be,
	//!! despite could be just fine, esp. as there's now delete("reason") in C++26...)
	bool operator <  (SelfT o) const requires (!policy::controlled_wrap) { return value_ <  o.value_; }
	bool operator <= (SelfT o) const requires (!policy::controlled_wrap) { return value_ <= o.value_; }
	bool operator >  (SelfT o) const requires (!policy::controlled_wrap) { return value_ >  o.value_; }
	bool operator >= (SelfT o) const requires (!policy::controlled_wrap) { return value_ >= o.value_; }

private:
	underlying_type value_{};
};


//----------------------------------------------------------------------------
//!!?? Where to put these?...
struct Identity { constexpr auto operator()(auto t) const noexcept { return t; } };
//!! See also impl::Scale_x2...


//----------------------------------------------------------------------------
namespace detail {
	struct StandaloneClock {}; // Tag struct for type dispatching...

	// Hack to get the "Planck unit" of T, without including <numeric_limits>
	// (Basically mimicking what std::numeric_limits::epsilon does, AFAIK.)
	template<typename T> consteval T epsilon() {
		T one = T(1);
		T eps = one;
		while (one + eps > one) eps *= T(0.5);
		return eps * T(2);  // Return the last value that actually made a difference
	}
}

//----------------------------------------------------------------------------
// Generic clock (as a CRTP base)
//----------------------------------------------------------------------------
template<class Derived, typename TimePolicy> class generic_clock_common_base
{public:
//!!	using time = time<TimePolicy>; //!!C++: No point, won't be inherited!... :-/
	using underlying_type = typename TimePolicy::underlying_type;
#ifdef DEBUG
	operator underlying_type() { return underlying_type(static_cast<Derived*>(this)->read()); }
#endif
};

//----------------------------------------------------------------------------
// Chaining clocks sourced from another (upstream) clock instance:
template<class Derived,
	typename TimePolicy,
	class UpstreamClockT, //!!?? = detail::StandaloneClock
	typename TransformT = Identity> //!!?? Is this default ever used/needed?
class generic_clock
	: public generic_clock_common_base<Derived, TimePolicy>
{
public:
	using time = time<TimePolicy>;
//	using underlying_type = typename time::underlying_type;

	generic_clock() = default;
	//!! Copy/move implicitly enabled!... (Right?...)

	explicit generic_clock(UpstreamClockT& source, TransformT xform = TransformT{})
		: source_(&source), transform_(xform) {}

	time now() { return time(transform_(source_->now().value())); }

	auto since(time earlier) noexcept { return now().since(earlier); }

protected:
/*
#ifndef _MSC_VER
	[[no_unique_address]]
#else
	[[msvc::no_unique_address]]
#endif
*/	UpstreamClockT* source_ = nullptr; // No memory used if self-sourced (i.e. UpstreamClockT is empty)
	TransformT transform_ = TransformT{};
};

//----------------------------------------------------------------------------
// Part. spec. version for self-sourced clocks:
template<class Derived, typename TimePolicy>
class generic_clock<Derived, TimePolicy, detail::StandaloneClock>
	: public generic_clock_common_base<Derived, TimePolicy>
{
public:
	using time = time<TimePolicy>;
//	using underlying_type = typename time::underlying_type;

	generic_clock() = default;

	time now() noexcept
	{
		static_cast<Derived*>(this)->advance(); //!!?? Or update last? Shouldn' matter much in the current design.
		return static_cast<Derived*>(this)->read(); //!!?? Or update last? Shouldn' matter much in the current design.
	}

	auto since(time earlier) noexcept { return now().since(earlier); }
};


//----------------------------------------------------------------------------
// Basic clocks (as reference implementations)...
//----------------------------------------------------------------------------
namespace impl
{
//----------------------------------------------------------------------------
// C++ boilerplate: Clock "anchor" class generator macro
// These classes are needed ONLY for the deduction guides that allow writing
// `clock_type_alias myclock;` instead of `clock_type_alias<> myclock;`... :-o :)
#define _SZ_CLOCK_CLASS_BEGIN(_ClockName_, _DefaultNumT_) \
	template<typename NumT, typename UpstreamClockT, typename TransformT> \
	class _ClockName_ : public synthetic_clock<_ClockName_<NumT, UpstreamClockT, TransformT>, policy<NumT>, UpstreamClockT, TransformT> \
	{             using base = synthetic_clock<_ClockName_<NumT, UpstreamClockT, TransformT>, policy<NumT>, UpstreamClockT, TransformT>; \
	public: \
		using time = time<policy<NumT>>; \
		using base::base; \
		/*!!C++: Explicit forwarding ctor required for CTAD...: !!*/ \
		explicit _ClockName_(UpstreamClockT& src, TransformT tr = TransformT{}) : base(src, tr) {} \

#define _SZ_CLOCK_CLASS_END };

//----------------------------------------------------------------------------
// C++ boilerplate: Deduction guide generator macro (for the clock "anchor" classes)
// Mostly to resolve chained clock types, but also to allow writing `alias`
// insteead of `alias<>` in important lexical contexts.
// (Note: the default template args are required (only) for Clang, interestingly.)
#define _SZ_CLOCK_DEDUCTION_GUIDES(_ClockName_, _DefaultNumT_) \
	template<typename NumT = _DefaultNumT_, typename UpstreamClockT = detail::StandaloneClock, typename TransformT = Identity> \
	_ClockName_() -> _ClockName_<NumT, UpstreamClockT, Identity>; \
	template<typename NumT = _DefaultNumT_, typename UpstreamClockT = detail::StandaloneClock, typename TransformT = Identity> \
	_ClockName_(UpstreamClockT&) -> _ClockName_<NumT, UpstreamClockT, Identity>; \
	template<typename NumT = _DefaultNumT_, typename UpstreamClockT = detail::StandaloneClock, typename TransformT = Identity> \
	_ClockName_(UpstreamClockT&, TransformT) -> _ClockName_<NumT, UpstreamClockT, TransformT>;


	//----------------------------------------------------------------------------
	// Example clock chaining transformations (applied to upstream now().value):
	struct Scale_x2 { constexpr auto operator()(auto t) const noexcept { return t*2; } };


	//----------------------------------------------------------------------------
	// Unified "synthetic" clock implementation (CRTP base)
	//----------------------------------------------------------------------------
	template<class Derived, typename PolicyT,
		typename UpstreamClockT = detail::StandaloneClock,
		typename TransformT = Identity>
	class synthetic_clock : public generic_clock<Derived, PolicyT, UpstreamClockT, TransformT>
	{
		using base = generic_clock<Derived, PolicyT, UpstreamClockT, TransformT>;
	public:
		using base::base;
		using time = time<PolicyT>;

		//!!C++: Forwarding ctor for CTAD
		explicit synthetic_clock(UpstreamClockT& src, TransformT tr = TransformT{})
			: base(src, tr) {}

		time read() const noexcept { return t_; }

		void advance() noexcept
			requires requires(typename PolicyT::underlying_type& t) //!!C++: FFS, this syntax!
			{ PolicyT::advance(t); }
		{
			PolicyT::advance(t_.value_ref());
		}

		void advance(typename PolicyT::diff_type delta) noexcept
			requires requires(typename PolicyT::underlying_type& t, typename PolicyT::diff_type d) //!!C++: FFS, this syntax!
			{ PolicyT::advance(t, d); }
		{
			PolicyT::advance(t_.value_ref(), delta);
		}

	protected:
		time t_{};
	};


	//----------------------------------------------------------------------------
	// Auto-incrementing (self-sourced), wrap-aware tick counter
	//----------------------------------------------------------------------------
	namespace wrapping_counter
	{
		template<discrete_time_type_c NumT>
		struct policy
		{
			using underlying_type = NumT;
			using diff_type = duration<NumT>;

			static constexpr bool discrete                 = true;
			static constexpr bool controlled_wrap          = true;
			static constexpr bool strictly_monotonic       = true;
			static constexpr diff_type chronon             = 1;
			static void advance(NumT& t)                  noexcept { ++t; }
			static void advance(NumT& t, diff_type delta) noexcept { t = NumT(t + delta); } //!! Silent type reqs.!...
	//		static constexpr bool explicit_from_underlying = true; //!!?? Would false here ever be that much more helpful?
	//		static constexpr bool explicit_to_underlying   = controlled_wrap; // No restrictions if not supposed to wrap!
			//!! ...
		};

		_SZ_CLOCK_CLASS_BEGIN(clock, unsigned)
			#ifdef DEBUG
				/*!! Must be defined at the derived class: base-class op= will be ignored! :-/ !!*/
				public: clock& operator = (NumT count) { this->t_.value_ref() = count; return *this; }
			#endif
		_SZ_CLOCK_CLASS_END
		_SZ_CLOCK_DEDUCTION_GUIDES(clock, unsigned)
	} // namespace wrapping_counter

	/*!! TODO: Digest this old proto design:
	struct clock_ticks
	{
		std::uint32_t t; //!! uint32_t is "optional" in std! (Let's hope we'll only ever support platforms where it's available...)
		// Implicit conversions are not that risky in this context:
		constexpr operator unsigned () const { return t; }
		constexpr clock_ticks() : t(0) {}
//		constexpr clock_ticks(auto start_from) : t(start_from) {}
	};
	!!*/


	//----------------------------------------------------------------------------
	// Auto-advancing (self-sourced), long-running ("non-wrapping", "analog") clock
	//----------------------------------------------------------------------------
	namespace analog_synthetic
	{
		template<analog_synthetic_time_type_c NumT>
		struct policy
		{
			using underlying_type = NumT;
			using diff_type = duration<NumT>;

			static constexpr bool discrete                 = false;
			static constexpr bool controlled_wrap          = false;
			static constexpr bool strictly_monotonic       = true;

		#ifdef DEBUG
			static constexpr diff_type chronon = NumT(0.01); //!! JUST FOR EASIER TESTING!
		#else
			static constexpr diff_type chronon = detail::epsilon<NumT>();
		#endif
			static void advance(NumT& t, diff_type delta = chronon) noexcept { t += delta; } //!! Silent type req. of += !...
		};

		_SZ_CLOCK_CLASS_BEGIN(clock, unsigned)
			#ifdef DEBUG
				/*!! Must be defined at the derived class: base-class op= will be ignored! :-/ !!*/
				public: clock& operator = (NumT count) { this->t_.value_ref() = count; return *this; }
			#endif
		_SZ_CLOCK_CLASS_END
		_SZ_CLOCK_DEDUCTION_GUIDES(clock, double)
	} // namespace analog_synthetic

#undef _SZ_CLOCK_CLASS_BEGIN
#undef _SZ_CLOCK_CLASS_END
#undef _SZ_CLOCK_DEDUCTION_GUIDES
} // namespace impl


//----------------------------------------------------------------------------
// Exported vocabulary aliases for the stock synthetic clocks
// (Strictly monotonic, self-update on calling now().)
//----------------------------------------------------------------------------

// Auto-incrementing discrete clock (tick counter)
//
// Wraparound-safe for intervals shorter than NumT's range.
//
template<discrete_time_type_c NumT = unsigned,
	typename UpstreamClockT = detail::StandaloneClock,
	typename TransformT = Identity>
using tick_clock = impl::wrapping_counter::clock<NumT, UpstreamClockT, TransformT>;

	static_assert(tick_clock<>::time::policy::controlled_wrap);
	static_assert(clock_c<tick_clock<>>); //!!C++ Can't deduce it without <> in this context!
	static_assert(clock_c<tick_clock<unsigned short>>);
	static_assert(clock_c<tick_clock<unsigned long long>>);


// Auto-advancing "analog" timer, up to NumT's max. value
//
// Note: float may be too risky for a general-purpose underlying type: comparing
// high values with nanosecond precision could lose the last digits too easily,
// and would always just look equal, even minutes apart!... (E.g. when linked
// to a realtime clock driven by chrono::steady_clock, which may hold any value,
// not just a few hours of system uptime.)
//
template<analog_synthetic_time_type_c NumT = double,
	typename UpstreamClockT = detail::StandaloneClock,
	typename TransformT = Identity>
using model_clock = impl::analog_synthetic::clock<NumT, UpstreamClockT, TransformT>;

	static_assert( ! model_clock<>::time::policy::controlled_wrap);
	static_assert(clock_c<model_clock<>>); //!!C++ Can't deduce it without <> in this context!
	static_assert(clock_c<model_clock<float>>);
	static_assert(clock_c<model_clock<long double>>);

} // namespace time
} // namespace sz


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!! Remnants of the old time.hh:
#if 0
/*!! Yeah, no... Still no generic "operator dispatch" in C++, so this is hopeless without
     a bunch of annoying boilerplate to define all the op= and comparison etc. functions...
     And even with all those, an innocent check for is_floating_point<decltype(time)>
     would STILL FAIL! :)
	struct seconds
	{
		double s;
		// Implicit conversions are not that risky in this context:
		constexpr seconds() : s(0) {}
		constexpr seconds(double sec) : s(sec) {}
		constexpr operator double() const { return s; }
	};
!!*/
	//
	// So... Thanks for nothing, C++! ;-p
	//
	using seconds = double;


	// ...crickets...
#endif