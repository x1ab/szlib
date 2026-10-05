#include "error.hh"

//!!
//!! The dependency injection for logging is currently "handled" by expecting the
//!! LOG* macros to have been defined by the caller (#includer)! :-o
//!!
//!!#include "log.hh"
//!!

#include <sstream>
	using std::ostringstream;
#include <iostream>

//using namespace std; //! If log.hh was included with SZ_LOG_REPLACE_IOSTREAM (e.g. by an adapter/wrapper,
                       //! like in O2N...), this IS NOT ENOUGH to pin the cerr/endl calls to std! :-o
                       //! Better just to qualify them individually...

//!!
//!! NOTE:
//!!
//!! - Now the source location is blatantly included in the user-facing messages too, but that shouldn't be the case!
//!! - However, the actual location *can't* be forwarded to the LOG macros, which would just grab the fixed locations here,
//!!   so, as a workaround, the orig. locations are embedded into the log messages themselves! :-/
//!! - Also, we assume here that the logger _may_ add its own ERROR/WARNING etc. prefixes or severity tags (risking omission).
//!! - Similarly, we assume that the logger _may not_ add the source location already (so we do it here, risking duplication).
//!!

namespace sz {
using namespace err; //!! Can't do namespace err {...} yet, as only some parts are declared in that!

//!! No, this doesn't work either (comp. error: conflicting `using`...):
//!!using std::cerr, std::endl; //! Must be inside sz::, to override any SZ_LOG_REPLACE_IOSTREAM shenanigans!

namespace {
	auto src_loc_suffix = [](const src_loc& loc, const char* preposition = "at") {
		return ( ostringstream()
			<< " ("<<preposition <<" "
			<< loc.function_name() << ":" << loc.line()
			<< ")"
		).str();
	};
}

void NOTE_impl (const src_loc& loc, std::string_view message, ...)
{
// The source location is omitted from the plain cerr output:
//	auto msg = ostringstream() << message << src_loc_suffix(loc);

	LOGN      << message << src_loc_suffix(loc);
	std::cerr << "Note: " << message << std::endl;
}

void WARNING_impl (const src_loc& loc, std::string_view message, ...)
{
	auto msg = ostringstream() << message << src_loc_suffix(loc);

	LOGW      << msg.str();
	std::cerr << "Warning: " << msg.str() << std::endl;
}

void ERROR_impl   (const src_loc& loc, std::string_view message, ...)
{
	auto msg = ostringstream() << message << src_loc_suffix(loc);

	LOGE      << msg.str();
	std::cerr << "- ERROR: " << msg.str() << std::endl;
}

void FATAL_impl   (const src_loc& loc, std::string_view message, ...)
{
	auto msg = ostringstream() << message << src_loc_suffix(loc);

	LOGF      << msg.str();
	std::cerr << "- FATAL ERROR: " << msg.str() << std::endl;

	Abort();
}

void BUG_impl   (const src_loc& loc, std::string_view message, ...)
{
	auto msg = ostringstream()
		<< "\n" // Don't just assume we're on a new line already; break away forcefully
		        // (at the cost of an extra empty line in the most common case)...
		<< "**************** INTERNAL ERROR" << src_loc_suffix(loc) << ":\n"
		<< message << "\n"
		<< "****************\n";

	LOGE  //!!<< "\n" // Guarantee an empty line after a possible "ERROR: " log entry prefix
	      //!!        // — but there would be *two* empty lines if no prefix; awkward!...
	          << msg.str();
	std::cerr << msg.str() << std::endl;

	//!! Bug() may need a flag to abort on its own (like Fatal), or a FatalBug() is needed!
}

void ABORT_impl   (const src_loc& loc, std::string_view message, ...)
{
	const char* prefix = "--- ABORTING";
	if (message != "") {
//		if (loc) {
			LOG       << message << src_loc_suffix(loc);
			std::cerr << prefix << ": " << message << src_loc_suffix(loc) << std::endl;
//		} else {
//			LOG       << message;
//			std::cerr << prefix << ": " << message << std::endl;
//		}
	} else {
		LOG       << prefix << "...";
		std::cerr << prefix << "..." << std::endl;
	}

	switch (cfg_abort_method) {
		case AbortMethod::Abort: std::abort();
		case AbortMethod::Exit:  std::exit(cfg_abort_exit_code);
//!!??		case AbortMethod::Debug: ...??
		case AbortMethod::Throw:
		default:                 throw FatalError(message, loc);
			// This requires a try{} block around main, with a matching catch (FatalError& or std::exception)!
			//!! ALSO: The app may not even have exceptions enabled!!
	}

}

} // namespace sz