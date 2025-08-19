// v0.2.1

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

inline std::string endslash_fixup(std::string_view dirpath)
{
	std::string result(dirpath);
	endslash_fixup(&result);
	return result;
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
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
#endif
		return
			path.length() == 1 && (path[0] == '.')
			||
			path.length() >= 2 && (path[0] == '.')
			                   && (path[1] == '/' || BACKSLASH_ON_WINDOWS(path[1]))
			// Don't normalize the logic above, prefer readability!
		;
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
	}

	inline bool slashed_optout(std::string_view path)
	// Returns true if `path` starts with / (or also \ on Windows).
	{
		return path.length() >= 1 && (path[0] == '/' || BACKSLASH_ON_WINDOWS(path[0]));
	}
} // internal

// Path prefixing according to a light "intent protocol" (#17) that encodes
// in the path itself whether to allow "rebasing" it or not.
// The args can be string, string_view or const char* (-- for the latter
// see the corresponding partially specialized template below the main one!)
//template <class Str, typename StrOrCharPtr>
inline std::string prefix_by_intent(std::string_view prefix, std::string_view path)
{
	using namespace std;

	if (internal::syntactically_unprefixable(path)
	 || internal::dotted_optout(path)
	 || internal::slashed_optout(path)
	 || prefix.empty()) {
	    return string(path);
	}

	assert(!prefix.empty());

	return path.empty() ? string(prefix) // Just being nice; std::fs::path prefix / "" would return "prefix/"!
	                    : string(prefix)
	                      + (prefix.back() == '/' || BACKSLASH_ON_WINDOWS(prefix.back())
	                         ? "" : "/") // Yeah, also flippin' a / to Windows! ;-p
	                      + string(path)
	;
}
#ifdef _WIN32
#  undef BACKSLASH_TOO_ON_WINDOWS
//#  undef _W_SEP_
#endif

}; // namespace sz::fs


//============================================================================
// g++ -std=c++20 -g -gdwarf-4 -gstrict-dwarf -fsanitize=undefined -fsanitize-trap -Wall -Wextra -pedantic -DUNIT_TEST -x c++ fs.hh
// cl -std:c++20 -EHsc -Zi -fsanitize=address -W4 -DUNIT_TEST -TP fs.hh
#ifdef UNIT_TEST
#include "../diag/test.hh"

using namespace sz::fs;
using namespace sz::test;
using std::cout, std::cerr, std::string_view, std::string;
using namespace std::literals;
namespace fs = std::filesystem;

