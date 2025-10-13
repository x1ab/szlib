// v0.3.1
// c++20

#ifndef _SZXCVBN3807R5YT68FNUGVHM678_
#define _SZXCVBN3807R5YT68FNUGVHM678_

/* Example usage:

#ifdef UNIT_TEST
#include "...diag/test.hh"
int main()
{
	using namespace sz::test;

	Test(
	// Optional title (the default is made of the file, line, func. of the Test call):
		"szlib/sys/is_absolute"),
	// The test subject:
		is_absolute,
	// Optional flags:
		Stop_on_Failure)
	// Cases:
		.run( In{ ""  }, Expect{ false } )
		.run( In{ " " }, Expect{ false } )
		.sep()                                       // With optional text instead of the default dashes.
		.run( In{ "/" }, Expect{ true }  )
		.skip( In{"nothing"}, Expect{"good things"}, "Not implemented!" ) // The "reason" is optional.
	;

	OR, using a wrapper to provide insight into what each test case does:

	Test	("is_absolute", [](string_view p) {
			bool abs = is_absolute(p);
			cout << "\""<< p <<"\" is "<< (abs ? "absolute" : "relative"); // No trailing \n helps formatting the test output.
			return abs;
		})
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
	then put `batch` into a scope with the closing } triggering the summary report.

	The stats from multiple Test() batches in the same thread are accumulated
	in a static instance, and can be reported by

		TestStats::report()

	usually when all the test have been run. The global stats can be cleared with:

		TestStats::reset()
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
#include <iomanip>
#include <source_location>
#include <cassert>

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


//----------------------------------------------------------------------------
enum TestFlags {                       //!! Too much collision risk for just `Flags` if `using namespace test`...
		Test_Defaults = 0,     //!! Too much collision risk for just `Defaults` if `using namespace test`...
//!!		Silent_Success       = 1, //!! Requires output buffering!
		Stop_On_Failure      = 2,
//!!		Stop_On_RunError     = 4,
		No_Auto_Summary      = 8, // Call test.report() manually for a summary.
};

//----------------------------------------------------------------------------
struct TestStats
{
	static constexpr auto ________NL = "------------------------------------------------------------------------------\n";
	static constexpr auto _SEP____NL = " - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n";
	static constexpr auto _EQ_____NL = "==============================================================================\n";
	static constexpr auto _B______NL = "/-----------------------------------------------------------------------------\n";
	static constexpr auto _E______NL = "\\-----------------------------------------------------------------------------\n";

	// State:
	struct Stats {
	  int	cases_total = 0,
		cases_run = 0,
		cases_failed = 0,  // run, but failed
		cases_skipped_explicitly = 0, // not run, because explicitly skip()ed
		cases_skipped_implicitly = 0; // skipped due to runtime error, or Stop_on_Failure etc.
		int cases_skipped() { return cases_skipped_explicitly + cases_skipped_implicitly; }
		Stats& operator += (const Stats& rhs) {
			cases_total  += rhs.cases_total;
			cases_run    += rhs.cases_run;
			cases_failed += rhs.cases_failed;
			cases_skipped_explicitly += rhs.cases_skipped_explicitly;
			cases_skipped_implicitly += rhs.cases_skipped_implicitly;
			return *this;
		}
	} stats;

	// Accumulated results from all runs in the same thread:
	thread_local static Stats global_stats; // See its def. below the class!

	//--------------------------------------------------------------------
	static void report(Stats& stats = TestStats::global_stats)
	{
		assert(stats.cases_total == stats.cases_run + stats.cases_skipped());

		if (&stats == &TestStats::global_stats) {
			report_grand_totals(stats);
			return;
		}

		if (stats.cases_total) {

			std::cerr << ________NL; // Trust the prev. run/skip outputs to always end with \n...

			if (!stats.cases_failed) {
				std::cerr
					<< " ALL OK."
					<< " ("
					<<	"Run: " << stats.cases_run
					<<	(stats.cases_run == stats.cases_total ? "" : " of " + std::to_string(stats.cases_total)
						+ ", skipped: " + std::to_string(stats.cases_skipped()))
					<< ")"
					<< "\n"
				;
			} else {
				std::cerr
					<< "\n"
					<< "\t--->>> "<< stats.cases_failed <<" case"<< (stats.cases_failed>1 ? "s":"") <<" FAILED!!! <<<---"
					<< " ("
					<<	std::fixed << std::setprecision(1)
					<<	(float(stats.cases_failed)/float(stats.cases_run) * 100) <<"%"
					<<	std::defaultfloat
	//				<<		" of "<< stats.cases_run <<" case"<< (stats.cases_run>1 ? "s":"") <<" run"
					<<		" of "<< stats.cases_run <<" run"
					<<		(stats.cases_skipped() ? ", skipped: " + std::to_string(stats.cases_skipped()): "")
					<< ")"
					<< "\n\n"
				;
			}
		}
		std::cerr << _E______NL;
	}

	//--------------------------------------------------------------------
	static void reset()
	{
		global_stats = Stats{};
	}

	//--------------------------------------------------------------------
	// Internals...
	//--------------------------------------------------------------------
protected:
	static void report_grand_totals(Stats& stats)
	{
		auto ppad = 25;

		std::cerr << "\n";
		std::cerr << _EQ_____NL;
		std::cerr
			<< " GRAND TOTALS: " << "\n"
			<<   std::setw(ppad)<<std::left<< "\t- cases:"              <<std::right<<std::setw(4)<< stats.cases_total  <<"\n"
			<<   std::setw(ppad)<<std::left<< "\t- run:"                <<std::right<<std::setw(4)<< stats.cases_run    <<"\n"
			<<   std::setw(ppad)<<std::left<< "\t- failed:"             <<std::right<<std::setw(4)<< stats.cases_failed <<"\n"
			<<   std::setw(ppad)<<std::left<< "\t- explicitly ignored:" <<std::right<<std::setw(4)<< stats.cases_skipped_explicitly <<"\n"
			<<   std::setw(ppad)<<std::left<< "\t- couldn't run:"       <<std::right<<std::setw(4)<< stats.cases_skipped_implicitly <<"\n"
		;

		if (stats.cases_total) {

			std::cerr << ________NL;

			if (!stats.cases_failed) {
				std::cerr << " ALL OK." << "\n";
			} else {
				std::cerr
					<< "\n"
					<< "\t--->>> "<< stats.cases_failed <<" case"<< (stats.cases_failed>1 ? "s":"") <<" FAILED!!! <<<---"
					<< " ("
					<<	std::fixed << std::setprecision(1)
					<<	(float(stats.cases_failed)/float(stats.cases_run)   * 100) <<"% of "<< stats.cases_run <<" run, "
					<<	(float(stats.cases_failed)/float(stats.cases_total) * 100) <<"% of all"
					<<	std::defaultfloat
					<< ")"
					<< "\n\n"
				;
			}
		}
		std::cerr << _EQ_____NL;
	}
};

// Accumulated results from all runs in the same thread:
thread_local TestStats::Stats TestStats::global_stats{};


//----------------------------------------------------------------------------
template <typename Signature>
class Test : protected TestStats
// Run-once, throw-away test runner
//----------------------------------------------------------------------------
{
	// Config:
	std::string batch_name;
	std::function<Signature> f; // Copy semantics for now, to allow replacing!
	TestFlags flags;

	// State:
	bool reported = false;

public:
	//--------------------------------------------------------------------
	template<typename Fn>
	Test(Fn&& f, TestFlags flags = Test_Defaults,
		std::source_location src_loc = std::source_location::current())
		: Test(src_loc, "", std::forward<Fn>(f), flags) {}

	template<typename Fn>
	Test(std::string_view name, Fn&& f, TestFlags flags = Test_Defaults,
		std::source_location src_loc = std::source_location::current())
		: Test(src_loc, name, std::forward<Fn>(f), flags) {}

//! Internal common path:
	template<typename Fn>
	Test(std::source_location src_loc, std::string_view name, Fn&& f, TestFlags flags)
	        : batch_name(name.empty()
			? std::string(src_loc.file_name())
				+ ", line " + std::to_string(src_loc.line())
				+ ": " + std::string(src_loc.function_name())
			: name)
	        , f(std::forward<Fn>(f))
	        , flags(flags)
	{
		show_header();
	}

	~Test()
	{
		global_stats += stats;

		if (!(flags & No_Auto_Summary)
			&& !reported) report();
	}

	// Since the callable type (Fn) is frozen at comp. time, this is pretty limited,
	// but there could still be use cases for this (e.g. comparative tests against
	// different versions/implementations of the same function etc.)
	template<typename Fn>
		requires std::convertible_to<Fn, std::function<Signature>>	
	auto& set(Fn&& f)
	{
        	this->f = std::forward<Fn>(f);
		//std::cerr << "---- The test-subject callable has been replaced. ----\n";
		return *this;
	}

	//--------------------------------------------------------------------
	template <class Input, class ExpectedT>
	auto& run(const Input& input, const ExpectedT& expected)
		requires using_that_fkn_tuple<std::decay_t<ExpectedT>>::value
		//!!requires requires { []<typename... Args>(const TupleFromBraces<Args...>&){}(expected); }
			//! ...Instead of the missing std::is_specialization_of... No comment; just "modern C++".
			//!! GCC constraint errors are worse than failed static_asserts, BTW:
			//&& is_applicable_v<std::function<Signature>, decltype(input.values)>
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
		requires (!using_that_fkn_tuple<std::decay_t<ExpectedT>>::value)
		//!!requires (!requires { []<typename... Args>(const TupleFromBraces<Args...>&){}(expected); })
			//! ...Instead of the missing std::is_specialization_of... No comment; just "modern C++".
			//!! GCC constraint errors are worse than failed static_asserts, BTW:
			// && is_applicable_v<std::function<Signature>, decltype(input.values)>
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

	//--------------------------------------------------------------------
	//!! Or: ignore(), just to distinguish more from "implicitly skipped"?...
	//!! But then there would be hopeless ambiguity whether "ignored" is also "skipped"... :)
	template <class Input, typename ExpectedT>
	auto& skip(const Input&, const ExpectedT&, std::string_view reason = "")
	{
		__prepare_tc_run(false, reason);
		return *this;
	}

	//--------------------------------------------------------------------
	auto& show_header()
	{
		std::cerr
			<< _B______NL
//!!OLD			<< " Test batch \""<< batch_name <<"\"...\n"
			<< " TEST: "<< batch_name <<"...\n"
			<< ________NL
		;
		return *this;
	}

	//--------------------------------------------------------------------
	auto& sep(std::string_view text = "")
	{
		std::cerr
			<< (text.empty() ? _SEP____NL : text)
		;
		return *this;
	}

	//--------------------------------------------------------------------
	auto& report(bool silence_the_dtor = true)
	{
		TestStats::report(this->stats);
		if (silence_the_dtor) reported = true;
		return *this;
	}

	auto& reset()
	{
		stats = Stats{};
		return *this;
	}

protected:
	bool __prepare_tc_run(std::string_view comment = "") { return __prepare_tc_run(true, comment); }
	bool __prepare_tc_run(bool run_mode, std::string_view comment = "")
	{
		++stats.cases_total;
		if (flags & Stop_On_Failure && stats.cases_failed) {
			++stats.cases_skipped_implicitly;
			return false;
		}

		if (run_mode) {
			++stats.cases_run;
			print_tc_id(std::cerr, stats.cases_total);
			return true;
		} else { // "explicit skip mode"
			++stats.cases_skipped_explicitly;
			std::cerr << "(--- ";
				print_tc_id(std::cerr, stats.cases_total, 0); // 0: no padding
			std::cerr
				<< "SKIPPED"
				<< (comment.empty() ? "!" : std::string("! ") + std::string(comment))
			<< " ---)\n";
			return false;
		}
	}

	bool __handle_tc_result(bool passed)
	{
		if (!passed) { ++stats.cases_failed; }
		std::cerr << (passed ? "\t\t[ok]" : "\t--- [!!! FAILED !!!]") << "\n";
		return passed;
	}

	auto& print_tc_id(std::ostream& out, int id, int pad = 5)
	{
		out << std::setw(pad) << (std::string("#") + std::to_string(id)) << ": "; // Right-align up to 999...
		return out;
	}

}; // class Test

// --- Deduction guides for deducing Test<Signature> from a ctor call...
// - #1: For lambdas and other callables (functors):
template<typename Fn>
Test(std::string_view name, Fn&& func, TestFlags flags = Test_Defaults)
	-> Test<typename function_traits<decltype(&std::decay_t<Fn>::operator())>::signature>;

// - #2: Untitled test:
template<typename Fn>
Test(Fn&& func, TestFlags flags = Test_Defaults)
	-> Test<typename function_traits<decltype(&std::decay_t<Fn>::operator())>::signature>;

// - #3: For a fn. pointer we can just use its callable type as-is:
template<typename R, typename... Args>
Test(std::string_view name, R(*)(Args...), TestFlags flags = Test_Defaults)
	-> Test<R(Args...)>;

// - #4: Untitled, with fn. ptr.:
template<typename R, typename... Args>
Test(R(*)(Args...), TestFlags flags = Test_Defaults)
	-> Test<R(Args...)>;


} // namespace sz::test


#ifdef UNIT_TEST
//============================================================================
// g++ -std=c++20 -g -gdwarf-4 -gstrict-dwarf -fsanitize=undefined -fsanitize-trap -Wall -Wextra -pedantic -DUNIT_TEST -x c++ runner.hh
// cl -std:c++20 -EHsc -Zi -fsanitize=address -W4 -DUNIT_TEST -TP runner.hh

using namespace sz::test;
using std::cout, std::cerr, std::string_view, std::string; using namespace std::literals;

int main()
{
	TestStats::report();

	Test([]{});

	//!! Void test subjects can't be run currently!...
	//!!Test([]{}).run(In{}, ...);
	//!!Test([]{}).run(In{}, Expect{});

	Test("Skip void test subject",
		[]{}).skip(In{}, Expect{});

	//!! This doesn't work either:
	//!! Test("Skip void test subject, with no Expect{}", []{}).skip(In{}, {});
	//!! Test("Skip void test subject, with no Expect{}", []{}).skip(In{}, (void)0);

	TestStats::report();

	Test([]{ return true; }).run(In{}, true);

	Test(	"This test should fail!",
		[]{ return true; }).run(In{}, false);

	Test([](){ cout << "hello\n"; return true; })
		.run(In{}, true)
		.sep()
		.sep("The next case should fail:")
		.run(In{}, false) // Fail!
		.sep()
	;

	TestStats::report();

	cerr << "\nAfter TestStats::reset()...:"; // <- The cumulative report starts with a newline...
	TestStats::reset();
	TestStats::report();


	Test("ALL SKIP", []{ return false; })
		.skip(In{}, true)
		.skip(In{}, false)
		.skip(In{}, true)
		.skip(In{}, false)
	;


	auto f1 = [](const char* in) { cout << in << ": f1"; return true; };
	auto f2 = [](const char* in) { cout << in << ": f2"; return false; };
	Test("Replacing the test callable (lamdas should be replaceable, too!)", f1)
		.run( In{"Running original"}, Expect{ true } )
		.set(f2)
		.run( In{"Running replacement"}, Expect{ false } )
		.sep("Now setting a compatible lambda...\n")
		.set([](string_view x) { cout << ">>> "<< x <<" <<<"; return false; })
		.run( In{"Running something else..."}, Expect{ false } )
	;

	//
	// More reporting...
	//
	{
		auto report_test = Test("Auto reporting DISALED by manual reporting", []{return true;});
		report_test.run(In{}, true);
		report_test.run(In{}, true);
	//!!	report_test.report(false); // false: don't disable the dtor's (duplicate!) auto reporting
		report_test.report();
		report_test.sep("The dtor will NOT also auto-report now; this failed case should appear in no summary:\n");
		report_test.run(In{}, false);
	}

	Test("INTERMEDIATE BY-REQUEST reporting; no auto report at destruction", []{return true;})
		.run(In{}, true)
		.run(In{}, true)
		.run(In{}, true)
		.sep("Early interim results... - DISABLES auto-reporting!\n")
		.report() // Only by-req reporting in this instance from now on!

		.sep("Cumulative by-req. report follows after this section...\n")
		.run(In{}, true)
		.skip(In{}, true)
		.run(In{}, false)
		.report()

		.sep("This section will have fresh stats, due to .reset()...\n")
		.reset()
		.run(In{}, true)
		.run(In{}, false)
		.report()

		.sep("The following skipped cases will NOT be reported in a summary:\n")
		.skip(In{}, true).skip(In{}, true).skip(In{}, true)
		.sep("\n...except in a manually called global report:\n")
	;
	TestStats::report();


	{	auto report_test_1 = Test("CONTROLLED auto reporting #1 - VERY MESSY THOUGH!", []{return true;});
		report_test_1.run(In{}, true);
		report_test_1.run(In{}, false);

	  {	auto report_test_2 = Test("CONTROLLED auto reporting #2 - VERY MESSY THOUGH!", []{return true;});
		report_test_2.skip(In{}, true);
		report_test_2.skip(In{}, false);
	  } // Reporting #2 here first...

		cerr << "\n\tSome random output between the two test stat. reports...\n\n";

	}   // ...then #1 here.

} // main
#endif // UNIT_TEST

#endif // _SZXCVBN3807R5YT68FNUGVHM678_
