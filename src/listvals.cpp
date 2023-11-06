// { INTERFACE ===============================================================
#include <string>
namespace Sz {
inline std::string listvals(auto const& container,
	const char prewrap[] = "", const char postwrap[] = "",
	const char sep[] = ", ",
	const char* quote = "\"", // not const char[], to hint that it accepts nullptr!
	const char* scary_chars = " \t\n");
} // namespace Sz
// } INTERFACE ===============================================================


//#ifdef SZ_IMPLEMENTATION //======================================================
#include <string_view>
#include <cstring>
#include <cassert>

namespace Sz {
using namespace std;

#define FOUND(expr) ((expr) != std::string::npos)
#define CONTAINS(str, chars) FOUND((str).find_first_of(chars))
string listvals(auto const& container, const char prewrap[], const char postwrap[], const char sep[],
	const char* quote, const char* scary_chars)
{
	string result;
	if (!container.empty()) {
		size_t QLEN = quote ? strlen(quote) : 0;
		// Precalc. size... (Note: we're processing cmd args. We got time.)
		size_t size = strlen(prewrap) + (container.size() - 1) * strlen(sep) + strlen(postwrap);
		for (auto& v : container)
			size += v.length()
				+ (quote && *quote && CONTAINS(v, scary_chars) ? // add quotes...
					(QLEN>1 ? QLEN:2) : 0); // special case for 1 (-> pair)!
		result.reserve(size);
		// Write...
		result += prewrap;
		for (auto v = container.begin(); v != container.end(); ++v) {
			if (quote && *quote && CONTAINS(*v, scary_chars))
				{ result += string_view(quote, quote + (QLEN/2 ? QLEN/2 : 1)); // special case for 1 quote!
				  result += *v;
				  result += string_view(quote + QLEN/2); }
			else    { result += *v; }
			result += (++v == container.end() ? postwrap : sep); --v;
		}
		assert(result.length() == size);
	}
	return result;
}
#undef FOUND
#undef CONTAINS
}
//#endif // SZ_IMPLEMENTATION //===================================================
