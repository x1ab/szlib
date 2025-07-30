// v0.1.0

#ifndef _SZXCVBN3807R5YT68FNUGVHM678_
#define _SZXCVBN3807R5YT68FNUGVHM678_

/* Example usage:

#ifdef UNIT_TEST
#include "...diag/test.hh"
int main()
{
	using namespace sz::test;

	Test(
	// The test subject:
		is_absolute,
	// Optional title, flags:
		"szlib/sys/is_absolute") //, Stop_on_Failure)
	// Test cases:
		.run( In{ ""  }, Expect{ false } )
		.run( In{ " " }, Expect{ false } )
		.run( In{ "/" }, Expect{ true }  )
	;

	OR, using a wrapper to provide insight into what each test case does:

	Test	( [](string_view p) {
			bool abs = is_absolute(p);
			cout << "\""<< p <<"\" is "<< (abs ? "absolute" : "relative"); // No trailing \n helps formatting the test output.
			return abs;
		}, "is_absolute" )
		.run( In{ ""  }, Expect{ false } )
		.run( In{ " " }, Expect{ false } )
		.run( In{ "/" }, Expect{ true }  )
	;
}
#endif // UNIT_TEST

NOTES:

	The Test destructor will report the summary. If you want that to happen
	earlier, either call .report() (which will silence the dtor), or, if you
	don't use the Test().rum(...) idiom, but prefer `Test batch; batch.run(...)`,
	then put `batch` into a scope with the closing } being the reporting point.

*/

/*!!TODO: Sg. like this (but such tables are rigid against optional per-TC props!):

	Test(test_abs) // perhaps: {
		{ In{ ""        }, Expect{ false } },
		{ In{ " "       }, Expect{ false } },
		{ In{ "/"       }, Expect{ true }  },
		{ In{ "\\"      }, Expect{ true }  },
		{ In{ " /"      }, Expect{ false } },
		{ In{ " \\"     }, Expect{ false } },
		{ In{ " /rel"   }, Expect{ false } },
		{ In{ " \\rel"  }, Expect{ false } },
		{ In{ "C:"      }, Expect{ false } },
		{ In{ "C:."     }, Expect{ false } },
		{ In{ "C:.\\"   }, Expect{ false } },
		{ In{ "C:./"    }, Expect{ false } },
		{ In{ "C:rel"   }, Expect{ false } },
		{ In{ "C:rel"   }, Expect{ false } },
		{ In{ "C:/"     }, Expect{ true }  },
		{ In{ "C:\\"    }, Expect{ true }  },
		{ In{ "C:/abs"  }, Expect{ true }  },
		{ In{ "C:\\abs" }, Expect{ true }  },
		{ In{ "."       }, Expect{ false } },
		{ In{ "./"      }, Expect{ false } },
		{ In{ ".\\"     }, Expect{ false } },
		{ In{ ".."      }, Expect{ false } },
		{ In{ "./"      }, Expect{ false } },
		{ In{ ".\\"     }, Expect{ false } },
		{ In{ "/rel"    }, Expect{ false } },
		{ In{ "\\rel"   }, Expect{ false } },
	// }
	;
!!*/

#include <string>
#include <string_view>
#include <functional> // function
#include <tuple> // tuple, tuple_size, get, apply
#include <utility> // forward, decay
#include <type_traits> // false_type, true_type
#include <iostream>

