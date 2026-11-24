// v0.0.1
#ifndef SDFNC87FTD23487G7YFGDBGXCVE7YU
#define SDFNC87FTD23487G7YFGDBGXCVE7YU

#include "common.hh" // abs, absdiff, max

namespace sz::math {


// |a-b| <= e_rel * max(|a|,|b|)
// Note: (for e_rel < 1) always false if exactly one of a, b is 0, or if they differ in sign!
// (The "near 0" problem: "should be 0", but comes out as 1e-17 or -1e-17, and it will never match
// an exact 0 "relatively". That's when you need to use an e_abs term (too).
template <typename T>
bool close_rel(T a, T b, T e_rel) { return absdiff(a, b) <= e_rel * max(abs(a), abs(b)); }

// |a-b| <= e_abs
template <typename T>
bool close_abs(T a, T b, T e_abs) { return absdiff(a, b) <= e_abs; }

// |a-b| <= e_abs + e_rel * max(|a|,|b|)
template <typename T>
bool close(T a, T b, T e_abs, T e_rel) { return absdiff(a, b) <= e_abs + e_rel * max(abs(a), abs(b)); }


} // namespace sz::math

#endif // SDFNC87FTD23487G7YFGDBGXCVE7YU
