//---------------------------------------------------------------------------
// Raw HEX + ASCII mem. dump, with a glyph map for custom UTF-8 visualization
// v1.1.0
//
// Usage examples:
//
//	MemDump().at(buffer, sizeof buffer);
//
//	MemDump::GlyphMap::Subst map[] = {
//		{0, "\xc3\x98"},  // LATIN CAPITAL LETTER O WITH STROKE (U+00D8 -> C3 98)
//	                          // (or EMPTY SET: "Ã˜", or NUL (U+2400 -> E2 90 80)
//		{C_SUBST, "→"},   // RIGHT ARROW (or just "$")
//		{C_LIST, "…"},    // ELLIPSIS (or just "@", or "â‰¡")
//		{MemDump::GlyphMap::Nonprinting, "\xc2\xb7"} // MIDDLE DOT (U+00B7), looks better than '?'
//	};
//	MemDump({map /* or other cfg opts. */}).at(buffer, bytes);
//---------------------------------------------------------------------------

#ifndef _DUMPEUBG2420C6Y357890YYYYYYYYYYYYYYYYY35M708DNFVUUIV_
#define _DUMPEUBG2420C6Y357890YYYYYYYYYYYYYYYYY35M708DNFVUUIV_

#include <stdio.h>

namespace sz {

class MemDump
{
public:
	using Byte = unsigned char;
	using Size = unsigned; // We're not duping gigabytes are we? (Also to avoid size_t; albeit
	                       // if we have printf, pretty damn sure we have size_t too, though. :) )
	//---------------------------------------------------------------------------
	// Optional byte -> displayed-char map for fancified output
	struct GlyphMap
	{
		using Key = int; constexpr static Key Nonprinting = 256;
		//!!enum Key : unsigned { Nonprinting = 256 }; //!! C++ won't cast char literals to enums! :-(
		struct Subst { Key key; const char* utf8; }; // Not raw Byte, to allow virtual mappings!
			//! Whoa! Since this is a mem dumper, testing it with itself revealed this
			//! is actually 16 bytes!... :-o For decades, I've always found a way to
			//! look away; now my sleep is gone forever! ;)
		const Subst* mappings_ = nullptr;
		unsigned     count_ = 0;
		const char*  Nonprtinting_glyph_ = nullptr;
		// Ctor to deduce the size of a literal map & cache the Nonprint. mapping, if exists:
		template <Size N>
		GlyphMap(const Subst (&map)[N]) : mappings_(map), count_(N) {
			for (unsigned i = 0; mappings_ && i < count_; ++i)
				if (mappings_[i].key == Nonprinting) { Nonprtinting_glyph_ = mappings_[i].utf8; break; }
		}
		// And then we need an explicit default, too:
		GlyphMap() = default;
		// Find glyph string (or return null):
		const char* operator [] (Key b) const {
			for (unsigned i = 0; mappings_ && i < count_; ++i) {
				if (mappings_[i].key == (Key)b) return mappings_[i].utf8;
			}
			// No match for direct byte val., check for special keys...
			return (b == Nonprinting && Nonprtinting_glyph_) ? Nonprtinting_glyph_ : nullptr;
		}
	};