namespace sz::test {

// ---- The ubiquitous "Function Traits" helper that should be std::,
//     but never will, because C++... ---

// Primary template (unimplemented)
template<typename T>
struct function_traits;
// Specialization for regular function pointers
template<typename R, typename... Args>
struct function_traits<R(*)(Args...)> {
    using signature = R(Args...);
};
// Specialization for any callable object with an operator()
// (lambdas, functors, etc.)
template<typename T>
struct function_traits : function_traits<decltype(&T::operator())> {};
// Specializations for member function pointers (for the operator() case)
template<typename C, typename R, typename... Args>
struct function_traits<R(C::*)(Args...)> {
    using signature = R(Args...);
};
template<typename C, typename R, typename... Args>
struct function_traits<R(C::*)(Args...) const> { // const version
    using signature = R(Args...);
};
//!! ...add specs. for noexcept etc. as needed!


// ---- Another helper for static_asserting callability before
//      letting the compiler delve deep into it, only to find an
//      error at the end, and shit out an ocean of indecypherable
//      templte errors...
template<typename Fn, typename Tuple>
struct is_applicable;

template<typename Fn, typename... Args>
struct is_applicable<Fn, std::tuple<Args...>> {
	static constexpr bool value = std::is_invocable_v<Fn, Args...>;
};
template<typename Fn, typename Tuple>
inline constexpr bool is_applicable_v = is_applicable<Fn, Tuple>::value;


// Helper struct to "convert struct to tuple", enabling the untyped brace-init
// literal syntax for `.run( {input1, 2, ...}, expected)`
template<typename... Ts>
struct TupleFromBraces {
	std::tuple<Ts...> values;
	template<typename... Args>
	TupleFromBraces(Args&&... args) : values(std::forward<Args>(args)...) {}
};

// Deduction guide to avoid `TupleFromBraces<some_t, other_t, ...>{...}`:
template<typename... Ts>
//!! Without the decay, temporaries, refs and move may cause surprises with inputs.
//!! The decayed (by-val) version is safer, but does require copyable inputs!
//!! Let's see, which one will hurt more...:
TupleFromBraces(Ts&&...) -> TupleFromBraces<std::decay_t<Ts>...>;
//TupleFromBraces(Ts&&...) -> TupleFromBraces<Ts...>;

// Semantic DSL-sugarcoating:
template<typename... Ts> using In     = TupleFromBraces<Ts...>;
template<typename... Ts> using Expect = TupleFromBraces<Ts...>;

// Helper boilerplate to prevent even compilers pooping themselves from the
// `requires requires` bullshit (for which I got an MSVC internal error...):
template<typename T>     struct using_that_fkn_tuple : std::false_type {};
template<typename... Ts> struct using_that_fkn_tuple<TupleFromBraces<Ts...>> : std::true_type {};

enum RunFlags : int {
//!! Too much collision risk with `using namespace test` if unqualified...:
		RunFlags_Defaults = 0,
		Stop_On_Failure = 1,
		No_Auto_Summary = 2, // Call test.report() manually for a summary.
//		... = 4,
};

//----------------------------------------------------------------------------
template <typename Signature>
struct Test
// Run-once, throw-away test runner
{
	// Config:
	std::function<Signature> f; // Copy semantics for now, to allow replacing!
	std::string batch_name;
 	RunFlags flags;
	// State:
	int cases_run = 0, cases_failed = 0;
	bool reported = false;

	template<typename Fn>
	Test(Fn&& f, std::string_view name = "", RunFlags flags = RunFlags_Defaults)
        	: f(std::forward<Fn>(f))
		, batch_name(name.empty() ? __func__ : name)
		, flags(flags)
	{
		std::cerr
			<< "------------------------------------------------------------------------------\n"
			<< " Test batch \""<< batch_name <<"\"...\n"
			<< "------------------------------------------------------------------------------\n"
		;
	}

	~Test()
	{
		if (!(flags & No_Auto_Summary)
			&& !reported) report();
	}

	/* Since the callable type (of f) is frozen at comp. time, this is pretty much pointless:
	template<typename Fn>
		requires std::convertible_to<Fn, std::function<Signature>>	
	auto& set(Fn&& f)
	{
        	this->f = std::forward<Fn>(f);
		std::cerr << "---- The test-subject callable has been replaced. ----\n";
		return *this;
	}
	*/

	auto& report(bool silence_the_dtor = true)
	{
		if (!cases_failed) {
			std::cerr // << "\n" // Trust the prev. run() outputs to always end with \n...
				<< "------------------------------------------------------------------------------\n"
				<< " ALL OK. (Cases run: " << cases_run << ")"
				<< "\n"
				<< "------------------------------------------------------------------------------\n"
			;
		} else { 
			std::cerr // << "\n" // Trust the prev. run() outputs to always end with \n...
				<< "------------------------------------------------------------------------------\n"
				<< " --->>> "<< cases_failed <<" case"<< (cases_failed>1 ? "s":"") <<" FAILED!!!"
				<< " ("<< (float(cases_failed)/cases_run * 100.f) <<"%"
				<< " of "<< cases_run <<" case"<< (cases_run>1 ? "s":"") <<" run)"
				<< "\n"
				<< "------------------------------------------------------------------------------\n"
			;
		}
		if (silence_the_dtor) reported = true;

		return *this;
	}

	template <class Input, class ExpectedT>
	auto& run(const Input& input, const ExpectedT& expected)
//!!		requires requires { []<typename... Args>(const TupleFromBraces<Args...>&){}(expected); }
			//! ...Instead of the missing std::is_specialization_of... No comment; just "modern C++".
		requires using_that_fkn_tuple<std::decay_t<ExpectedT>>::value
// This makes the GCC error worse than the static_assert below:
//			&& is_applicable_v<std::function<Signature>, decltype(input.values)>
	{
		if (!__prepare_tc_run())
			return *this;

		static_assert(is_applicable_v<std::function<Signature>, decltype(input.values)>,
			"\n\n- ERROR: Argument count mismatch!\n"
			    "  The number of arguments in In{...} does not match the signature of the test function.\n"
			    "  You may need to provide values for default parameters explicitly.\n");
		auto result = std::apply( //!! Remember: `ExpectedT result` would be the wrapper type, not the tuple...
			std::forward<std::function<Signature>>(f),
			input.values
		);

		//! Note: this requires f's return type to be ==able to the `expected` tuple!
		//! Between tuples that'a given, but unfortunately, the vast majority of test
		//! functions will just return a single value (that is not a tuple), and a
		//! tuple's op== won't automatically fall back to member<0>::op==, so we need
		//! to take care of that manualy here (below)...
		//!
		//! Also: as there's no native C++ support for comparing structs to tuples, for
		//! functions returning a struct a free-standing `op==(struct, tuple)` must be
		//! written (somewhere in the test code)!
		bool passed;
		if constexpr (std::tuple_size_v<decltype(expected.values)> == 1) {
			// Compare the result directly:
			passed = (result == std::get<0>(expected.values)); // .values is the tuple inside Expected
		} else {
			// Member-wise op== for the rare multi-value f cases:
			passed = (result == expected.values); // .values is the tuple inside Expected
		}
		__handle_tc_result(passed);

		return *this;
	}

	template <class Input, typename ExpectedT>
	auto& run(const Input& input, const ExpectedT& expected)
//!!		requires (!requires { []<typename... Args>(const TupleFromBraces<Args...>&){}(expected); })
			//! ...Instead of the missing std::is_specialization_of... No comment; just "modern C++".
		requires (!using_that_fkn_tuple<std::decay_t<ExpectedT>>::value)
// This makes the GCC error worse than the static_assert below:
//			&& is_applicable_v<std::function<Signature>, decltype(input.values)>
	{
		if (!__prepare_tc_run())
			return *this;

		static_assert(is_applicable_v<std::function<Signature>, decltype(input.values)>,
			"\n\n- ERROR: Argument count mismatch!\n"
			    "  The number of arguments in In{...} does not match the signature of the test function.\n"
			    "  You may need to provide values for default parameters explicitly.\n");
		auto result = std::apply( //!! `ExpectedT result = ...` could introduce dangerous silent, premature narrowing conversions!
			std::forward<std::function<Signature>>(f),
			input.values
		); //!!OLD: invoke(forward<Fn>(f), forward<Args>(args)...);

		__handle_tc_result(result == expected);
		return *this;
	}

protected:
	bool __prepare_tc_run() {
		if (flags & Stop_On_Failure && cases_failed) {
			return false;
		} else {
			++cases_run;
			std::cerr << " #"<< cases_run << ": ";
			return true;
		}
	}
	bool __handle_tc_result(bool passed) {
		if (!passed) { ++cases_failed; }
		std::cerr << (passed ? "\t\t[ok]" : "\t--- [!!! FAILED !!!]") << "\n";
		return passed;
	}
}; // class Test

// --- Deduction Guides for deducing Test<Signature> from a ctor call:
// - #1: For lambdas and other callables (functors)
template<typename Fn>
Test(Fn&& func, std::string_view name = "", RunFlags flags = RunFlags_Defaults)
	-> Test<typename function_traits<decltype(&std::decay_t<Fn>::operator())>::signature>;

// - #2: For function pointers
template<typename R, typename... Args>
Test(R(*)(Args...), std::string_view name = "", RunFlags flags = RunFlags_Defaults)
	-> Test<R(Args...)>; // Directly construct Test with the clean signature

} // namespace sz::test

#endif // _SZXCVBN3807R5YT68FNUGVHM678_
