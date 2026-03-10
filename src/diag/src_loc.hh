// v0.20.0

/*
!! Should take constexpr static config options maybe (e.g. instead of _sz_NO_MSVC_CLEANUP_)?!
*/
#ifndef XC8DIYFHUCWED5V98TY7M9SDVH8WCR0VIYNMJH
#define XC8DIYFHUCWED5V98TY7M9SDVH8WCR0VIYNMJH


#include <source_location>
#include <string_view>


//----------------------------------------------------------------------------
// Config...
//----------------------------------------------------------------------------

//#define _sz_NO_MSVC_CLEANUP_  // Don't strip std::source_location.function_name()
                                // (A test compilation took 175 vs. 165 s when enabled.)

namespace sz {

#if defined(_MSC_VER) && !defined(_sz_NO_MSVC_CLEANUP_)

	// ---- Simplistic compile-time MSVC function signature cleaner ----
	//      Typical MSVC format: `return_type __cdecl function_name(args)`
	//      - member functions:  `return_type __cdecl MyClass::function_name(args)`
	//	Alas, spaces also in: ... `template_func<arg1, arg2>(args)`!
	//	And "premature" parens in `operator()()`!
	//!!
	//!! WOULD BE MUCH CLEANER AND CHEAPER TO JUST USE __FUNCTION__ WITH MSVC INSTEAD!
	//!! But that would require reverting to a top-level macro API. :-(
	//!!
	constexpr std::string_view strip_function_name(std::string_view full_name) {
		// Find the '(' of the arg. list... - Try to recognize operator()() too!
		auto end = full_name.find('(');
		if (end == std::string_view::npos) return full_name; // Should not happen, but...
		if (full_name.substr(end).starts_with("()()")) return full_name.substr(0, end + 2);

		// Try to skip the template args list, too, if present...
		// (And avoid mixing it up with the < and << operators!)
		auto templ_end = full_name.find_first_of('<'); //!! .find_first_of("<", 0, end) is not constexpr in MSVC! (GCC's fine.)
		if (templ_end != std::string_view::npos
			&& templ_end > 0 && full_name[templ_end-1] != 'e' // Not "template<"?
		) end = templ_end;

		// Reverse search for the space before the function name
		// (Should work for both members and free functions.)
		auto start = full_name.rfind(' ', end);
		// Fallback for unusual ones with no preceding space:
		if (start == std::string_view::npos) return full_name.substr(0, end);
		
		return full_name.substr(start + 1, end - (start + 1));
	}

	// ---- "consteval wrapper" struct for source_location ---
	//      Mimics std::source_location, but with cleaned-up function name.
	struct StrippedSourceLoc {
		const int ln;
		const int col;
		const std::string_view fn;
		const std::string_view file;

		int                    line()          const noexcept { return ln; }
		int                    column()        const noexcept { return col; }
		const std::string_view function_name() const noexcept { return fn; }
		const std::string_view file_name()     const noexcept { return file; }

		// The magic happens here: a consteval constructor that does all the work at compile time.
		// It takes the compiler-provided std::source_location and "fixes" the function name.
		consteval StrippedSourceLoc(
			const std::source_location& loc = std::source_location::current()
		) :
			ln(loc.line()),
			col(loc.column()),
			fn(strip_function_name(loc.function_name())), // Use our compile-time parser
			file(loc.file_name())
		{}

		static consteval StrippedSourceLoc current(
			const std::source_location& loc = std::source_location::current()
		) { return StrippedSourceLoc(loc); }
	};

	using src_loc = StrippedSourceLoc;

#else

	using src_loc = std::source_location;

#endif

} // namespace sz


#endif // XC8DIYFHUCWED5V98TY7M9SDVH8WCR0VIYNMJH
