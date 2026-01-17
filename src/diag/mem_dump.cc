//!! Not yet...: #define sz_MEMDUMP_IMPL
#include "mem_dump.hh"


//============================================================================
// g++ -std=c++20 -g -gdwarf-4 -gstrict-dwarf -fsanitize=undefined -fsanitize-trap -Wall -Wextra -pedantic -DUNIT_TEST -x c++ mem_dump.cc
// cl -std:c++20 -EHsc -Zi -fsanitize=address -W4 -DUNIT_TEST -TP mem_dump.cc
#ifdef UNIT_TEST
//#include <iostream>
//#include "../diag/test.hh"

namespace sz::test {
	auto H(const char* heading) { fprintf(stderr, "\n%s\n\n", heading); }
}

using namespace sz::test;
//using std::cout, std::cerr, std::string_view, std::string;
//using namespace std::literals;

int main()
{
	using sz::MemDump;

	MemDump vanilla_dump;

	H("Deduced obj. size:");
	vanilla_dump.of(vanilla_dump);

	H("Vanilla config, ptr + explicit size:");
	vanilla_dump.of(&vanilla_dump, sizeof vanilla_dump);

	H("UTF-8 rendering (and line prefix):");
	MemDump::GlyphMap::Subst map[] = {
		{0, "\xc3\x98"},  // LATIN CAPITAL LETTER O WITH STROKE (U+00D8 -> C3 98)
//		{0, "."},
//		{0, " "},         // Space is only practical when counting chars is not a thing!
	                          // (or EMPTY SET: "Ã˜", or NUL (U+2400 -> E2 90 80)
		{1, "→"},         // RIGHT ARROW (or just "$")
		{2, "…"},         // ELLIPSIS (or just "@", or "â‰¡")
		{MemDump::GlyphMap::Nonprinting, "\xc2\xb7"} // MIDDLE DOT (U+00B7), looks better than '?' (well-supported too)
//		{MemDump::GlyphMap::Nonprinting, "?"}
//		{MemDump::GlyphMap::Nonprinting, "_"}
	};
	MemDump decorated_dump({map, ">>> "});
	decorated_dump.of(decorated_dump);

	H("The glyph map used for UTF-8 rendering:");
	decorated_dump.cfg.hex_zero_00 = true;
	decorated_dump.of(map);

	H("Show dec offsets:");
	decorated_dump.cfg.show_dec_offset = true;
	#define _SIZE_ 1000
	static const char safe_zone[_SIZE_] = "Some legit acerage for the memory dumper to forage on...";
	decorated_dump.of(safe_zone);

	H("No positions:");
	decorated_dump.cfg.show_pos = false;
	decorated_dump.cfg.line_prefix = "    ";
	decorated_dump.of(map);

	H("Position as address (+ dec offset):");
	decorated_dump.cfg.show_pos = true;
	decorated_dump.cfg.pos_as_addr = true;
	decorated_dump.cfg.line_prefix = "";
	decorated_dump.of(map);

	decorated_dump.cfg.show_dec_offset = false;
	decorated_dump.cfg.pos_as_addr = false;

	H("Split text view:");
	decorated_dump.cfg.split_text_view = true;
	decorated_dump.of(map);

	H("No divider for 8 bytes:");
	decorated_dump.of(safe_zone, 24);

	H("Zeros in the glyph map:");
	decorated_dump.cfg.show_stats = true;
	decorated_dump.cfg.split_text_view = false;
	decorated_dump.cfg.hex_zero_00 = false;
	decorated_dump.of(map);

/* Just a copy-pasted template:
	Test( "sz::is_absolute == std::fs::path.is_absolute?",
		[](string_view p) {
				bool abs = is_absolute(p);
				cout << "\""<< p <<"\" is "<< (abs ? "absolute" : "relative"); // No trailing \n helps formatting the test output.
				return abs;
		})
		.run( In{ ""        }, Expect{ fs::path(""       ).is_absolute() } )
		.run( In{ " "       }, Expect{ fs::path(" "      ).is_absolute() } )
		.run( In{ "/"       }, Expect{ fs::path("/"      ).is_absolute() } )
	;
*/
} // main
#endif // UNIT_TEST
