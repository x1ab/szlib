// v0.3.1
/*
  Limitations:

  - Only supports MSVC, GCC, Clang (no "vanilla for others" fallback — yet).
  - No support for long double (yet).
  - Explicitly not being liturgic with constexpr, noexcept, [[nodiscard]] pedantry,
    minimizing C++-insanity noise, at the cost of losing... nothing much.
    (I don't subscribe to "constexpr everything"; C++ should fix its defaults.)
  - Can't constrain ...unset() to fp. numbers without extra cruft from std (e.g.
    <type_traits> or <cmath>).
  - Even though I can't constrain abs() to numbers (without either including
    sg. like <type_traits> or losing support for unsigned integers), those `< 0`
    and `-n` in the template pretty clearly requires numers anyway, so dunno why
    I'm even telling this, sorry. :)
*/

#ifndef SZMATBCM89498Y2CU4V89EYHGWX47895HGV94R2
#define SZMATBCM89498Y2CU4V89EYHGWX47895HGV94R2

#include <stdint.h> //!! Rid even this!
	// See the quiet-NaN gymnastics for the "uninitialized"
	// sentinel values in ...unset()!
	// Hopefully, <stdint.h> will stay lean C for a long time.
	// (Less sure about <cstdint> though.)


//----------------------------------------------------------------------------
// --- COMPILER DETECTION & INTRINSICS ---
//----------------------------------------------------------------------------

//!! C++ bullshit: `inline` is a cancerous keyword. It doesn't even mean
//!! "inline" at all nowadays. And even when it did, it was a lie. ;)
//!! Also notice how — apart from it being an abomination all by itself,
//!! this thing below begs to live in a generic shim header, adding yet another
//!! cross-cutting dependency to what was supposed to be simple, prisitine,
//!! standalone std-only math code...:
#if defined(_MSC_VER)
#define _MATH_DO_INLINE_  [[msvc::forceinline]] inline
#elif defined(__GNUC__) || defined(__clang__)
#define _MATH_DO_INLINE_  [[gnu::always_inline]] inline
#else
#define _MATH_DO_INLINE_  inline
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#define _MATH_MSVC_FLAVOR_
//!! math.h in MSVC is still pure C, so just including it is fine:
	#include <math.h>
//!! ...and even desirable, as the symbols have more complex declarations
//!! than just the ones below, which don't cover DLL imports!
/*!!
	// MSVC: We must declare the symbols first to apply `pragma intrinsic` to them.
	//       Except for __builtin_bit_cast! It seems to be a keyword, like in GCC/Clang! :-o
extern "C" {
	double sqrt(double);
	float  sqrtf(float);
	double pow(double, double);
	float  powf(float, float);
	double ceil(double);
	float  ceilf(float);
	double floor(double);
	float  floorf(float);
	// Not an intrinsic (still?) in MSVC (will be function calls):
	double round(double);
	float  roundf(float);
	double fabs(double);
	float  fabsf(float);
}
	// Tell MSVC to use the intrinsic versions ("if possible"??!!)
	#pragma intrinsic(sqrt, sqrtf, pow, powf, ceil, ceilf, floor, floorf)
		//!! Alas, no round, roundf...
		//!! Also, fabs & fabsf are not intrinsics.

!!*/
#else
	// GCC/Clang: Builtins are keywords, so the declarations are not needed.
#endif

namespace sz::math {

	// --- SQRT ---
	_MATH_DO_INLINE_ float sqrt(float n) {
	#ifdef _MATH_MSVC_FLAVOR_
		return ::sqrtf(n);
	#else
		return __builtin_sqrtf(n);
	#endif
	}

	_MATH_DO_INLINE_ double sqrt(double n) {
	#ifdef _MATH_MSVC_FLAVOR_
		return ::sqrt(n);
	#else
		return __builtin_sqrt(n);
	#endif
	}

	// --- ABS ---
	//! C++ bullsit: There's no uniform (templated) std::abs in <cmath> suitable for floats and ints!
	//!              I guess if I included BOTH <cmath> AND <cstdlib>, I'd get the full overload set, though.
	//!!??           Also: The MSVC intrinsics aren't even constexpr?! :-o

