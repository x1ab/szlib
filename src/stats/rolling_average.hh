// v0.32.0

#ifndef _DFG8FUYHM8UHG8UBRNBGHNUFC5B6_
#define _DFG8FUYHM8UHG8UBRNBGHNUFC5B6_

#include "sz/vocab/int.hh" // u32, u64

namespace sz {

	namespace _impl {

		// Zero-include ("singular, quiet") NAN for `float` and `double`.
		// Not suitable for general NAN checking!
		// Note: __builtin_bit_cast is available in each of the Big Three, yay!
		//!! TBD: 80 or 128-bit floats... Or even float16? But that doesn't have NAN, right?

		template <typename T>
		inline constexpr auto unset() noexcept {
			if constexpr (sizeof(T) == 4) {
				return __builtin_bit_cast(T, u32(0x7FC00000));
			} else if constexpr (sizeof(T) == 8) {
				return __builtin_bit_cast(T, u64(0x7FF8000000000000)); // No ull suffix is needed here.
			} else {
				static_assert(sizeof(T) == 4 || sizeof(T) == 8, "Unsupported float size!");
			}
		}

		inline constexpr bool is_unset(float n) noexcept {
			return __builtin_bit_cast(u32, n) == u32(0x7FC00000);
		}
		inline constexpr bool is_unset(double n) noexcept {
			return __builtin_bit_cast(u64, n) == u64(0x7FF8000000000000); // No ull suffix is needed here.
		}


		struct Use_Deduced_Type { bool Use_Deduced = true; };
		struct Use_Default_Type { bool Use_Default = true; };
	}

namespace stats {


//----------------------------------------------------------------------------
// Sliding-window rolling average (SMA)
// Warning: this counter can be HUGE! It will contain MAX_SAMPLES+1 float or
// double for the sample history!
// See SmoothRollingAverage/FastSmoothRollingAverage for lightweight (EMA!)
// alternatives.
template<int MAX_SAMPLES = 60, typename TFloat = float>
struct RollingAverage
{
	using type = TFloat;

	TFloat samples[MAX_SAMPLES] = {};
	TFloat sum = 0;
	int tail = 0;

	void update(TFloat new_sample)
	{
		sum += new_sample - samples[tail];
		samples[tail++] = new_sample;
		tail %= MAX_SAMPLES;
	}

	// Queries (Note: initial results are incorrect during ramp-up!)
	// Constraining this would require sg. like <type_traits>, so... Imma just trust Future Caller Me:
	template <typename TargetT>
	operator TargetT()    const { return TargetT(sum / MAX_SAMPLES); }
	//!!C++ bullshit:
	// Ridiculos as it may be, the above almighty op. is still useless when
	// it would be most useful: with numeric ops... :-/ So, we still need
	// non-templ. conv. ops.
	// OTOH... Having both float & double, impl. conversions would be ambiguous! :-(
//!!	operator float()    const { return float (sum / MAX_SAMPLES); }
	operator double()   const { return double(sum / MAX_SAMPLES); }
};



//!!
//!! MERGE THESE TWO BELOW: If `initial` is supplied, use the branchless update loop!
//!! Also, reconcile: the SMA is float by default, the Smooth* version is double!
//!!


//----------------------------------------------------------------------------
// Exponential moving average; needs a ramp-up delay of N measurements to
// stabilize. (N = 1 / (1 − β), so for 0.99, it's 100, or ~3 seconds at 30 FPS.)
// - Small: Only has one TFloat member.
// - Robust: Can be reset freely any time.
// - Reasonably fast: one highly-predictable extra branch per update.
//   For maximum speed, at the cost of initial bias, see FastSmoothRollingAverage
/*!!C++: Can't do `if constexpr` in non-function scope! Would need +1 dependent template impl... :-/
  !!     But the param. list sucks anyway: both should be optional INDEPENDENTLY! :-/
template<auto retention_factor = 0.99, typename TFloat = _impl::Use_Deduced_Type>
{
	if constexpr (TFloat::Use_Deduced) {
		using type = decltype(retention_factor); //!! Awkward though, if `retention_factor` is omitted, but they want e.g. float!
	} else {
		using type = TFloat;
	}
!!*/
template<typename TFloat = double, auto retention_factor = 0.99>
struct SmoothRollingAverage //!!C++: Alas, can't deduce TFloat from `retention_factor`! :-/
{
	using type = decltype(retention_factor); //!! Awkward though, if `retention_factor` is omitted, but they want e.g. float!
	//!! Add basic float check, OR degrade gracefully when int!

