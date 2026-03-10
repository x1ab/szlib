// 0.1.1
// C++20 for fn(auto)

#include <string>
#include <charconv> // to_chars, C++17
#include <cstdint>  // uintptr_t
#include <cassert>
//#include <type_traits> // Uncomment if you accept the reality that you've
                         // already paid for those `requires`es below anyway
                         // and now you want to enable them after all, because
                         // <type_traits> absolutely *has* been pulled in a
                         // few times already... ;)

//----------------------------------------------------------------------------
//
// NOTE: Doesn't 0-pad the result, lengths vary!
//
// We need a template for also accepting non-object (i.e. function) pointers!
//static_assert(std::is_convertible_v<void(*)(), void*>); // MSVC accepts it, but non-std!
inline std::string ptr_to_string(auto* p) // - `auto` needed (and therefore C++20), because
                                          //   void* wouldn't pick up function ptrs
                                          // - but not just `auto`: auto* to require pointers
                                          // - but not `const auto*`: templ. ded. covers that!
                                          // - `inline` is optional here, as this is already a template
// With just plain `auto p` above, you could also do this, if willing to include <type_traits>:
//	requires (std::is_pointer_v<decltype(p)>
//	       || std::is_null_pointer_v<decltype(p)>) // add this if you want to accept nullptr!
{
	using namespace std; // everything's std in here

	char buf[sizeof(uintptr_t) * 2]; // - 2 hex chars per byte
	                                 // - EOS not needed for the string ctor
	                                 // - zeroing not needed for to_chars
	auto [past_last, ec] = to_chars(buf,
		buf + sizeof(buf),       // or end(buf), but you'd need #include <iterator> for that, technically
		reinterpret_cast<uintptr_t>(p), // fn* -> uniptr is condit. supported: if compiles, it's legit!
		16
	);
	assert(ec == errc{}); // to_chars should never fail in this case!
	return string(buf, past_last);
};
// This is to support nullptr with the auto* signature above (and disabled `requires`):
inline auto ptr_to_string(decltype(nullptr)) { return ptr_to_string((void*)0); }


#ifdef UNIT_TEST
#include <iostream>

int main()
{
	// Objects:
	std::cout << ptr_to_string(&std::cin) << std::endl;
	// Also null:
	std::cout << ptr_to_string(nullptr) << std::endl;
	std::cout << ptr_to_string((void*)0) << std::endl;

	// Functions:
	//std::cout << ptr_to_string(main) << std::endl; // Well, main is sacred, can't utter its name! :-o
	//std::cout << ptr_to_string(ptr_to_string) << std::endl; // Umm, and this is a template, and an overload set now... :)
	// Well, fuckit:
	std::cout << ptr_to_string(+[]{}) << std::endl;

	// Wrong arg. types:
	//std::cout << ptr_to_string(0) << std::endl;
	//std::cout << ptr_to_string(1) << std::endl;
	//std::cout << ptr_to_string(std::cin) << std::endl;
}
#endif // UNIT_TEST
