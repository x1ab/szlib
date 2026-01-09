// VERSION: See after the class def.!

#ifndef _MW4MC8907U8V9J7DYFUNHYEUIUM7HCV9ITHY9856HY985CR6MJIHBV_
#define _MW4MC8907U8V9J7DYFUNHYEUIUM7HCV9ITHY9856HY985CR6MJIHBV_

namespace sz {

struct SemVer
{
	// Plumbing...
	constexpr static const unsigned Tag_Max_Size = 100; //! NOT including the EOS!
	constexpr static bool           is_tag_sep(char c) {
		return c > ' ' && !('a' <= c && c <= 'z' || 'A' <= c && c <= 'Z'); // Reject ASCII letters, whitespace, etc.
		//!!TBD: More restrictive? Could even be just '-' || '_' || '+' || '.' || etc.,
		//!!     but that would be the "wrong kind of arbitrary"...
	}

	// The fields are all yours, free to set manually...
	unsigned major = Invalid_, minor = Invalid_, patch = Invalid_;
	char tag[Tag_Max_Size + 1] = ""; // +1 for EOS! Empty string by default. (Alas, the whole buffer gets
	                                 // zeroed, so "only pay for what you use" is a lie here... Note: you can't
	                                 // do tag[0] = '\0' without a ctor, which would kill the aggregate status...

	// "from-string" fatory:
	static SemVer parse(const char* s);

	// "from-string" converting op=, but with a name...
	// Not going all-in with a real op=, because of
	//  a) potentially confusing ambiguity with the implicit copy-op=,
	//  b) parsing also does validation, with a fail mode,
	//  c) v = "1.2.3" looks like a raw assignment; very misleading.
	auto& set(const char* s) { *this = parse(s); return *this; } // May become invalid!

	public: explicit operator bool() const // `false` means invalid
	{
		return major != Invalid_ && minor != Invalid_ && patch != Invalid_;
		// Not "each part...", so a partially successful parse will leave it invalid!
	} 

	// Version of this versioning API itself (just for dogfooding/example):
	static const SemVer VERSION;

protected:
	constexpr const static unsigned Invalid_ = ~0u;
};


inline const SemVer SemVer::VERSION = SemVer::parse("0.1.2");

//!! Alas, this doesn't compile, as the UDL function is declared later:
//!!inline const SemVer SemVer::VERSION = "1.22.333"_v;


//----------------------------------------------------------------------------
// Standalone ops...
//----------------------------------------------------------------------------

constexpr bool operator == (const SemVer& a, const SemVer& b);
// For ordering comparisons the tag is ignored!
// Also: not using <=>, as that requires an std. header! :-/
constexpr bool operator <  (const SemVer& a, const SemVer& b);
constexpr bool operator >  (const SemVer& a, const SemVer& b) { return b < a; }
constexpr bool operator <= (const SemVer& a, const SemVer& b) { return !(b < a); }
constexpr bool operator >= (const SemVer& a, const SemVer& b) { return !(a < b); }


inline SemVer operator ""_v(const char* cstr, decltype(sizeof(0)) /*len*/)
{//! a) Exactly std::size_t is required by C++ ^^^here^^^, b) no size_t without headers... Great...
	return SemVer::parse(cstr);
}


template <typename OSTREAM>
OSTREAM& operator << (OSTREAM& out, const SemVer& v);
//!!OLD:
//!!#include <iosfwd>
//!!template <typename Char, typename Traits>
//!!inline std::basic_ostream<Char, Traits>& operator<<(std::basic_ostream<Char, Traits>& out, const SemVer& v)


//============================================================================
// Impl...
//
//!! constexpr and/or templating requires keeping these in the header... :-/
//============================================================================

	namespace impl {

		// Minimalist C string compare, avoiding strcmp and <string.h> altogether:
		constexpr bool streq(const char* a, const char* b) {
			while (*a && *a == *b) { ++a; ++b; }
			return *a == *b;
		}
	}

//----------------------------------------------------------------------------
constexpr bool operator == (const SemVer& a, const SemVer& b)
{
	return  a.major == b.major && a.minor == b.minor && a.patch == b.patch
		&& impl::streq(a.tag, b.tag); //!! strcmp(a.tag, b.tag) == 0;
}

//----------------------------------------------------------------------------
constexpr bool operator < (const SemVer& a, const SemVer& b)
{
    if (a.major != b.major) return a.major < b.major;
    if (a.minor != b.minor) return a.minor < b.minor;
    return a.patch < b.patch;
}


//----------------------------------------------------------------------------
template <typename OSTREAM>
OSTREAM& operator << (OSTREAM& out, const SemVer& v)
//!! auto ... -> decltype(out.put(' '), out) // Headerless SFINAE hack to eliminate non-ostreamlike `out` types...
//!!                                // But with SemVer as RHS, what is the risk it tries to avoid, again?
{
	if (v) { out <<'['
		     << v.major <<" . "<< v.minor <<" . "<< v.patch;
		     if (*v.tag) out << " - " << v.tag;
		 out <<']';
	} else { out << "[!! INVALID VERSION !!]"; }
	return out;
}

} // namespace sz

#endif // _MW4MC8907U8V9J7DYFUNHYEUIUM7HCV9ITHY9856HY985CR6MJIHBV_
