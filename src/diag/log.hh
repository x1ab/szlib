#ifndef _SDDOSIFUGHODIUGHUC4V509485767N80YTG_
#define _SDDOSIFUGHODIUGHUC4V509485767N80YTG_


//----------------------------------------------------------------------------
// MACRO API
//----------------------------------------------------------------------------

// SZ_LOG_DISABLE:               Redirect the logging macros to a null sink stream
// SZ_NO_LOG_MACROS:             Only keep the SZ_LOG... macros.
// SZ_LOG_KEEP_THE_DEBUG_MACROS_DESPITE_NDEBUG
// SZ_LOG_USE_DEFAULT_LEVEL:     Change the default filtering level (from `info`)
// SZ_LOG_REPLACE_IOSTREAM:      Use a crude, lightweight iostream replacement

#define SZ_LOGF SZ_LOG_INIT_AND_EXEC(PLOGF)
#define SZ_LOGE SZ_LOG_INIT_AND_EXEC(PLOGE)
#define SZ_LOGW SZ_LOG_INIT_AND_EXEC(PLOGW)
#define SZ_LOGN SZ_LOG_INIT_AND_EXEC(PLOGI)
#define SZ_LOGI SZ_LOG_INIT_AND_EXEC(PLOGD)
#define SZ_LOGD SZ_LOG_INIT_AND_EXEC(PLOGV)
#define SZ_LOG  SZ_LOGN

#ifndef SZ_NO_LOG_MACROS
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

// This convoluted hack is needed because the PLOG... macros are an open if()...else!
#define SZ_LOG_INIT_AND_EXEC(PlogMacro) \
    if (((void)sz::log::LogMan::instance(), false)) {;} \
    else PlogMacro


//----------------------------------------------------------------------------
// Prep. & get the embedded PLOG sources:
#define PLOG_CHAR_AS_NARROW
#define PLOG_CHAR_IS_UTF8 1
#define PLOG_OMIT_LOG_DEFINES
#include "plog/Log.h"
#include "plog/Init.h"
//#include "plog/Appenders/RollingFileAppender.h"  //!! Pulls in <iostream> (via the console appender), unfortunately!
//#include "plog/Appenders/ColorConsoleAppender.h" //!! Pulls in <iostream>, unfortunately!
// For custom writers/formatters:
#include "plog/Appenders/IAppender.h"

//----------------------------------------------------------------------------
// Prep. our side of the sources...
#ifndef  SZ_LOG_REPLACE_IOSTREAM
#  include <iostream> // *Sigh...*
#  define SZ_LOG_IOS_NS std
#else
#  include "../streams.hh"
#  define SZ_LOG_IOS_NS sz
   namespace sz {
	constexpr const char* endl = "\n"; // No flush() feature in sz/streams yet! :-/
	inline OStream cout{stdout}, cerr{stderr}; // Init must happen before the first stream op.!
   }
#endif // SZ_LOG_REPLACE_IOSTREAM

#include <string> // snprintf

#ifdef SZ_LOG_DEBUG_ITSELF
# include <iostream>
#endif



//----------------------------------------------------------------------------
// SZ_LOG DEFINITIONS & IMPL.
//----------------------------------------------------------------------------

#ifndef SZ_LOG_BUILD
# define SZ_LOG_INLINE inline
#else
# define SZ_LOG_INLINE
#endif

namespace sz {
namespace log {

enum Level : int
{
	_invalid_ = 0, // E.g. for checking with `if (!level) ...` after conversions

	fatal   = 1,   // 'F'
	error   = 2,   // 'E'
	warning = 3,   // 'W'
	notice  = 4,   // 'N' - "normal", "neutral" (the default — see init()!)
	info    = 5,   // 'I' - debug info
	detail  = 6,   // 'D' - debug detail

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


SZ_LOG_INLINE Level letter_to_level(char code);
SZ_LOG_INLINE char level_to_letter(Level level);
SZ_LOG_INLINE const char* level_to_str(Level level, bool full_form = false);


//----------------------------------------------------------------------------
// BACKEND ADAPTER & UTILITY CODE
//----------------------------------------------------------------------------

namespace internal {

	// Mapping from the hardcoded plog severity codes to Szim log levels
#ifdef _MSC_VER
#   pragma warning(suppress: 26812) //  Prefer 'enum class' over 'enum'
#endif
	SZ_LOG_INLINE Level level(const plog::Record& record);
	SZ_LOG_INLINE plog::Severity level_to_severity(Level l);

	//--------------------------------------------------------------------
	class plog_Szim_ConsoleAppender : public plog::IAppender
	{
	public:
		SZ_LOG_INLINE void write(const plog::Record& record) override;
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

	//--------------------------------------------------------------------
	class plog_Szim_FileAppender : public plog::IAppender
	{
		FILE* f = nullptr;
	public:
		SZ_LOG_INLINE explicit plog_Szim_FileAppender(const char* filename, const char* fmode = "a");
		SZ_LOG_INLINE ~plog_Szim_FileAppender();
		SZ_LOG_INLINE void write(const plog::Record& record) override;
	};

	//--------------------------------------------------------------------
/*!!
	class plog_Szim_RollingFileAppender : public plog::IAppender
	{
	public:
		void write(const plog::Record& record) override {

			//!!...
		}
	}
!!*/

	//----------------------------------------------------------------------------
	inline auto plog_instance(/*!! instance_ID = ...!!*/) {
		return plog::get(/*!! instance_ID !!*/);
	}