	//!!C++: static_asserts produce less horrible errors than `requires`, generally...
	static_assert(sizeof(type) == 4 || sizeof(type) == 8, "Only IEEE float and double (with a well-defined NAN) is supported!");
	static_assert(retention_factor >= 0 && retention_factor <= 1);


	type value = _impl::unset<type>();

	void update(type new_sample)
	{

		static constexpr type sf = type(retention_factor);
			// To avoid both casting in the interface definition above, and also the
			// non-0 chance of redundant runtime precision. extension/narrowing with
			// value = type( retention_factor * value + (1 - retention_factor) * new_sample );

		value = _impl::is_unset(value) ? new_sample : sf * value + (1 - sf) * new_sample;
	}

	// Queries

	// Constraining this would require sg. like <type_traits>, so... Imma just trust Future Caller Me:
	template <typename TargetT>
	operator TargetT()    const { return TargetT(value); }
	//!!C++ bullshit:
	// Ridiculos as it may be, the above almighty op. is still useless when
	// it would be most useful: with numeric ops... :-/ So, we still need
	// non-templ. conv. ops.
	// OTOH... Having both float & double, impl. conversions would be ambiguous! :-(
//!!	operator float()    const { return float(value); }
	operator double()   const { return double(value); }
};


//----------------------------------------------------------------------------
// Branchless, fast, invariant-free, but biased rolling average (EMA):
// the initial value has a diminishing pull on the result.
// Optimal for small, fast, high-freq., non-critical counters.
// Preset it to something close to the expected average, and/or prime it
// for a while to get a useful result! ("For a while" normally means:
// 1 / (1 − β), so for 0.99, it's 100, or ~3 seconds at 30 FPS.)
// See SmoothRollingAverage for a slightly slower, but self-priming version
// (with no need for `initial`)!
template<auto initial = 0.0, auto retention_factor = 0.99>
struct FastSmoothRollingAverage //!!C++: Alas, can't deduce TFloat from the other args! :-/
{
	static_assert(retention_factor >= 0 && retention_factor <= 1); //!!C++: Produces less horrible error than `requires`, generally.

	using TFloat = decltype(initial); //!! Very awkward though, if `initial` is omitted, becoming
	                                  //!! double by default, and the retention factor is set as float!...
	using type = TFloat; //!! Add basic float check, OR degrade gracefully when int!

	TFloat value = TFloat(initial); // Exp.-smoothed average; can be reset freely!

	void update(TFloat new_sample)
	{
		static constexpr TFloat β = TFloat(retention_factor);
			// To avoid both casting in the interface definition above, and also the
			// non-0 chance of redundant runtime precision. extension/narrowing with
			// value = TFloat( retention_factor * value + (1 - retention_factor) * new_sample );

		value = β  * value + (1 - β) * new_sample;
	}

	// Queries

	// Constraining this would require sg. like <type_traits>, so... Imma just trust Future Caller Me:
	template <typename TargetT>
	operator TargetT()    const { return TargetT(value); }
	//!!C++ bullshit:
	// Ridiculos as it may be, the above almighty op. is still useless when
	// it would be most useful: with numeric ops... :-/ So, we still need
	// non-templ. conv. ops.
	// OTOH... Having both float & double, impl. conversions would be ambiguous! :-(
//!!	operator float()    const { return float(value); }
	operator double()   const { return double(value); }

	/*!! OLD mess:
	//!!operator TFloat() const { return value; } //!! Just use .value instead; more straightforward anyway!
	//!! To distinguish ^this from float and double, we'd need sg. like <type_traits>.
	//!! So... Just add (float) & (double) regardless of TFloat, for convenience:
	//!! (Because there's no impl. conv. if the context != TFloat!)
	operator float()  const { return float(value); }
	operator double() const { return double(value); }
	operator int()    const { return int(value); }
	!!*/
};


} // namespace stats
} // namespace sz

#endif // _DFG8FUYHM8UHG8UBRNBGHNUFC5B6_