int main()
{
	Test( "sz::is_absolute == std::fs::path.is_absolute?",
		[](string_view p) {
				bool abs = is_absolute(p);
				cout << "\""<< p <<"\" is "<< (abs ? "absolute" : "relative"); // No trailing \n helps formatting the test output.
				return abs;
		})
		.run( In{ ""        }, Expect{ fs::path(""       ).is_absolute() } )
		.run( In{ " "       }, Expect{ fs::path(" "      ).is_absolute() } )
		.run( In{ "/"       }, Expect{ fs::path("/"      ).is_absolute() } )
#ifdef _WIN32 //!! This should be configurable: there are /-rooted path-resolving envs. also on Windows!
		.run( In{ "\\"      }, Expect{ fs::path("\\"     ).is_absolute() } )
#endif
		.run( In{ "/rel"    }, Expect{ fs::path("/rel"   ).is_absolute() } )
#ifdef _WIN32 //!! This should be configurable: there are /-rooted path-resolving envs. also on Windows!
		.run( In{ "\\rel"   }, Expect{ fs::path("\\rel"  ).is_absolute() } )
#endif
		.run( In{ " /"      }, Expect{ fs::path(" /"     ).is_absolute() } )
		.run( In{ " /rel"   }, Expect{ fs::path(" /rel"  ).is_absolute() } )
#ifdef _WIN32 //!! This should be configurable: there are /-rooted path-resolving envs. also on Windows!
		.run( In{ " \\"     }, Expect{ fs::path(" \\"    ).is_absolute() } )
		.run( In{ " \\rel"  }, Expect{ fs::path(" \\rel" ).is_absolute() } )
#endif
		.run( In{ "."       }, Expect{ fs::path("."      ).is_absolute() } )
		.run( In{ "./"      }, Expect{ fs::path("./"     ).is_absolute() } )
		.run( In{ ".."      }, Expect{ fs::path(".."     ).is_absolute() } )
		.run( In{ "./.."    }, Expect{ fs::path("./.."   ).is_absolute() } )
#ifdef _WIN32 //!! This should be configurable: there are /-rooted path-resolving envs. also on Windows!
		.run( In{ ".\\"     }, Expect{ fs::path(".\\"    ).is_absolute() } )
		.run( In{ "C:"      }, Expect{ fs::path("C:"     ).is_absolute() } )
		.run( In{ "C:."     }, Expect{ fs::path("C:."    ).is_absolute() } )
		.run( In{ "C:.\\"   }, Expect{ fs::path("C:.\\"  ).is_absolute() } )
		.run( In{ "C:./"    }, Expect{ fs::path("C:./"   ).is_absolute() } )
		.run( In{ "C:rel"   }, Expect{ fs::path("C:rel"  ).is_absolute() } )
		.run( In{ "C:rel"   }, Expect{ fs::path("C:rel"  ).is_absolute() } )
		.run( In{ "C:/"     }, Expect{ fs::path("C:/"    ).is_absolute() } )
		.run( In{ "C:\\"    }, Expect{ fs::path("C:\\"   ).is_absolute() } )
		.run( In{ "C:/abs"  }, Expect{ fs::path("C:/abs" ).is_absolute() } )
		.run( In{ "C:\\abs" }, Expect{ fs::path("C:\\abs").is_absolute() } )
#endif
	;

	Test( "endslash_fixup",
		[](string_view p) {
			auto result = endslash_fixup(p);
			cout << "endslash_fixup(\""<< p <<"\") => " << result; // No trailing \n helps formatting the test output.
			return result;
		})
		.run( In{ ""        }, Expect{ ""      } )
		.run( In{ " "       }, Expect{ " /"    } )
		.run( In{ "."       }, Expect{ "./"    } )
		.run( In{ "/"       }, Expect{ "/"     } )
		.run( In{ "//"      }, Expect{ "//"    } )
		.run( In{ "dir"     }, Expect{ "dir/"  } )
		.run( In{ "dir/"    }, Expect{ "dir/"  } )
#ifdef _WIN32 //!! This should be configurable: there are /-rooted path-resolving envs. also on Windows!
		.run( In{ "\\"      }, Expect{ "\\"    } )
		.run( In{ "\\\\"    }, Expect{ "\\\\"  } )
		.run( In{ "dir"     }, Expect{ "dir/"  } )
		.run( In{ "dir\\"   }, Expect{ "dir\\" } )
#endif
	;

/*
cerr<<"\n"<< "prefix_by_intent(...)\n\n";

	// - it's a template, so wrapping it into a generic lambda!...
	Test([](auto&&... args) {
		// This lambda simply forwards its arguments to a matching
		// prefix_by_intent, which will be instantiated nicely buried here:
		return prefix_by_intent(std::forward<decltype(args)>(args)...);
		//!! However...
		//!! A generic lambra is ALSO completely unsuitable for deducing
		//!! its type for the Signature template argument of Test<>, so...
		//!! No better idea than just biting the bullet and picking concrete
		//!! specializations...
	}, "prefix_by_intent(...)") //, Stop_on_Failure)
*/
	Test( "prefix_by_intent(const char*, const char*)",

		[](const char* prefix, const char* path) {
			auto result = prefix_by_intent(prefix, path);
			cout << "\""<< prefix <<"\" + \""<< path <<"\" => \""<< result <<"\""; // No trailing \n helps formatting the test output.
			return result;
		})
		.run( In{ "crap/", "target"  }, Expect{ "crap/target" } )
		.run( In{ "crap", "/target"  }, Expect{ "/target"     } )
		.run( In{ "crap", "/"        }, Expect{ "/"           } )
		.run( In{ "crap", ""         }, Expect{ "crap"        } )
#ifdef _WIN32
		.run( In{ "crap", "\\target" }, Expect{ "\\target"    } )
		.run( In{ "crap", "\\"       }, Expect{ "\\"          } )
		.run( In{ "crap", "c:"       }, Expect{ "c:"          } )
#endif
		.sep(" -- Empty prefix...\n")

		.run( In{ "", "target"   }, Expect{ "target"      } )
		.run( In{ "", "/target"  }, Expect{ "/target"     } )
		.run( In{ "", "/"        }, Expect{ "/"           } )
		.run( In{ "", ""         }, Expect{ ""            } )
		.run( In{ "", "./.."     }, Expect{ "./.."        } )
		.run( In{ "", "."        }, Expect{ "."           } )
#ifdef _WIN32
		.run( In{ "", "\\target" }, Expect{ "\\target"    } )
		.run( In{ "", "\\"       }, Expect{ "\\"          } )
		.run( In{ "", "c:"       }, Expect{ "c:"          } )
		.run( In{ "", ".\\.."    }, Expect{ ".\\.."       } )
#endif
	;

	Test( "prefix_by_intent(\"prefix\", ...) - `.*` cases",

		[=](const char* path) {
			auto prefix = "prefix";
			auto result = prefix_by_intent(prefix, path); //!!OLD: , enable_dot_optout);
			cout << "\""<< prefix <<"\" + \""<< path <<"\" => \""<< result <<"\""; // No trailing \n helps formatting the test output.
			return result;
		}
//!! Just sneaking in a random test for the test runner itself...:
//!!		, Stop_On_Failure
		)
		.run( In{ "."      }, Expect{ "."           } )
		.run( In{ "./.."   }, Expect{ "./.."        } )
		.run( In{ "./"     }, Expect{ "./"          } )
		.run( In{ "./keep" }, Expect{ "./keep"      } )
		.run( In{ ".."     }, Expect{ "prefix/.."   } )
		.run( In{ "../"    }, Expect{ "prefix/../"  } )
		.run( In{ ".x"     }, Expect{ "prefix/.x"   } )
		.run( In{ "..x"    }, Expect{ "prefix/..x"  } )
#ifdef _WIN32
		.run( In{ ".\\"    }, Expect{ ".\\"         } )
		.run( In{ ".\\.."  }, Expect{ ".\\.."       } )
		.run( In{ "..\\"   }, Expect{ "prefix/..\\" } )
#endif
	;

//!!	Test("prefix_by_intent: EMPTY cases", prefix_by_intent)
	Test(prefix_by_intent)

		.sep(" - EMPTY string_view prefix: ") .run( In{""sv, "tail"}, "tail" )
		.sep(" - EMPTY string      prefix: ") .run( In{""s , "tail"}, "tail" )
		.sep(" - EMPTY const char* prefix: ") .run( In{""  , "tail"}, "tail" )
		.sep(" - EMPTY string_view tail: ")   .run( In{"pre", ""sv }, "pre"sv)
		.sep(" - EMPTY string      tail: ")   .run( In{"pre", ""s  }, "pre"s )
		.sep(" - EMPTY const char* tail: ")   .run( In{"pre", ""   }, "pre"  )
		.sep(" Prefix with a trailing slash...\n")
		.sep(" - EMPTY string_view tail: ")   .run( In{"pre/", ""sv }, "pre/"sv)
		.sep(" - EMPTY string      tail: ")   .run( In{"pre/", ""s  }, "pre/"s )
		.sep(" - EMPTY const char* tail: ")   .run( In{"pre/", ""   }, "pre/"  )
#ifdef _WIN32
		.sep(" - EMPTY string_view tail: ")   .run( In{"pre\\", ""sv }, "pre\\"sv)
		.sep(" - EMPTY string      tail: ")   .run( In{"pre\\", ""s  }, "pre\\"s )
		.sep(" - EMPTY const char* tail: ")   .run( In{"pre\\", ""   }, "pre\\"  )
#endif
		.sep()
		.sep(" - BOTH EMPTY (string_view): ") .run( In{""  , ""  }, ""  )
		.sep(" - BOTH EMPTY (string):      ") .run( In{""s , ""s }, ""s )
		.sep(" - BOTH EMPTY (const char*): ") .run( In{""sv, ""sv}, ""sv)
	;

	TestStats::report();

cerr<<"\n"<< "prefix_by_intent() manual cases to smoke-test each supported type...\n\n";

	cerr<< "- string_view tail:\t"; cout << prefix_by_intent("pre/", "string_view tail"sv) << '\n';
	cerr<< "- string      tail:\t"; cout << prefix_by_intent("pre/", "string tail"s) << '\n';
	cerr<< "- const char* tail:\t"; cout << prefix_by_intent("pre/", "const char* tail") << '\n';

	cerr<< "- string_view prefix:\t"; cout << prefix_by_intent("string_view pre/"sv, "tail") << '\n';
	cerr<< "- string      prefix:\t"; cout << prefix_by_intent("string pre/"s, "tail") << '\n';
	cerr<< "- const char* prefix:\t"; cout << prefix_by_intent("const char* pre/", "tail") << '\n';

	cerr<< "- EMPTY string_view prefix:\t"; cout << prefix_by_intent(""sv, "tail") << '\n';
	cerr<< "- EMPTY string      prefix:\t"; cout << prefix_by_intent(""s, "tail") << '\n';
	cerr<< "- EMPTY const char* prefix:\t"; cout << prefix_by_intent("", "tail") << '\n';

	cerr<< "- EMPTY string_view tail:\t"; cout << prefix_by_intent("pre", ""sv) << '\n';
	cerr<< "- EMPTY string      tail:\t"; cout << prefix_by_intent("pre", ""s) << '\n';
	cerr<< "- EMPTY const char* tail:\t"; cout << prefix_by_intent("pre", "") << '\n';

	cerr<< "- BOTH EMPTY (string_view):\t"; cout << prefix_by_intent("", "") << '\n';
	cerr<< "- BOTH EMPTY (string)     :\t"; cout << prefix_by_intent(""s, ""s) << '\n';
	cerr<< "- BOTH EMPTY (const char*):\t"; cout << prefix_by_intent(""sv, ""sv) << '\n';

} // main
#endif // UNIT_TEST

#endif // _LSF39847G45796GK890G676G42GF35_