	//----------------------------------------------------------------------------
	// Dummy sink for the disabled macros to still allow the LOG << ... syntax:
	struct NullStream {
		template<typename T> NullStream& operator<<(const T& /*unused*/) { return *this; }
	};

} // namespace internal


//----------------------------------------------------------------------------
// API
//----------------------------------------------------------------------------
// NOTES:
//
// - Any public call would implicitly auto-initialize the logging system with
//   default settings, if it hasn't been initialized yet.
// - Thread-safe, mutex-guarded init (via the `Initializer` inner class)
//----------------------------------------------------------------------------
class LogMan
{
public:
	// Configuration parameterrs for initializing the logger
	struct Cfg
	{
		Level filter_level = normal;
		std::string target; // empty means console
		std::string fopen_mode = "a"; // use "w+" to truncate
	};

private:
	static inline bool initialized = false;
	static inline plog::util::Mutex init_mutex; // Nice: PLOG has its own lightweight mutex impl.; we can avoid #include <mutex>!

	// Utility class to encapsulate the thread-safe init logic
	class Initializer
	{
	public:
		static void init(const Cfg& cfg, bool explicit_init) {
			plog::util::MutexLock lock(init_mutex);
			if (!initialized) {
				__do_init(cfg);
				initialized = true;
				__report_after_init(cfg, explicit_init);
			}
#ifdef SZ_LOG_DEBUG_ITSELF
else { std::cerr << "LogMan already initialized (tried level: "<< cfg.filter_level <<")\n"; }
#endif
		}
	private:
		//--------------------------------------------------------------------
		// - These helpers are ONLY ever called from within a locked critical section,
		//   so no need to worry about thread safety here!
		//
		// -  They also need to be static, because this entire class is used also inside ctor
		//    (not just LogMan::init), i.e. when there's no fully constructed instance yet!
		static void __do_init(const Cfg& cfg) {
			plog::IAppender* appender = nullptr;
			if (cfg.target.empty()) {
				static internal::plog_Szim_ConsoleAppender console_appender;
				appender = &console_appender;
			} else {
				static internal::plog_Szim_FileAppender file_appender(cfg.target.c_str(), cfg.fopen_mode.c_str());
				appender = &file_appender;
			}
#ifdef SZ_LOG_DEBUG_ITSELF
std::cerr << "LogMan: calling plog::init(filter_level = "<< cfg.filter_level <<")...\n";
#endif
			plog::init(static_cast<plog::Severity>(cfg.filter_level), appender);
		}

		static void __report_after_init(const Cfg& cfg, bool explicit_init) { // true: non-default explicit init
			// NOTE:
			// - NO SZ_LOG... here: the init sequence has not finished yet! :-o (Using them would
			//   be a recursion into a locked section with no lock release ever...)
			// - The default filter level (set in the ctor) and the level used here (LOGN, a.k.a
			//   PLOGI now) should be kept consistent!
			if (!explicit_init) {
				//!!SZ_LOGN — see the comment above!
				PLOGI << "Logger implicitly initialized with defaults.";
			} else {
				//!! Query directly from the logger, don't just repeat the "wishful config"!
				const char* target = cfg.target.empty() ? "console" : cfg.target.c_str();
				//!!SZ_LOGN — see the comment above!
				PLOGI << "Logger initialized. (Filter level: " << level_to_str(cfg.filter_level, true)
						<< " (" << level_to_letter(cfg.filter_level) << ")"
						<< ", target: " << target
						<< ")";
			}
		}
	}; // class Initializer

public:
	//--------------------------------------------------------------------
	// The default ctor. is triggered by the first call of instenca(), and
	// will perform default initialization, but ONLY if an explicit init()
	// hasn't been called yet.
	explicit LogMan() {
		Cfg default_cfg;
#ifdef NDEBUG
		default_cfg.filter_level = normal;
#else
		default_cfg.filter_level = debug;
#endif
		Initializer::init(default_cfg, false); // false: default implicit init
	}

	//--------------------------------------------------------------------
	// Primary, explicit initializer for the logging facility
	static void init(const Cfg& cfg) {
		Initializer::init(cfg, true); // true: non-default explicit init
	}

	//--------------------------------------------------------------------
	// Returns a pointer to an initialized instance.
	// Implicitly triggers default init (via the constructor) if called
	// for the first time, and init() has not been called before.
 	static LogMan* instance(/*!! instance_ID = ... !!*/) { //!! Reconcile the semantics with plog_instance() above!...
		static LogMan logman;
		return &logman;
	}

	//--------------------------------------------------------------------
	// Setup method to change the log level at run-time
	//!! There will be more like this (as well as a setup(cfg), possibly even supporting retrageting)!
	static void set_level(/*!! instance_ID = ..., !!*/Level level) {
		instance()->__set_level(level);
		if (level >= notice) {
			SZ_LOGN   << "Log level updated to \"" << level_to_str(level, true)
			          << "\" (" << level_to_letter(level) << ")";
		} else {
			SZ_LOG_IOS_NS::cerr << "Log level updated to \"" << level_to_str(level, true)
			          << "\" (" << level_to_letter(level) << ")" << SZ_LOG_IOS_NS::endl;
		}
	}

protected:
	//--------------------------------------------------------------------
	void __set_level(Level level) {
		internal::plog_instance(/*!!instance_ID!!*/)->setMaxSeverity(internal::level_to_severity(level));
	}


	//--------------------------------------------------------------------
	// Prevent accidental copying:
	LogMan(const LogMan&) = delete;
	LogMan& operator=(const LogMan&) = delete;
	LogMan(LogMan&&) = delete;
	LogMan& operator=(LogMan&&) = delete;

}; // class LogMan


} // namespace log
} // namespace sz


#ifndef SZ_LOG_BUILD
#include "log.cc"
#endif


#endif // _SDDOSIFUGHODIUGHUC4V509485767N80YTG_