	#ifdef _MATH_MSVC_FLAVOR_      //! MSVC warns (or even errs, with /sdl) at -n below, if
	#pragma warning(push)          //! n is unsigned, even though it's well-defined C++ (and
	#pragma warning(disable: 4146) //! that branch isn't even reachable for unsigned anyway)!
	#endif
	template <typename T>
	T abs(T n) { return n < 0 ? -n : n; }
	//!! Note: fabs* is NOT an MSVC intrinsic, but may STILL be faster without the FP->int cast!
	//!! (Haven't checked the generated code, but noticed no diff. in an ad-hoc FPS meas.)
	#ifdef _MATH_MSVC_FLAVOR_
	#pragma warning(pop)
	#endif

	_MATH_DO_INLINE_ float abs(float n) {
	#ifdef _MATH_MSVC_FLAVOR_
		//!!auto bits = __builtin_bit_cast(uint32_t, n);
		//!!bits &= 0x7FFFFFFF; // Mask to clear the sign (top) bit
		//!!return __builtin_bit_cast(float, bits);
		return ::fabsf(n);
	#else
		return __builtin_fabsf(n);
	#endif
	}

	_MATH_DO_INLINE_ double abs(double n) {
	#ifdef _MATH_MSVC_FLAVOR_
		//!!auto bits = __builtin_bit_cast(uint64_t, n);
		//!!bits &= 0x7FFFFFFFFFFFFFFF; // Mask to clear the sign (top) bit
		//!!return __builtin_bit_cast(double, bits);
		return ::fabs(n);
	#else
		return __builtin_fabs(n);
	#endif
	}

	// --- POWER ---
	// Note: 'pow' in the general case is not just a CPU instruction or two,
	// but the builtin still supports common special cases, which *can* end
	// up being 1-2 instructions eventually.
	_MATH_DO_INLINE_ double pow(double base, double exp) {
	#ifdef _MATH_MSVC_FLAVOR_
		return ::pow(base, exp);
	#else
		return __builtin_pow(base, exp);
	#endif
	}

	_MATH_DO_INLINE_ float pow(float base, float exp) {
	#ifdef _MATH_MSVC_FLAVOR_
		return ::powf(base, exp);
	#else
		return __builtin_powf(base, exp);
	#endif
	}

	// --- ROUNDING... ---

	_MATH_DO_INLINE_ double ceil(double n) {
	#ifdef _MATH_MSVC_FLAVOR_
		return ::ceil(n);
	#else
		return __builtin_ceil(n);
	#endif
	}
	_MATH_DO_INLINE_ float ceil(float n) {
	#ifdef _MATH_MSVC_FLAVOR_
		return ::ceilf(n);
	#else
		return __builtin_ceilf(n);
	#endif
	}

	_MATH_DO_INLINE_ double floor(double n) {
	#ifdef _MATH_MSVC_FLAVOR_
		return ::floor(n);
	#else
		return __builtin_floor(n);
	#endif
	}
	_MATH_DO_INLINE_ float floor(float n) {
	#ifdef _MATH_MSVC_FLAVOR_
		return ::floorf(n);
	#else
		return __builtin_floorf(n);
	#endif
	}

