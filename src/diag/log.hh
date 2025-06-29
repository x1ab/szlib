#ifndef _SDDOSIFUGHODIUGHUC4V509485767N80YTG_
#define _SDDOSIFUGHODIUGHUC4V509485767N80YTG_


//----------------------------------------------------------------------------
// MACRO API
//----------------------------------------------------------------------------

// SZ_NO_LOG_MACROS:             Only keep the SZ_LOG... macros.
// SZ_LOG_KEEP_THE_DEBUG_MACROS_DESPITE_NDEBUG

#define SZ_LOGF PLOGF
#define SZ_LOGE PLOGE
#define SZ_LOGW PLOGW
#define SZ_LOGN PLOGI
#define SZ_LOGI PLOGD
#define SZ_LOGD PLOGV
#define SZ_LOG  SZ_LOGN

#ifndef SZ_NO_LOG_MACROS // Defining these (or not) is an app responsibility, but... a courtesy...:
# ifndef SZ_LOG_DISABLE
#  define LOGF SZ_LOGF
#  define LOGE SZ_LOGE
#  define LOGW SZ_LOGW
#  define LOGN SZ_LOGN
# ifdef NDEBUG
#  ifdef SZ_LOG_KEEP_THE_DEBUG_MACROS_DESPITE_NDEBUG
#   define LOGI SZ_LOGI
#   define LOGD SZ_LOGD
#  else // SZ_LOG_KEEP_THE_DEBUG_MACROS_DESPITE_NDEBUG
#   define LOGI (sz::log::internal::NullStream())
#   define LOGD (sz::log::internal::NullStream())
#  endif // SZ_LOG_KEEP_THE_DEBUG_MACROS_DESPITE_NDEBUG
# else // NDEBUG
#  define LOGI SZ_LOGI
#  define LOGD SZ_LOGD
# endif // NDEBUG
#  define LOG  LOGN // This probably needs its own control macro, being in a whole other class of "offensive"... ;)
# else // !SZ_LOG_DISABLE
#  define LOGF (sz::log::internal::NullStream())
#  define LOGE (sz::log::internal::NullStream())
#  define LOGW (sz::log::internal::NullStream())
#  define LOGN (sz::log::internal::NullStream())
#  define LOGI (sz::log::internal::NullStream())
#  define LOGD (sz::log::internal::NullStream())
#  define LOG  LOGN
# endif // !SZ_LOG_DISABLE 
#endif // SZ_NO_LOG_MACROS


//----------------------------------------------------------------------------
// PLOG ADAPTER
//----------------------------------------------------------------------------

#define PLOG_CHAR_AS_NARROW
#define PLOG_CHAR_IS_UTF8 1
#define PLOG_OMIT_LOG_DEFINES

#include "plog/Log.h"

//!!?? Why does it work just fine with the wrong init header?! :-o
//!!?? The file version of plog::init() even expects a filename (with no default)! :-o
//!!??#include "extern/plog/Initializers/RollingFileInitializer.h"
#include "plog/Initializers/ConsoleInitializer.h"

//#include "plog/Appenders/RollingFileAppender.h"
//#include "plog/Appenders/ColorConsoleAppender.h"
// For custom writers/formatters:
#include "plog/Appenders/IAppender.h"
#include <cstdio> // snprintf
#include <ctime> // tm, strftime


namespace sz {
namespace log {

enum Level : int
{
	_invalid_ = 0, // E.g. for checking with `if (!level) ...` after conversions

	fatal   = 1,
	error   = 2,
	warning = 3,
	notice  = 4,   // "neutral" or "normal" (the default — see init()!)
	info    = 5,   // debug info
	detail  = 6,   // debug detail

	max = detail,

	// Allow preconfiguring the default level. (Alas, can't call the ID itself "default"; thanks, C/C++!... ;-p )
#ifdef SZ_LOG_USE_DEFAULT_LEVEL
	normal  = SZ_LOG_USE_DEFAULT_LEVEL,
#else	
	normal  = notice,
#endif	
	// The "debug cut" level, for disabling the LOGI... and LOGD... macros by NDEBUG:
	debug   = info,   // Levels above this are intended for debugging only.
};

inline Level letter_to_level(char code)
{
	switch (code) {
	case 'F':   return fatal;   // F
	case 'E':   return error;   // E
	case 'W':   return warning; // W
	case 'N':   return notice;  // N - "NOTICE": Default ("neutral") level, avoid the clutter! (Lower (debug) levels are disabled for NDEBUG!)
	case 'I':   return info;    // I - "DEBUG INFO", or just "INFO"
	case 'D':   return detail;  // D - "DEBUG DETAIL", or just "DETAIL" or "DEBUG"
	default:      return _invalid_; //!! Make it "" if it's not actually a bug! (Dunno plog enough to decide.)
	}
}

inline char level_to_letter(Level level)
{
	switch (level) {
	case fatal:   return 'F';
	case error:   return 'E';
	case warning: return 'W';
	case notice:  return 'N';
	case info:    return 'I';
	case detail:  return 'D';
	default:      return '?';
	}
}

inline const char* level_to_str(Level level, bool verbose = false)
{
	switch (level) {
	case fatal:   return "FATAL";
	case error:   return "ERROR";
	case warning: return "WARNING";
	case notice:  return verbose ? "NOTICE"       : ""; // Default level — declutter the most common case!
	case info:    return verbose ? "DEBUG INFO"   : "DEBUG";
	case detail:  return verbose ? "DEBUG DETAIL" : "DETAIL";
	default:      return "?";
	}
}


namespace internal {

