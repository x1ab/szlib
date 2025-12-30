/*
!! Should take constexpr static config options maybe (e.g. instead of _sz_NO_MSVC_CLEANUP_)?!
*/
#ifndef _XC8DIYFHUCWED5V98TY7M9SDVH8WCR0VIYNMJH_
#define _XC8DIYFHUCWED5V98TY7M9SDVH8WCR0VIYNMJH_


#include <source_location>
#include <string_view>


//----------------------------------------------------------------------------
// Config...
//----------------------------------------------------------------------------

//#define _sz_NO_MSVC_CLEANUP_  // Don't strip std::source_location.function_name()
                                // (A test compilation took 175 vs. 165 s when enabled.)

namespace sz {

#if defined(_MSC_VER) && !defined(_sz_NO_MSVC_CLEANUP_)

	// ---- Compile-Time MSVC Function Signature Cleaner ---
	//      Typical MSVC format: `return_type __cdecl function_name(args)`
	//      - member functions:  `return_type __cdecl MyClass::function_name(args)`
	constexpr std::string_view strip_function_name(std::string_view full_name) {
		// Find the ( of the arg. list... - Would fail for operator()()!!
		auto end = full_name.find('(');
		if (end == std::string_view::npos) return full_name; // Should not happen, but...

		// Reverse search for the space before the function name
		// (Should work for both members and free functions.)
		auto start = full_name.rfind(' ', end);
		// Fallback for unusual names with no preceding space:
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


#endif // _XC8DIYFHUCWED5V98TY7M9SDVH8WCR0VIYNMJH_
