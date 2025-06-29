//============================================================================
// Error handilng & (user-facing) reporting utilities
//
// (For introspection/debug tools see e.g. log.hh, DBG.hh etc.)
//============================================================================

#ifndef _CM029874Y687G43YT078GN6RTCF039VM74B5NTYN798_
#define _CM029874Y687G43YT078GN6RTCF039VM74B5NTYN798_


//----------------------------------------------------------------------------
// Config...
//----------------------------------------------------------------------------

//#define _sz_ERROR_USE_MEMCPY_ // instead of std::string, for storing the message in exceptions


//----------------------------------------------------------------------------
// API...
//----------------------------------------------------------------------------
//!
//! See the actual call signatures near the end of the file!
//!
//----------------------------------------------------------------------------

#include <string_view>
#include <exception> // Since <string_view> is already included, std::exception comes for free...
// To support ""s at concatenating literals for the function parameters
#include <string>
using namespace std::string_literals;
//! <sstream> would be nice for << support, but that's almost <iostream>-level heavy.
//! Maybe integrating plog's nostream instead? (This is a sibling of sz/log anyway!)

// Note:
//
//  - The `""s + ...` hack is to support concatenating const char* literals directly
//    in calls like `Note("Found " + filename)` (instead of `"Found "s + ...`,
//    which is easy to forget, ugly, and requires <string> + using the string_literals
//    namespace, too).
//
//!! The vararg. syntax is not implemented yet! (Mostly due to the legacy
//!! preprocessor still being the default in MSVC, complicating things.)
//!!
//!! And the even more flexible fmt syntax is not supported because of the heavy
//!! dependency of std::format. (Error.hpp is assumed to be included for many TUs,
//!! so any significant compilation burden here could be painful.) The current
//!! general policy is _removing_ std C++ dependencies, not adding new ones...
//!!
//!! Add behavior, too, not just reporting!
//!! ABORT already does abort, but FATAL should too, and ERROR could also _optionally_ throw something!
//!!
//
/*
#define Note(msg, ...)        ( sz::NOTE_impl    ({__FUNCTION__, __LINE__}, ""s + msg) )
#define Warning(msg, ...)     ( sz::WARNING_impl ({__FUNCTION__, __LINE__}, ""s + msg) )
#define Error(msg, ...)       ( sz::ERROR_impl   ({__FUNCTION__, __LINE__}, ""s + msg) )
#define Fatal(msg, ...)       ( sz::FATAL_impl   ({__FUNCTION__, __LINE__}, ""s + msg) )
#define Bug(msg, ...)         ( sz::BUG_impl     ({__FUNCTION__, __LINE__}, ""s + msg) )
#define Abort(...)            ( sz::ABORT_impl   ({__FUNCTION__, __LINE__} __VA_OPT__(,) __VA_ARGS__) ) //! Requires -Zc::preprocessor for MSVC
*/

//!! A subtle loss for #623 is not being able to apply the nice concat. support hack of `""s + msg`:
//void bug(string_view msg, std::_source_location loc = std::_source_location::current());


//----------------------------------------------------------------------------
// Impl...
//----------------------------------------------------------------------------

#include "src_loc.hh"

#ifdef _sz_ERROR_USE_MEMCPY_
# include <cstring> // memcpy
#else
# include <string>
#endif


namespace sz {

// ---- FatalError exception...
#ifdef _sz_ERROR_USE_MEMCPY_
struct FatalError : std::exception { //! That inheritance makes it no longer an aggregate; good bye simple init, hello required ctor...
                                     //! However, for copying the message over to the local buffer, we'd need a ctor anyway!
	constexpr static auto MsgMaxLen = 255;
	char message[MsgMaxLen + 1];
	src_loc loc;
	const char* what() const noexcept override { return message; } // Just to align with std a bit...
	FatalError(std::string_view msg, const src_loc& loc) :
		loc(loc)
	{
		auto len = msg.length() < MsgMaxLen ? msg.length() : MsgMaxLen;
		std::memcpy(message, msg.data(), len);
		message[len] = '\0';
	}
};
#else
struct FatalError : std::exception { //! That inheritance makes it no longer an aggregate; good bye simple init, hello required ctor...
                                     //! However, for copying the message over to the local buffer, we'd need a ctor anyway!
	std::string message;
	src_loc loc;
	const char* what() const noexcept override { return message.c_str(); } // Just to align with std a bit...
	FatalError(std::string_view msg, const src_loc& loc) : message(msg), loc(loc) {}
};
#endif

void NOTE_impl    (const src_loc& loc, std::string_view message = "", ...);
void WARNING_impl (const src_loc& loc, std::string_view message = "", ...);
void ERROR_impl   (const src_loc& loc, std::string_view message = "", ...);
void FATAL_impl   (const src_loc& loc, std::string_view message = "", ...);
void BUG_impl     (const src_loc& loc, std::string_view message = "", ...);
void ABORT_impl   (const src_loc& loc, std::string_view message = "", ...);


//----------------------------------------------------------------------------
// OK, the API now, for reeal...
//----------------------------------------------------------------------------
inline void Note   (std::string_view msg     , const src_loc& loc = src_loc::current()) { NOTE_impl   (loc, msg); }
inline void Warning(std::string_view msg     , const src_loc& loc = src_loc::current()) { WARNING_impl(loc, msg); }
inline void Error  (std::string_view msg     , const src_loc& loc = src_loc::current()) { ERROR_impl  (loc, msg); }
inline void Fatal  (std::string_view msg     , const src_loc& loc = src_loc::current()) { FATAL_impl  (loc, msg); }
inline void Bug    (std::string_view msg     , const src_loc& loc = src_loc::current()) { BUG_impl    (loc, msg); }
inline void Abort  (std::string_view msg = "", const src_loc& loc = src_loc::current()) { ABORT_impl  (loc, msg); }

} // namespace sz

// Support existing macro-based code:
using sz::Note;
using sz::Warning;
using sz::Error;
using sz::Fatal;
using sz::Bug;
using sz::Abort;


#endif // _CM029874Y687G43YT078GN6RTCF039VM74B5NTYN798_