	_MATH_DO_INLINE_ double round(double n) {
	#ifdef _MATH_MSVC_FLAVOR_
		return ::round(n);
	#else
		return __builtin_round(n);
	#endif
	}
	_MATH_DO_INLINE_ float round(float n) {
	#ifdef _MATH_MSVC_FLAVOR_
		return ::roundf(n);
	#else
		return __builtin_roundf(n);
	#endif
	}


//----------------------------------------------------------------------------
// NaN...
//----------------------------------------------------------------------------
// Hijacking the IEEE "quiet NAN" value(s) for a light "optional" feature
// ("uninitialized" or "null" sentinel values) for floats...
//
// CAVEAT:
//
// Since NANs can't be compared, remember NEVER TO accidentally do `x == unset()`!...
// Hopefully the cumbersome `... = unset<type>()` is enough friction to block it. :)
// (I tried to only provide mark_unset(&)/is_unset(&), but then mark_unset() would
// be too annoying, making the one-step `Thing x = unset;` init syntax impossible!)

template <typename T>
inline constexpr auto unset() noexcept {
	if constexpr (sizeof(T) == 4) {
		return __builtin_bit_cast(T, uint32_t(0x7FC00000));
	} else if constexpr (sizeof(T) == 8) {
		return __builtin_bit_cast(T, uint64_t(0x7FF8000000000000)); // No ull suffix is needed here.
	} else {
		//!! TBD: 80 or 128-bit floats?...
		static_assert(sizeof(T) == 4 || sizeof(T) == 8, "Unsupported float size!");
	}
}

template <typename T>
inline constexpr void mark_unset(T& x) noexcept { // C++20 constexpr can mutate arguments.
	if constexpr (sizeof(T) == 4) {
		x = __builtin_bit_cast(T, uint32_t(0x7FC00000));
	} else if constexpr (sizeof(T) == 8) {
		x = __builtin_bit_cast(T, uint64_t(0x7FF8000000000000)); // No ull suffix is needed here.
	} else {
		//!! TBD: 80 or 128-bit floats?...
		static_assert(sizeof(T) == 4 || sizeof(T) == 8, "Unsupported float size!");
	}
}

/*!! This crashes my MSVC 19.40!
//	https://developercommunity.visualstudio.com/t/__builtin_bit_cast-Internal-compiler-err/10680632
//	https://developercommunity.visualstudio.com/t/MSVC-miscompiles-__builtin_bit_cast-when/10974125
template <typename T>
inline constexpr bool is_unset(T x) noexcept {

	//!!?? Could mask off the sign bit (0x7FFFFFFF...) to check for both NaN/Inf...

	if constexpr (sizeof(T) == 4) {
		return __builtin_bit_cast(uint32_t, x) == uint32_t(0x7FC00000);
	} else if constexpr (sizeof(T) == 8) {
		return __builtin_bit_cast(uint64_t, x) == uint64_t(0x7FF8000000000000); // No ull suffix is needed here.
	} else {
		// Not sure what to do with 80 or 128-bit floats...
		static_assert(sizeof(T) == 4 || sizeof(T) == 8, "Unsupported float size!");
	}
}
!!*/
/*
	_MATH_DO_INLINE_ constexpr void mark_unset(float& n) noexcept {
		n = __builtin_bit_cast(float, uint32_t(0x7FC00000));
	}
	_MATH_DO_INLINE_ constexpr void mark_unset(double& n) noexcept {
		n = __builtin_bit_cast(double, uint64_t(0x7FF8000000000000)); // No ull suffix is needed here.
	}
*/
#ifdef _MSC_VER__for_A_B_testing__ //!! If anything, this one was a tad slower overall, maybe? (Not specifically benchmarked!)
	template <typename T> _MATH_DO_INLINE_ /*!! NOT constexpr!!*/ bool is_unset(T n) noexcept {
		return isnan(n); // A macro, AFAIK.
	}
#else
	_MATH_DO_INLINE_ constexpr bool is_unset(float n) noexcept {
		return __builtin_bit_cast(uint32_t, n) == uint32_t(0x7FC00000);
	}
	_MATH_DO_INLINE_ constexpr bool is_unset(double n) noexcept {
		return __builtin_bit_cast(uint64_t, n) == uint64_t(0x7FF8000000000000); // No ull suffix is needed here.
	}
#endif


/*!! OLD, largely misguided draft, replaced by the direct, unambiguous
     "non-NAN" API above (which does not falsely suggest "IEEE NANs in general"!):

template <typename T> constexpr T NaN = NAN;
// Since NaN values can't be compared (but *can* be assigned, BTW),
// we need to adapt the robust std C check (usually a macro):
// ...But, as Gemini3 pointed out, NaNs might get optimized out
// of existence completely with fast-math options, so... instead of
//constexpr bool IsNaN(auto x) { return isnan(x)};
// let's use this hardened version (also from Gemini):
constexpr bool IsNaN(auto x) {
#ifndef _MSC_VER
	if consteval
	{
		// The hardware-based NaN != NaN trick is pretty safe here, because
		// fast-math optim. (hopefully...) don't corrupt const. eval. (yet?... ;-p )
		return x != x;
	}
	else
#endif
	{
	        return isnan(x);
	}
}
!!*/


/*!! These are just toy examples from Gemini3 Pro, just for display, and no
     touching, as they're not suitable for actual IEEE NAN semantics: there
     are "infinitely" many (*pun* intended) NaN bit patterns than just the
     one "quiet NAN" used below (and also not just one INF, albeit that's
     just two, for +/-).
     Basically kept here as a warning (and a handy reference for the bit patterns)...

	// "HASSLE-FREE" NAN/INF (No <math.h> required) ---
	// Using bit-puns via unions or simple hex literals since we have <stdint.h>
	static inline float Nan() {
		union { uint32_t i; float f; } pun = { 0x7FC00000 };
		return pun.f;
	}
	static inline float Infinity() {
		union { uint32_t i; float f; } pun = { 0x7F800000 };
		return pun.f;
	}
!!*/

} // namespace sz::math

#endif // SZMATBCM89498Y2CU4V89EYHGWX47895HGV94R2