	// Mapping from the hardcoded plog severity codes to Szim log levels
#ifdef _MSC_VER
#   pragma warning(suppress: 26812) //  Prefer 'enum class' over 'enum'
#endif
	static Level level(const plog::Record& record)
	{
		return static_cast<Level>( record.getSeverity() );
	}

	static plog::Severity level_to_severity(Level l)
	{
		return static_cast<plog::Severity>(l);
	}

	class plog_Szim_ConsoleAppender : public plog::IAppender
	{
	public:
		void write(const plog::Record& record) override {

			// [timestamp]
			char time_buf[40]; // = ""; not needed, as long as it's always populated!
			std::tm t;
			plog::util::localtime_s(&t, &record.getTime().time);
			// Writing the full date to the console would be pretty stupid:
			//std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &t);
			std::strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &t);

			// [thread ID]
			char tid_buf[20] = "";
			if (level(record) >= debug) { // Would be too much screen clutter for "normal"!
				snprintf(tid_buf, sizeof(tid_buf), " [%u]",
					(unsigned)(record.getTid()));
			}

			// [SEVERITY] — only below 'warning': warnings/errors get formatted to say so themselves!...
			char level_buf[15] = "";
			const char* severity_str = level_to_str(level(record));
			if (level(record) <= warning) {
				snprintf(level_buf, sizeof(level_buf), " [%s]",
					severity_str);
			}

			// Origin (location): [func@line] — only for debug levels!
			char loc_buf[40] = ""; // Long fn. names will be truncated!
			const char* func_ptr = record.getFunc();
			std::string func_str = func_ptr ? func_ptr : ""; // Just being null-safe...
			if (level(record) >= info) {
				snprintf(loc_buf, sizeof(loc_buf), " [%.32s@%zu]", //! Note the .32 str length limit!
					func_str.c_str(), record.getLine());
			}
		
			// Message (possibly prefixed/postfixed)
			const char* msg_ptr  = record.getMessage();
			std::string msg_str  = msg_ptr ? msg_ptr : "";   // Just being null-safe...
			const char* msg_prefix = "";
			const char* msg_suffix = "";
			switch (level(record)) {
			case warning: msg_prefix = "WARNING: "; break;
			case error:   msg_prefix = "- ERROR: "; break;
			case fatal:   msg_prefix = "--==<< FATAL ERROR: "; msg_suffix = " >>==--"; break;
			default:;
			}

			// Colors...
			const char* color_start = "";
			const char* color_end = "\x1B[0m";

//!!		if (the output is redirected to a file...) { //!! Pro'ly can't detect that here, but an entirely
							//!! different Appender should be picked at init?
			// Disable colors
			//color_end = "";
//!!		} else {
// Black: 30, Red: 31, Green: 32, Yellow: 33, Blue: 34, Magenta: 35, Cyan: 36, White: 37
#if defined(_WIN32)
			switch (level(record)) {
			case fatal:   color_start = "\x1B[96m"; break;
			case error:   color_start = "\x1B[91m"; break; //! 31 is too dark in my Win10 Terminal. :-/
			case warning: color_start = "\x1B[93m"; break; //! 33 literally looks like crap in my Win10 Terminal. :-/
			case notice:  color_start = "\x1B[97m"; break; // Default level!
			case info:    color_start = "\x1B[92m"; break; // "debug info"
			case detail:  color_start = "\x1B[32m"; break; // "debug detail"
			default: color_end = ""; break;
			}
#else // Linux term. colors might be be better balanced in gerneral, so let them differ:
			switch (level(record)) {
			case fatal:   color_start = "\x1B[91m"; break;
			case error:   color_start = "\x1B[31m"; break;
			case warning: color_start = "\x1B[33m"; break;
			case notice:  color_start = "\x1B[97m"; break; // Default level!
			case info:    color_start = "\x1B[92m"; break; // "debug info"
			case detail:  color_start = "\x1B[32m"; break; // "debug detail"
			default: color_end = ""; break;
			}
#endif
//!!		} // colors enabled?

