// v0.1.0
/*!! TODO:
- Either forbid INFINITY, or handle it, to avoid close[_rel](INF, x, e) -> true!
  Also (INF, INF, e) -> false... While it makes sense math'lly, IEEE itself
  defined INF == INF as true, because INF is actually a number there... :-o
  Claude:
	A cheap fix for both: return true early if a == b, which also covers
	exact matches. After that, any remaining infinity means "not close".
	Without std, x - x != T(0) detects inf and NaN (true for both,
	but -ffast-math breaks it).

- Integer tolerance mul/add can overflow or wrap.
!!*/


#ifndef SDFNC87FTD23487G7YFGDBGXCVE7YU
#define SDFNC87FTD23487G7YFGDBGXCVE7YU

#include "common.hh" // abs, absdiff, max

namespace sz::math {

//!! C++ bullshit: boilerplate to avoid comp. errors when a, b, e are compatible, but different types:
	template <typename T> struct _same { using type = T; }; // Not dragging in <type_traits> just for this!
	template <typename T> using _nodeduce = typename _same<T>::type;


//----------------------------------------------------------------------------
// Difference relative to magnitude: |a-b| <= e_rel * max(|a|,|b|)
//
// (I.e. a and b differ by at most an e_rel fraction of the larger magnitude.
// Could as well be the smaller (min(|a|,|b|)), for stricter checking; the
// distinction is usually not significant for small e_rel.)
//
// e_rel must be >= 0.
//
// Notes (for the typical e_rel < 1 case):
//
// - Nothing nonzero is ever "close enough" to 0 this way (except for rounding
//   effects in the underflow range). Similarly, values of opposite sign are
//   never considered close either, no matter how small they are (since their
//   difference is at least as large as the larger of their magnitudes).
//
//   This makes close_rel(...) unsuitable for testing whether a value is
//   near 0 (e.g. calcs. like 0.1 + 0.2 - 0.3), or for comparing results
//   that may differ in sign due to rounding errors near 0.
//
//   Use close() with an absolute threshold for such comparisons!
//
// - OTOH, tiny magnitude * tiny e_rel is NOT inherently a precision problem
//   here: the multiplication incurs only a small relative rounding error,
//   unless the product enters the subnormal range (below ~1e-38 for float,
//   ~2e-308 for double), where quantization can throw off the comparison.
//
template <typename T>
bool close_rel(T a, _nodeduce<T> b, _nodeduce<T> e_rel) {
	return absdiff(a, b) <= e_rel * max(abs(a), abs(b));
}

// |a-b| <= e_rel * max(|a|,|b|) + e_abs
// Note: e_rel and e_abs must be >= 0.
template <typename T>
bool close(T a, _nodeduce<T> b, _nodeduce<T> e_rel, _nodeduce<T> e_abs) {
	return absdiff(a, b) <= e_rel * max(abs(a), abs(b)) + e_abs;
}

// |a-b| <= e_abs
// Note: e_abs must be >= 0.
template <typename T>
bool close_abs(T a, _nodeduce<T> b, _nodeduce<T> e_abs) {
	return absdiff(a, b) <= e_abs;
}


} // namespace sz::math

#endif // SDFNC87FTD23487G7YFGDBGXCVE7YU
