// v0.3.0

#ifndef SZSTRXPWOEMWIERCUWEIRU3489367B207X2
#define SZSTRXPWOEMWIERCUWEIRU3489367B207X2

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


} // namespace sz

#endif // SZSTRXPWOEMWIERCUWEIRU3489367B207X2


//============================================================================
#ifdef UNIT_TEST

#include <iostream>

using namespace sz;
using namespace std;

int main(int argc, char** argv)
{
	if (argc < 2) return false;

	cerr <<  "- ERROR: NO TESTS YET! :(\n";
}

#endif
