// v0.1.5

#ifndef _LSF39847G45796GK890G676G42GF35_
#define _LSF39847G45796GK890G676G42GF35_

#include <filesystem>
#include <string>
#include <string_view>
#include <cassert>

namespace sz::fs {

inline std::string getcwd()
{
	std::error_code ec;
	std::filesystem::path cwd = std::filesystem::current_path(ec);
	return ec ? "" : cwd.string();
}

inline std::string dirname(std::string_view path)
{
	return std::filesystem::path(path).parent_path().string();
}

inline std::string basename(std::string_view path, bool keep_last_suffix = true)
{
	return keep_last_suffix ? 
		std::filesystem::path(path).filename().string() :
		std::filesystem::path(path).stem().string();
}

// This one updates the input in-place
inline std::string& endslash_fixup(std::string* dirpath)
{
	assert(dirpath);
	std::string& result = *dirpath;
	if (!result.empty() && result.back() != '/' && result.back() != '\\')
	       result += '/';
	return result;
}

inline std::string endslash_fixup(const std::string& dirpath)
{
	std::string result(dirpath);
	return endslash_fixup(&result);
}

#ifdef _WIN32
#  define BACKSLASH_ON_WINDOWS(char) ((char) == '\\')
//#  define _P_SEP_ '\\'
#else
#  define BACKSLASH_ON_WINDOWS(char) (false)
//#  define _P_SEP_ '/'
#endif
inline bool is_absolute(std::string_view path)
{
#ifdef _WIN32 //!! This should be configurable: there are /-rooted path-resolving envs. also on Windows!

	return
		path.length() >= 3 && (path[1] == ':') //!!?? What are the edge cases (apart from CON: and
		&&                                     //!!?? LPT1: etc. that are not supported here anyway)?
		(path[2] == '/' || BACKSLASH_ON_WINDOWS(path[2]))
	;

#else // Assuming Unix-like (!!incorrectly!!)...

	return path.length() > 0 && (path[0] == '/');

#endif
}

namespace internal {

	inline bool syntactically_unprefixable(std::string_view path)
	// Windows: `c:...`, and `~` on non-Win (assuming UNIX — incorrectly though!)
	// Note: `//...` and `\\...` are technically prefixable: they'd yield a valid path.
	{
#ifdef _WIN32 //!! This should be configurable: there are /-rooted path-resolving envs. also on Windows!

		return
			path.length() >= 2 && (path[1] == ':') //!!?? What are the edge cases (apart from CON: and
		;	                                       //!!?? LPT1: etc. which are not supported here anyway)?

#else
		// Not really a (raw) path, but a shell symbol, but I see not reason not to support it here:
		return path.length() > 0 && (path[0] == '~');

#endif
	}

	inline bool dotted_optout(std::string_view path)
	// Returns true if `path` explicitly has the form of "./..." (or just ".").
	// Note: "../xxx" are not special; "./../xxx" could still be used to mark up those, too!
	{
		return
			path.length() == 1 && (path[0] == '.')
			||
			path.length() >= 2 && (path[0] == '.')
			                   && (path[1] == '/' || BACKSLASH_ON_WINDOWS(path[1]))
			// Don't normalize the logic above, prefer readability!
		;
	}

	inline bool slashed_optout(std::string_view path)
	// Returns true if `path` starts with / (or also \ on Windows).
	{
		return path.length() >= 1 && (path[0] == '/' || BACKSLASH_ON_WINDOWS(path[0]));
	}
} // internal

// Path prefixing via my "intent protocol" (#17) to decode from the path
// itself whether to allow "rebasing" a given path (suffix) or not.
// The args can be string, string_view or const char* (-- for the latter see
// the corresponding partially specialized template below the main one!)
template <class Str, typename StrOrCharPtr>
std::string prefix_by_intent(const StrOrCharPtr& prefix, Str path,
                             bool dont_prefix_if_dotted = true)
{
	using namespace std;

        string path_s{path};

	if (internal::syntactically_unprefixable(path)
	|| (internal::dotted_optout(path) && dont_prefix_if_dotted)
	||  internal::slashed_optout(path)) {
	    return path_s;
	}

        string prefix_s{prefix};
	return
		path_s.empty() ? prefix_s // Just being nice; std::fs::path prefix / "" would return "prefix/"!
		               : prefix_s
			         + (prefix_s.back() == '/' || BACKSLASH_ON_WINDOWS(prefix_s.back())
			            ? "" : "/") // Yeah, also flippin' a / to Windows! ;-p
		                 + path_s
	;
}
#ifdef _WIN32
#  undef BACKSLASH_TOO_ON_WINDOWS
//#  undef _W_SEP_
#endif

template <typename StrOrCharPtr> // 'path' can be string or string_view, prefix can also be const char*
std::string prefix_by_intent(const StrOrCharPtr& prefix, const char* path,
                          bool treat_dotrel_as_abs = true)
{
	return prefix_by_intent(std::forward<const StrOrCharPtr&>(prefix), std::string_view(path),
	                     treat_dotrel_as_abs);
}

}; // namespace sz::fs


