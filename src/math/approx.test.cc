#include "approx.hh"

//============================================================================
// [clan]g++ -x c++ -Wall -Wextra -Wconversion -Wsign-conversion -pedantic -DUNIT_TEST
// cl -EHsc -std:c++17 -TP -W4 -sdl -DUNIT_TEST

#include <iostream>

int main()
{
	using namespace sz::math;

#define CHECK(expr, expected) \
	std::cout << ((expr) == (expected) ? "ok    " : "FAIL! ") << (expected ? "true:  " : "false: ") << #expr "\n"

	// --- Basics ---
	CHECK(close_abs(1.0, 1.0 + 1e-12, 1e-9),     true);
	CHECK(close_abs(1e6, 1e6 + 1,     1e-9),     false); // abs-only is scale-blind...
	CHECK(close_rel(1e6, 1e6 + 1,     1e-5),     true);  // ...rel isn't: 1 <= 1e-5 * 1e6
	CHECK(close_rel(1e-30, 1.0000001e-30, 1e-6), true);  // rel is scale-invariant: tiny is fine

	// --- Exact zeros ---
	CHECK(close_rel(0.0,  0.0, 1e-9), true);
	CHECK(close_rel(0.0, -0.0, 1e-9), true);

	// --- The "near 0" problem ---
	double z = 0.1 + 0.2 - 0.3;   // "should be 0", but is ~5.55e-17
	CHECK(close_rel(z, 0.0, 1e-9),           false); // one side exactly 0: rel can never match
	CHECK(close_rel(1e-17, -1e-17, 0.5),     false); // opposite signs: ditto, even with a huge e_rel
	CHECK(close_rel(z, 0.0, 1.0),            true);  // ...unless e_rel >= 1 (useless)
	CHECK(close_abs(z, 0.0, 1e-12),          true);  // abs handles it
	CHECK(close(z, 0.0, 1e-12, 1e-9),        true);  // combined: abs term saves the day near 0...
	CHECK(close(1e6, 1e6 + 1, 1e-12, 1e-5),  true);  // ...rel term at large scale

	// --- Unsigned (the abs(a-b) wraparound case) ---
	CHECK(close_abs(3u, 5u, 3u), true);  // abs(3u-5u) would be ~4e9!
	CHECK(close_abs(5u, 3u, 1u), false);

	// --- Unsigned rel ---
	// Note: close_rel on integers is degenerate: e_rel = 0 -> exact match, e_rel >= 1 -> always true (for unsigned)
	CHECK(close_rel(5u, 5u, 0u), true);
	CHECK(close_rel(3u, 5u, 0u), false);
	CHECK(close_rel(3u, 5u, 1u), true);
	CHECK(close_rel(5u, 3u, 1u), true);
}
