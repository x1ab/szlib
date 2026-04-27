// v0.3.0
/*
  False values:
	- regex 0[0.] (e.g. 0, 0.00, or even 0...0!)
	- anything(!) starting with f/F or n/N (for false, no, none, null, nil...)
	- "off" (in any ASCII letter case)
	- regex dis.+ (in any ASCII letter case)
*/

#ifndef SZSTRTOBOOLXPWOEMWIERCUWEIRU2348256767
#define SZSTRTOBOOLXPWOEMWIERCUWEIRU2348256767


#include <string>
//!#include <string_view> // Can't just add a string_view overload, as the core fn. needs a 0-term. C-string!
#include <cassert>


namespace sz {


//----------------------------------------------------------------------------
namespace str {                               //!! Make this less barbaric!...
	enum: int {
		defaults,
		empty_is_true
	};
}
inline bool to_bool(const char* cstr, int flags = str::defaults)
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

	// "dis":
	if ((cstr[0] | 0x20) == 'd' && //! Bool shortcutting would prevent overread.
	    (cstr[1] | 0x20) == 'i' &&
	    (cstr[2] | 0x20) == 's' &&
	    cstr[3]) return false;


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

//----------------------------------------------------------------------------
inline bool to_bool(const std::string& str, int flags = str::defaults)
{
	return to_bool(str.c_str(), flags);
}


} // namespace sz


#endif // SZSTRTOBOOLXPWOEMWIERCUWEIRU2348256767


//============================================================================
// cl /nologo /EHsc /std:c++latest /Tp to_bool.hh /DUNIT_TEST
#ifdef UNIT_TEST

#include <iostream>

using namespace sz;
using namespace std;

int main(int argc, char** argv)
{
	auto p = [](auto s, int flags = str::defaults){
		std::cout <<"to_bool("<< s <<") -> "<< to_bool(s, flags) <<'\n';
	};

	if (argc < 2) {
		cerr << "Usage: <exe> str\n";

		std::cout << "...anyway, \"\" should be true with `empty_is_true`:\n";
		p("", sz::str::empty_is_true);

		std::cout << "...and false without:\n";
		p("", ~sz::str::empty_is_true);

		return 1;
	}

	auto arg = argv[1];
	p(arg);
}

#endif
