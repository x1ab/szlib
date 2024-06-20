#define StringSequence // Kludge to replace messing with templates...
// { INTERFACE ===============================================================

/*!! Hell no!...
#include <type_traits>
template <class T>
// Even this would already be cumbersome, but it doesn't even compile yet...
concept StringSequence = std::is_same_v<T, std::vector<std::string>> ||
                         std::is_same_v<T, std::array<std::string>>  ||
//                       std::is_same_v<T, std::list<std::string>>   ||
//                       std::is_same_v<T, std::deque<std::string>>  ||
//                       ...
                         std::is_array<T>); // plain old C array
// And it's not just string, but string_view, and who knows what else.
// And not to mention the compilation time cost, for just a simple little
// function like this!...
!!*/

#include <string>
namespace sz {
inline std::string listvals(StringSequence auto const& container,
	const char prewrap[] = "", const char postwrap[] = "",
	const char sep[] = ", ",
	const char* quote = "\"", // not const char[], to hint that it accepts nullptr!
	const char* scary_chars = " \t\n");
} // namespace sz

// } INTERFACE ===============================================================


//#ifdef SZ_IMPLEMENTATION //=================================================
//	Cutting off the IMPL. secetion is disabled, as this is a template
//	function, so it must be included! (And I'm not gonna bother with .inl
//	files.)

#include <string_view>
#include <cstring>
#include <cassert>

namespace sz {
using namespace std;

#define FOUND(expr) ((expr) != std::string::npos)
#define CONTAINS(str, chars) FOUND((str).find_first_of(chars))
string listvals(StringSequence auto const& container, const char prewrap[], const char postwrap[], const char sep[],
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
} // namespace sz

//#endif // SZ_IMPLEMENTATION //===================================================
#undef StringSequence