//============================================================================
#ifdef UNIT_TEST
#include "../diag/test.hh"

using namespace sz::fs;
using namespace sz::test;
using namespace std;

int main()
{
	Test([](string_view p) {
			bool abs = is_absolute(p);
			cout << "\""<< p <<"\" is "<< (abs ? "absolute" : "relative"); // No trailing \n helps formatting the test output.
			return abs;
		}
		, "is_absolute") //, Stop_on_Failure)
		.run( In{ ""        }, Expect{ false } )
		.run( In{ " "       }, Expect{ false } )
		.run( In{ "/"       }, Expect{ false }  )
#ifdef _WIN32 //!! This should be configurable: there are /-rooted path-resolving envs. also on Windows!
		.run( In{ "\\"      }, Expect{ false }  )
#endif
		.run( In{ "/rel"    }, Expect{ false } )
#ifdef _WIN32 //!! This should be configurable: there are /-rooted path-resolving envs. also on Windows!
		.run( In{ "\\rel"   }, Expect{ false } )
#endif
		.run( In{ " /"      }, Expect{ false } )  // Just means "./ "!
		.run( In{ " /rel"   }, Expect{ false } )
#ifdef _WIN32 //!! This should be configurable: there are /-rooted path-resolving envs. also on Windows!
		.run( In{ " \\"     }, Expect{ false } )  // (same)
		.run( In{ " \\rel"  }, Expect{ false } )
#endif
//!! Just sneaking in a random test for the test runner itself...:
//!!		.report(false)
		.run( In{ "."       }, Expect{ false } )
		.run( In{ "./"      }, Expect{ false } )
		.run( In{ ".."      }, Expect{ false } )
		.run( In{ "./.."    }, Expect{ false } )
#ifdef _WIN32 //!! This should be configurable: there are /-rooted path-resolving envs. also on Windows!
		.run( In{ ".\\"     }, Expect{ false } )
		.run( In{ "C:"      }, Expect{ false } )
		.run( In{ "C:."     }, Expect{ false } )
		.run( In{ "C:.\\"   }, Expect{ false } )
		.run( In{ "C:./"    }, Expect{ false } )
		.run( In{ "C:rel"   }, Expect{ false } )
		.run( In{ "C:rel"   }, Expect{ false } )
		.run( In{ "C:/"     }, Expect{ true }  )
		.run( In{ "C:\\"    }, Expect{ true }  )
		.run( In{ "C:/abs"  }, Expect{ true }  )
		.run( In{ "C:\\abs" }, Expect{ true }  )
#endif
//!! Just sneaking in a random test for the test runner itself...:
//!!		.set([](string_view x) { cout << "("<< x <<")"; return false; })
//!!		.run( In{"dummy"}, Expect{ false } )
	;

	cerr	<< "\n"
		<< "prefix_by_intent(...)\n"
		<< "\n";
/*
	// - it's a template, so wrapping it into a generic lambda!...
	Test([](auto&&... args) {
		// This lambda simply forwards its arguments to a matching
		// prefix_by_intent, which will be instantiated nicely buried here:
		return prefix_by_intent(std::forward<decltype(args)>(args)...);
		//!! However...
		//!! A generic lambra is ALSO completely unsuitable for deducing
		//!! its type for the Signature template argument of Test<>, so...
		//!! No better idea than just biting the bullet and pick concrete
		//!! specializations...
	}, "prefix_by_intent(...)") //, Stop_on_Failure)
*/
/*!! No longer needed with the non-template wrapper lambda instead (below at Test()):

	auto f = &prefix_by_intent<const char*, const char*>; // <..., true, false>

	// Adapter for supplying the default params to prefix_by_intent (which would otherwise
	// trip up the C++ templ. deduction!)...
	//!! Also: direclty supplying const char* here; the other supported input types
	//!! (string_view, string) need their own separate tests!!
	//!!?? What about the return type tho? It's all just implicit (I guess: string...) now! :-/
	struct In2_t_f : TupleFromBraces<const char*, const char*>
		{ In2_t_f(const char* prefix, const char* path)
		  : TupleFromBraces{prefix, path} {} };
!!*/
	Test([](const char* prefix, const char* path) {
			auto result = prefix_by_intent(prefix, path, true); // ..., special './'
			cout << "\""<< prefix <<"\" + \""<< path <<"\" => \""<< result <<"\""; // No trailing \n helps formatting the test output.
			return result;
		}, "prefix_by_intent(const char*, const char*, true (respect ./ opt-out))")
		.run( In{ "crap/", "target"  }, Expect{ "crap/target" } )
		.run( In{ "crap", "/target"  }, Expect{ "/target"     } )
		.run( In{ "crap", "/"        }, Expect{ "/"           } )
		.run( In{ "crap", ""         }, Expect{ "crap"        } )
		.run( In{ "crap", "./.."     }, Expect{ "./.."        } )
		.run( In{ "crap", ".."       }, Expect{ "crap/.."     } )
#ifdef _WIN32
		.run( In{ "crap", ".\\.."    }, Expect{ ".\\.."       } )
		.run( In{ "crap", "\\target" }, Expect{ "\\target"    } )
		.run( In{ "crap", "\\"       }, Expect{ "\\"          } )
		.run( In{ "crap", "c:"       }, Expect{ "c:"          } )
#endif
	;

	cerr	<< "\n"
		<< "- Disabled ./ optout... But /-prefixing should still prevent prefixing!\n"
		<< "\n";

	Test([](const char* prefix, const char* path) {
			auto result = prefix_by_intent(prefix, path, false); // ..., NO special treatment for './'
			cout << "\""<< prefix <<"\" + \""<< path <<"\" => \""<< result <<"\""; // No trailing \n helps formatting the test output.
			return result;
		}, "prefix_by_intent(const char*, const char*, false (NO ./ optout))")
		.run( In{ "crap", "/target"  }, Expect{ "/target"  } )
		.run( In{ "crap", "/"        }, Expect{ "/"        } )
		.run( In{ "crap", "./.."     }, Expect{ "crap/./.."} )
		.run( In{ "crap", ".."       }, Expect{ "crap/.."  } )
#ifdef _WIN32
		.run( In{ "crap", ".\\.."    }, Expect{ "crap\\.\\.."} )
		.run( In{ "crap", "\\target" }, Expect{ "\\target" } )
		.run( In{ "crap", "\\"       }, Expect{ "\\"       } )
#endif
	;

	cerr	<< "\n"
		<< "- More . cases...\n"
		<< "\n";

	auto test_dot = [](bool enable_dot_optout) {
		Test([=](const char* path) {
				auto prefix = "prefix";
				auto result = prefix_by_intent(prefix, path, enable_dot_optout);
				cout << "\""<< prefix <<"\" + \""<< path <<"\" => \""<< result <<"\""; // No trailing \n helps formatting the test output.
				return result;
			},
			string("prefix_by_intent ./ opt-out: ") + (enable_dot_optout ? "enabled" : "*** DISABLED ***"),
			Stop_On_Failure
		)
		.run( In{ "."      }, Expect { enable_dot_optout ? "."           : "prefix/."      } )
		.run( In{ "./"     }, Expect { enable_dot_optout ? "./"          : "prefix/./"     } )
		.run( In{ ".\\"    }, Expect { enable_dot_optout ? ".\\"         : "prefix/.\\"    } )
		.run( In{ "./keep" }, Expect { enable_dot_optout ? "./keep"      : "prefix/./keep" } )
		.run( In{ ".x"     }, Expect { enable_dot_optout ? "prefix/.x"   : "prefix/.x"     } )
		.run( In{ ".."     }, Expect { enable_dot_optout ? "prefix/.."   : "prefix/.."     } )
		.run( In{ "../"    }, Expect { enable_dot_optout ? "prefix/../"  : "prefix/../"    } )
		.run( In{ "..\\"   }, Expect { enable_dot_optout ? "prefix/..\\" : "prefix/..\\"   } )
		.run( In{ "..x"    }, Expect { enable_dot_optout ? "prefix/..x"  : "prefix/..x"    } )
		;
	};

	test_dot(true);  // .-optout ENABLED:
	test_dot(false); // .-optout DISABLED:

	TestStats::report();

	cerr	<< "\n"
		<< "Some random manual cases, basically to check compilation...\n"
		<< "\n";

	// Test with string_view tail:
	cout << prefix_by_intent("pre/", "string_view tail"sv) << '\n';
	// Test with string tail:
	cout << prefix_by_intent("pre/", "string tail"s) << '\n';
	// Test with const char* tail (again):
	cout << prefix_by_intent("pre/", "const char* tail") << '\n';

} // main

#endif // UNIT_TEST
#endif // _LSF39847G45796GK890G676G42GF35_
