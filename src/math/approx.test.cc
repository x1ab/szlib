#include "approx.hh"

//============================================================================
// [clan]g++ -x c++ -Wall -Wextra -Wconversion -Wsign-conversion -pedantic -DUNIT_TEST
// cl -EHsc -std:c++17 -TP -W4 -sdl -DUNIT_TEST

#include <iostream>

int main(int argc, [[maybe_unused]] char** argv)
{
	using namespace sz::math;

	bool all_ok = true;
#define CHECK(expr, expected) \
	do { \
		bool ok = (expr) == (expected); \
		all_ok &= ok; \
		std::cout << (ok ? "ok    " : "FAIL! ") << (expected ? "true:  " : "false: ") << #expr "\n"; \
	} while (0)

	// --- Basics
	CHECK(close_abs(1.0, 1.0 + 1e-12, 1e-9),     true);
	CHECK(close_abs(1e6, 1e6 + 1,     1e-9),     false); // abs-only is scale-blind...
	CHECK(close_rel(1e6, 1e6 + 1,     1e-5),     true);  // ...rel isn't: 1 <= 1e-5 * 1e6
	CHECK(close_rel(1e-30, 1.0000001e-30, 1e-6), true);  // rel is scale-invariant: tiny is fine

	// --- Exact zeros
	CHECK(close_rel(0.0,  0.0, 1e-9), true);
	CHECK(close_rel(0.0, -0.0, 1e-9), true);
	CHECK(close    (0.0,  0.0, 1e-9, 100), true);
	CHECK(close    (0.0, -0.0, 1e-9, 100), true);
	// --- Rel. compare to 0
	CHECK(close_rel(1.0, 0.5, 0.9), true);      // baseline
	CHECK(close_rel(0.0, 0.5, 0.9), false);     // surprise! :-o
	CHECK(close    (0.0, 0.5, 0.9, 0.5), true); // <- the correct way...

	// --- The "near 0" problem (inexact zeros)
	double z = 0.1 + 0.2 - 0.3;   // "should be 0", but is ~5.55e-17
	CHECK(close_rel(z, 0.0, 1e-9),           false); // one side exactly 0: rel can never match
	CHECK(close_rel(1e-17, -1e-17, 0.5),     false); // opposite signs: ditto, even with a huge e_rel
	CHECK(close_rel(z, 0.0, 1.0),            true);  // ...unless e_rel >= 1 (useless)
	CHECK(close_abs(z, 0.0, 1e-12),          true);  // abs handles it
	CHECK(close(z, 0.0, 1e-9, 1e-12),        true);  // combined: abs term saves the day near 0...
	CHECK(close(1e6, 1e6 + 1, 1e-5, 1e-12),  true);  // ...rel term at large scale
	// --- Guarding close()'s param. order (in case I swap them again... :)
	CHECK(close(1000., 1001., 0.002, 0.0), true);
	CHECK(close(1000., 1001., 0.0, 0.002), false);

	// --- Unsigned (the abs(a-b) wraparound case)
	CHECK(close_abs(3u, 5u, 3u), true);  // abs(3u-5u) would be ~4e9!
	CHECK(close_abs(5u, 3u, 1u), false);

	// --- Unsigned rel
	// Note: close_rel on integers is degenerate: e_rel = 0 -> exact match, e_rel >= 1 -> always true (for unsigned)
	CHECK(close_rel(5u, 5u, 0u), true);
	CHECK(close_rel(3u, 5u, 0u), false);
	CHECK(close_rel(3u, 5u, 1u), true);
	CHECK(close_rel(5u, 3u, 1u), true);

	// --- Heterog. arg. type deduction: should/may warn, but compile!
	CHECK(close_rel(1000.f, 1001, 0.1), true);  //! Should warn: double -> float!
	CHECK(close_abs(0.001f, 0,    0.1), true);  //! Should warn: double -> float! (literal int 0 is fine)
	CHECK(close_abs(0, 0.9, 0.1), true);        //! Should warn: double -> int!
		int x = argc; // Sg. not optimized out...
	CHECK(close_abs(100.0f, x, 0.1f), false);   //! Should warn: int -> float!
	CHECK(close_abs(100.0,  x, 0.1f), false);   //! No warning, (32-bit...) int/float -> double is safe

	return all_ok ? 0 : 1;
}