	struct Config
	{
		GlyphMap    glyph_map;
		const char* line_prefix     = "";     // const char*, so don't feed it tmps.!
		bool        hex_zero_00     = false;  // " 0" or "00"?
		bool        show_pos        = true;   // Mem. addresses or offsets
		bool        pos_as_addr     = false;  // Offsets, if false
		bool        show_dec_offset = false;  // Only if show_pos (regardless of pos_as_addr!)
		bool        show_stats      = false;  // Just count zeros for now...
		bool        split_text_view = false;  // Similar to the hex middle div.
	} cfg;

protected:
	struct TextView
	{
		constexpr static Size _BUF_SIZE = 4 * 16 + 1; // Ample room for UTF-8 overhead
		char data[_BUF_SIZE];
		Size pos = 0; // UTF-8 code units written into the buffer
		void append(const char* utf8) {
			do { data[pos++] = *utf8++; } while (*utf8 && pos < _BUF_SIZE);
		/*!! Single-code-point UTF-8-checking version:
			auto lead = (unsigned char)utf8[0];
			unsigned len = (lead < 0x80) ? 1 :
				       (lead < 0xE0) ? 2 :
				       (lead < 0xF0) ? 3 : 4;
			for (unsigned i = 0; i < len; ++i) {
				if (lead >= 0x80 && !utf8[i]) { return; } //!! Freak out on premature 0...
				text_view[t++] = utf8[i];
			}
		!!*/
		}
	};

public:
	MemDump() = default;
	MemDump(Config config) : cfg(config) {}

	// Wrappers to do the `sizeof thing` automatically:
	template <typename T, Size N> // - for arrays...
	void of(const T (&array)[N]) { of((const void*)array, N * sizeof(T)); }
	template <typename T>         // - for objects... (not pointers, not arrays!)
	void of(const T& obj) { of((const void*)&obj, sizeof(T)); }

	void of(const void* mem, Size bytes_to_go)
	{
		auto addr = (const Byte*)mem;

		Size offset = 0;
		unsigned stat_zeros_seen = 0;
		unsigned dec_offset_max_width = __count_dec_digits(bytes_to_go > 0 ? bytes_to_go - 1 : 0);
		while (bytes_to_go)
		{
			TextView txt;

			// Start 16-byte chunk...

			printf("%s", cfg.line_prefix);
			if (cfg.show_pos) {
				cfg.pos_as_addr ? printf("%p", (void*)addr) : printf("%08X", offset);
				if (cfg.show_dec_offset)
				printf("(%*u)", dec_offset_max_width, offset);
				printf(":  ");
			}

			unsigned i = 0;
			for (; i < 16 && bytes_to_go; ++i, ++addr, ++offset, --bytes_to_go)
			{
				// Hex...
				if (i == 8) printf("- "); // Middle hex div.
				Byte byte = *addr;
				if (byte) { printf("%02X ", byte); }
				else      { printf(cfg.hex_zero_00 ? "00 " : " 0 "); // " 0" is easier to spot!
				            ++stat_zeros_seen; }

				// Text...
				if (i == 8 && cfg.split_text_view) txt.append(" - "); // Middle text div:

				// Give a chance to translation first (because printables can be remapped too!),
				// then display as vanilla ASCII, with '.' for nonprs...
				if (auto glyph = cfg.glyph_map[byte])  { txt.append(glyph); }
				else if (byte >= 32 && byte <= 126) { char tmp[2] = {(char)byte, 0}; txt.append(tmp); }
				else { glyph = cfg.glyph_map[GlyphMap::Nonprinting]; txt.append(glyph ? glyph : "."); }
			}

			// Pad hex & text line segments if too short (for alignment)
			for (; i < 16; ++i)
			{
				if (i == 8) { printf("  "); }                             printf("   "); 
				if (i == 8 && cfg.split_text_view) { txt.append("   "); } txt.append(" ");
			}

			txt.append("\0");
			printf(" |%s|\n", txt.data);
		} // for: total size

		if (cfg.show_stats) {
			printf("Stats: %u zeros / %u bytes (%.1f%%)\n",
		               stat_zeros_seen, offset,
		               offset ? 100.0 * stat_zeros_seen / offset : 0.0);
		}
	} // of

	// ------------------------------------------------------------------- Helpers...
	unsigned __count_dec_digits(Size n) {
		if (n == 0) return 1;
		unsigned digits = 0;
		while (n) { n /= 10; ++digits; }
		return digits;
	}
}; // MemDump

} // namespace sz

#endif // _DUMPEUBG2420C6Y357890YYYYYYYYYYYYYYYYY35M708DNFVUUIV_