			fprintf(stderr,
				"%s"
					"[%s.%03d] %s%s%s%s%s%s"
				"%s\n",
				color_start,
					time_buf, record.getTime().millitm,
					"", //level_buf, // Severity tags clutter the screen; look better in files!
					msg_prefix,
					msg_str.c_str(),
					msg_suffix,
					tid_buf,
					loc_buf,
				color_end
			);
			fflush(stderr); //!!?? Should be optional (configurable)?
		} // write()

	}; // class plog_Szim_ConsoleAppender


/*!!?? Not needed on modern Windows configs any more?
	// Helper to enable ANSI escape codes on Windows 10+ consoles
	// Call this once at the beginning of your main() function.
	void enableAnsiColorsOnWindowsConsole()
	{
#ifdef _WINDOWS_ // Proceed only if windows.h has already been included!
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hOut == INVALID_HANDLE_VALUE) {
			return;
		}
		DWORD dwMode = 0;
		if (!GetConsoleMode(hOut, &dwMode)) {
			return;
		}
		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(hOut, dwMode); // Errors are ignored for brevity
#endif
	}
??!!*/

	//----------------------------------------------------------------------------
	inline auto plog_instance(/*!! instance_ID = ...!!*/)
		{ return plog::get(/*!! instance_ID !!*/); }


	//----------------------------------------------------------------------------
	// Dummy sink for the disabled macros to still allow the LOG << ... syntax:
	struct NullStream {
		template<typename T> NullStream& operator<<(const T& /*unused*/) { return *this; }
	};
} // namespace internal


//----------------------------------------------------------------------------
// API
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//!! Integrate into LogMan!...
inline void init(Level filter_level = normal)
{
	static internal::plog_Szim_ConsoleAppender plogAppender;

	//!!?? Not needed on modern Windows configs any more?!
	//!!??diag::enableAnsiColorsOnWindowsConsole();

	plog::init(static_cast<plog::Severity>(filter_level), &plogAppender);
/*
	LOGD << "debug detail log entry" << " test";
	LOGI << "debug info log entry" << " test";
	LOGN << "notice log entry" << " test";
	LOGW << "warning log entry" << " test";
	LOGE << "error log entry" << " test";
	LOGF << "fatal error log entry" << " test";
	LOG  << "default log entry" << " test";
*/
}


//----------------------------------------------------------------------------
struct LogMan {
        LogMan() { instance(); }
	inline static LogMan* instance();

	void set_level(/*!! instance_ID = ..., !!*/Level level) {
		internal::plog_instance(/*!!instance_ID!!*/)->setMaxSeverity(internal::level_to_severity(level));
		if (level >= notice) {
			SZ_LOGN   << "Log level updated to \"" << level_to_str(level, true) // true: verbose
			          << "\" (" << level_to_letter(level) << ")";
		} else {
			std::cerr << "Log level updated to \"" << level_to_str(level, true) // true: verbose
			          << "\" (" << level_to_letter(level) << ")" << std::endl;
		}
	}
};

namespace internal { static inline LogMan _logman_instance; }

	inline LogMan* LogMan::instance() {
		// This static bool ensures plog::init is effectively called only once
		// by this auto-initialization mechanism, even if the header were processed
		// in a way that might theoretically try to construct this more than once
		// (which 'inline static' should prevent for the object itself).
		static bool initialized = false;

		if (!initialized) {
#ifdef NDEBUG
			auto default_log_level = normal;
#else	
			auto default_log_level = debug;
#endif
			// Open the default log(s — well, only the console for now...):
			init(default_log_level);
//			init(default_log_level, "Szim-debug.log");
			
			// Optional: Add a console appender for immediate visibility
			// static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
			// if (plog::get()) { // Check if get() returns a valid logger
			//    plog::get()->addAppender(&consoleAppender);
			// }

			//!! Add condition to switch to `std::cerr << ... << std::endl;` if
			//!! default_log_level ever gets too tight to include log::notices!
			SZ_LOGN << "Default logger initialized. ("
				<< "Filter level: " << level_to_str(default_log_level, true) // true: verbose
				<< " (" << level_to_letter(default_log_level) << ")"
				//<< ", file: " << AUTO_PLOG_FILENAME
				<< ")";
			
			initialized = true;
		}
		return &internal::_logman_instance; //!! Not yet having a state at all, but preparing for that with this...
        }


} // namespace log
} // namespace sz


#endif // _SDDOSIFUGHODIUGHUC4V509485767N80YTG_
