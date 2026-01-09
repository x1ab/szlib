#define _CRT_SECURE_NO_WARNINGS 1 // Mute MSVC's "sscanf is not safe" warning.

#include "semver.hh"

//----------------------------------------------------------------------------
// Parse ("from_string")...
//----------------------------------------------------------------------------

#ifdef sz_SEMVER_PARSE_WITH_STRINGSTREAM

//----------------------------------------------------------------------------
#include <sstream>
#include <assert.h>
//#include <iostream> // Just for debugging!

namespace sz {

SemVer SemVer::parse(const char* s)
{
	SemVer v; // Invalid by default.

	assert(s);
	if (!*s) return SemVer{};

	std::istringstream iss(s);
    
	// 1. Parse Core: X.Y.Z
	// We let standard stream operators skip preceding whitespace for numbers,
	// which aligns with how %u behaves generally, but we need strict dot checking.
	char dot1 = 0, dot2 = 0;
	if (!(iss >> v.major >> dot1 >> v.minor >> dot2 >> v.patch)) {
		return SemVer{};
	}

	if (dot1 != '.' || dot2 != '.') return SemVer{};

	// 2. Parse Optional Tag
	// We must read RAW characters now to handle separators like '-' strictly,
	// and to stop exactly at whitespace.
	char sep = 0;
    
	// Try to read one character (the separator)
	if (!iss.get(sep)) {
		// EOF reached immediately after 1.2.3 -> Valid, no tag.
		return v; 
	}

	// sscanf "%c%...[^ \t\r\n]" Logic Check:
	// If we hit whitespace immediately after numbers (e.g. "1.2.3 "), 
	// sscanf reads space as 'sep'. Your is_tag_sep(' ') is TRUE (!isalnum).
	// So "1.2.3 " is technically parsed as version "1.2.3" with tag separator ' ' and empty tag.
    
	if (is_tag_sep(sep)) {
		// 3. Read Tag Content
		// We read until we hit whitespace or EOF or buffer limit.
		size_t i = 0;
		char c;
        
		// Peek to see if we are already done (case: "1.2.3-")
		if (iss.peek() == EOF) {
		// Loop won't run, tag remains empty. 
		// Logic matches sscanf n=4 case.
		} 
		else {
			while (i < Tag_Max_Size && iss.get(c)) {
				// Mimic sscanf `[^ \t\r\n]`: Stop at any whitespace
				if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
				break; 
				}
				v.tag[i++] = c;
			}
		}
		v.tag[i] = '\0'; // Cap it manually (safe because buffer is +1)

	#ifdef sz_SEMVER_FORBID_MISSING_TAG
		if (i == 0) return SemVer{};
	#endif

	} else {
		// Found junk that isn't a separator (e.g. "1.2.3a")
		return SemVer{};
	}

	return v;
}

} // namespace sz

#else

//--------------------------------------
#include <stdio.h>
#include <assert.h>
//#include <iostream> // Just for debugging!

namespace sz {

SemVer SemVer::parse(const char* s)
{
	SemVer v; // Invalid by default.
	char tag_sep;

	assert(s);
	if (!*s) return SemVer{};

	// Prep. the format string first with the tag buf size...
	char fmt[100]; snprintf(fmt, sizeof(fmt), "%%u.%%u.%%u%%c%%%u[^ \t\r\n]", SemVer::Tag_Max_Size);

	// Parse...
//!!Ref:int n = sscanf(s, "%u.%u.%u%c%127[^ \t\r\n]", &v.major, &v.minor, &v.patch, &tag_sep, v.tag);
	int n = sscanf(s, fmt, &v.major, &v.minor, &v.patch, &tag_sep, v.tag);

//std::cerr <<"n = "<< n <<", tag sep.: '"<< tag_sep <<"', format str: "<< fmt <<'\n';

	// Validate...
	if (n == 3) { // VER only
	} else if (n == 5 || n == 4) { // VER-TAG
		if (!v.is_tag_sep(tag_sep)) v = SemVer{}; // Invalidate!

	#ifdef sz_SEMVER_FORBID_MISSING_TAG
		if (!*v.tag) v = SemVer{}; // Invalidate "1.2.3-" forms!
	#endif
	} else { // Incomplete — if x.y, then auto-extend to x.y.0; ortherwise invalid!
		//!!if (n == 2 && ... Alas, there's no way to tell with sscanf if it stopped due to
		//!!                  an error, or just a premature EOS!... :-(
		assert(!v);
	}

	return v;
}

} // namespace sz

#endif



//----------------------------------------------------------------------------
#ifdef UNIT_TEST

#include <iostream>

int main(int argc, char** argv)
{
	using namespace sz;
	using namespace std;

cerr<<"\n" << "Init" <<"...\n";

	SemVer v = SemVer::parse(argv[argc-1]);
	cout<<"\t" << v <<'\n';
	cout<<"\t" << SemVer{10,2,3} <<'\n';         // Who needs ctors with aggreg. init?! ;)
	cout<<"\t" << SemVer{10,2,3, "rc-2"} <<'\n'; // ...tagged, too!
	cout<<"\t" << "9.6.9-from-C++literal"_v <<'\n';

cerr<<"\n" << "Implicit copy-op=" <<":\n";

	auto v2 = v;
	cout<<"\t" << v2 <<'\n';

cerr<<"\n" << "Smoke-test op==" <<":\n";
	cout<<"\t" << v <<" == "<< v2 <<"?... "<< std::boolalpha<< (v == v2) <<'\n';

cerr<<"\n" << "Converting ops" <<":\n";
	cout<<"\t" << v2.set("1.22.333") <<'\n';

cerr<<"\n" << "Edge cases" <<"...\n";
	cout<<"\t" << v2.set("0.0.0") <<'\n';

// Compile error:
//	0 << SemVer{};
}

#endif //UNIT_TEST
