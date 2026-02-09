// v0.2.1

#ifndef _XPWOEMWIERCUWEIRU3489367B207X2_
#define _XPWOEMWIERCUWEIRU3489367B207X2_

#include <cstring>
#include <cctype>  // isspace, toupper, ...
#include <cstdint> // uint32_t (<- not guaranteed to exist; let the world burn, if not defined!...)
#include <string>
#include <cassert>

namespace sz {

//---------------------------------------------------------------------------
//  Strip leading/ending whitespace (or other) chars, in-place
//
//  Returns `src`.
//
inline char* strtrim(char* src, const char* leading_junk = " \t\v\r\n\f")
{
	const char* res;
	const char* ptr;
	std::size_t newend;

	assert(src);

	// Skip leading junk...
	res = src + std::strspn(src, leading_junk);
	// Find the *last* non-space char...
	for (newend = 0, ptr = res; *ptr; ++ptr)
	{
		if (!std::isspace(*ptr))
			newend = ptr - res + 1;
	}
	// Move the string left...
	std::memmove(src, res, newend + 1);
	src[newend] = '\0';

	return src;
}

//---------------------------------------------------------------------------
inline char* strlwr(char* s)
{
	for (char* r = s; *r; ++r) *r = (char)std::tolower(*r);
	return s;
}

inline char* strupr(char* s)
{
	for (char* r = s; *r; ++r) *r = (char)std::toupper(*r);
	return s;
}

//---------------------------------------------------------------------------
inline bool escape_quotes(std::string* str, char quote = '"', char escmark = '\\')
// Intended to supplement istream >> std::quoted(...).
// And then the escape char itself must also be escaped!...
{
	bool changed = false;
	for (size_t pos = 0; pos < str->size(); ++pos) { //! str may grow, so < size() is mandatory!
		if ((*str)[pos] == quote || (*str)[pos] == escmark) {
			str->insert(pos, 1, escmark);
			++pos;
			changed = true;
		}
	}
	return changed;
}

inline bool escape_chars(std::string* str, const char* escapees, char escmark = '\\')
{
	bool changed = false;
	for (size_t pos = 0; pos < str->size(); ++pos) { //! str may grow, so < size() is mandatory!
		if (std::strchr(escapees, (*str)[pos] || (*str)[pos] == escmark)) {
			str->insert(pos, 1, escmark);
			++pos;
			changed = true;
		}
	}
	return changed;
}


//----------------------------------------------------------------------------
namespace str { enum: int { defaults, empty_is_true }; }
inline bool to_bool(const char* cstr, int flags = str::defaults)
//
// Supported false values (everything else is consireder true):
//      empty (""), 0[0...], and (mixed-case variations of) false, no, none, off.
//
{
	assert(cstr);

	if (!*cstr) return flags & str::empty_is_true;

	if (*cstr == '0') { // false also if 00000 or 0.0, but not 012 or 0.012!
		while (*++cstr)
			if (*cstr != '0' && *cstr != '.') return true;
		return false; // Meh, for 0...0 etc. :)
	}

	// Fast-track *heuristic* cheat for common "false 1st chars" (regardless of what follows):
	//!! Should revisit this on one slow, quiet day...
	if (*cstr == 'n'
	 || *cstr == 'N'
	 || *cstr == 'f'
	 || *cstr == 'F'
	) return false;

	// "off":
	if ((cstr[0] | 0x20) == 'o' && //! Bool shortcutting would prevent overread.
	    (cstr[1] | 0x20) == 'f' &&
	    (cstr[2] | 0x20) == 'f' &&
	    !cstr[3]) return false;

	/*!! This punning trick is unfortunately UB, for strict-aliasing and
	     potential misalignment reasons (it could actually fail e.g. on ARM). :-(
	     And it did trip GCC's UBsan...
	if (cstr[1] && cstr[2] && !cstr[3]) // strlen == 3
	#define _sz_u32(cstr) (*(const uint32_t*)(cstr))
		if (_sz_u32(cstr) == _sz_u32("off")
		 || _sz_u32(cstr) == _sz_u32("Off")
		 || _sz_u32(cstr) == _sz_u32("OFF")
	#undef _sz_u32
		) return false;
	!!*/

	return true;
}

inline bool to_bool(const std::string& str, int flags = str::defaults)
{
	return to_bool(str.c_str(), flags);
}


} // namespace sz

#endif // _XPWOEMWIERCUWEIRU3489367B207X2_


//============================================================================
#ifdef UNIT_TEST

#include <iostream>

using namespace sz;
using namespace std;

int main(int argc, char** argv)
{
	if (argc < 2) return false;

	std::cout << argv[1] <<" to bool: "<< to_bool(argv[1]) <<'\n';
	std::cout << "...and also with empty_is_true:\n";
	std::cout << "\"\"" <<" to bool: "<< to_bool("", sz::str::empty_is_true) <<'\n';
}

#endif
