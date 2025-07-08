#include "log.hh"

// For implementing the custom formatterd
#include <cstdio> // snprintf
#include <ctime> // tm, strftime


namespace sz {
namespace log {


SZ_LOG_INLINE Level letter_to_level(char code)
{
	switch (code) {
	case 'F':   return fatal;   //
	case 'E':   return error;   //
	case 'W':   return warning; //
	case 'N':   return notice;  // "NOTICE": Default ("neutral") level, avoid the clutter! (Lower (debug) levels are disabled for NDEBUG!)
	case 'I':   return info;    // "DEBUG INFO", or just "INFO"
	case 'D':   return detail;  // "DEBUG DETAIL", or just "DETAIL" or "DEBUG"
	default:      return _invalid_;
	}
}

SZ_LOG_INLINE char level_to_letter(Level level)
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

SZ_LOG_INLINE const char* level_to_str(Level level, bool full_form)
{
	switch (level) {
	case fatal:   return "FATAL";
	case error:   return "ERROR";
	case warning: return "WARNING";
	case notice:  return full_form ? "NOTICE"       : "."; // Default level — declutter the most common case!
	case info:    return full_form ? "DEBUG INFO"   : "DBG";
	case detail:  return full_form ? "DEBUG DETAIL" : "DBG...";
	default:      return "?"; //!! Make it "" if it's not actually a bug! (Dunno plog enough to decide.)
	}
}


namespace internal
{
	SZ_LOG_INLINE Level level(const plog::Record& record)
	{
		return static_cast<Level>( record.getSeverity() );
	}

	SZ_LOG_INLINE plog::Severity level_to_severity(Level l)
	{
		return static_cast<plog::Severity>(l);
	}


	SZ_LOG_INLINE void plog_Szim_ConsoleAppender::write(const plog::Record& record) //override
	{
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


	SZ_LOG_INLINE plog_Szim_FileAppender::plog_Szim_FileAppender(const char* filename, const char* fmode)
	{
#ifdef _MSC_VER
		fopen_s(&f, filename, fmode);
#else
		f = fopen(filename, fmode);
#endif
	}

	SZ_LOG_INLINE plog_Szim_FileAppender::~plog_Szim_FileAppender()
	{
		if (f) { fclose(f); }
	}

	SZ_LOG_INLINE void plog_Szim_FileAppender::write(const plog::Record& record) //override
	{
		if (!f) return;

		std::tm t;
		plog::util::localtime_s(&t, &record.getTime().time);
		char time_buf[40];
		strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &t);

		fprintf(f, "[%s.%03d] [%s] %s [%s@%zu] [%u]\n", //!! fixed len: [%-5s]
			time_buf,
			record.getTime().millitm,
			level_to_str(level(record), false), // false: abbrev. levels < warning
			record.getMessage(),
			record.getFunc(), record.getLine(),
			(unsigned)record.getTid()
		);
		fflush(f); //!! THIS SOHULD BE CONFIGURABLE, TOO!...
	}

} // namespace internal


} // namespace log
} // namespace sz
