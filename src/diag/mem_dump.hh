//---------------------------------------------------------------------------
// Raw HEX + ASCII mem. dump, with a glyph map for custom UTF-8 visualization
// v1.2.0
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
//		{MemDump::GlyphMap::Nonprinting, "\xc2\xb7"} // MIDDLE DOT (U+00B7)
//	};
//	MemDump({map /* or other cfg opts. */}).at(buffer, bytes);
//---------------------------------------------------------------------------

#ifndef _DUMPEUBG2420C6Y357890YYYYYYYYYYYYYYYYY35M708DNFVUUIV_
#define _DUMPEUBG2420C6Y357890YYYYYYYYYYYYYYYYY35M708DNFVUUIV_

#include <stdio.h>
#include <assert.h>

namespace sz {

class MemDump
{
public:
	using Byte = unsigned char;
	using Size = unsigned; // We're not dumping gigabytes are we? (Although... no real gain in
	                       // avoiding size_t here: it pretty much comes free with printf anyway.)
	//---------------------------------------------------------------------------
	// Optional byte -> displayed-char map for fancified output
	struct GlyphMap
	{
		using Key = int; constexpr static Key Nonprinting = 256;
		//!!enum Key : unsigned { Nonprinting = 256 }; //!! C++ won't cast char literals to enums! :-(
		struct Subst { Key key; const char* utf8; }; // Not raw Byte, to allow virtual mappings!
			//! Whoa! Since this is a mem dumper, testing it with itself revealed that
			//! this is actually 16 bytes!... :-o For decades, I've always found a way to
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
		// ...but then we need to declare the default manually:
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
		GlyphMap       glyph_map;
		const char*    line_prefix     = "";     // const char*, so don't feed it tmps.!
		unsigned short bytes_per_line  = 16;
		unsigned short groups_per_line = 2;      // How many byte groups per line? (0 is invalid, but taken as 1...)
		unsigned short grouped_bytes   = 0;      // Nr. of bytes in a group; 0 = auto from groups_per_line. (If both !0, grouped_bytes wins!)
		bool           group_text_too  = false;  // Grouping/splitting of text follows the hex layout?
		const char*    hex_group_sep   = "- ";   // In addition to the normal spacing between `01 23 ...`!
		const char*    text_group_sep  = " - ";  // (No default spacing between chars to consider.)
		const char*    text_edge       = "|";
		const char*    text_edge_l     = nullptr;
		const char*    text_edge_r     = nullptr;

		bool           show_hex        = true;
		bool           show_text       = true;
		bool           show_pos        = true;   // Mem. addresses or offsets
		bool           pos_as_addr     = false;  // Offsets, if false
		bool           show_dec_offset = false;  // Only if show_pos (regardless of pos_as_addr!)
		bool           show_stats      = false;  // Just count zeros for now...

		bool           hex_zero_00     = false;  // " 0" or "00"?

		// Computed:
		/*!! OLD:
		constexpr unsigned short group_size() const {
			// Edge case to avoid e.g. 4 - 4 - 1 for 9 / 2... (doing 5 - 4, or 4 - 5 instead):
			// Odd, and groups_per_line is set, but grouped_bytes isn't?
			if (bytes_per_line % groups_per_line && groups_per_line && !grouped_bytes)
				return (bytes_per_line + groups_per_line - 1) / groups_per_line;
			return grouped_bytes ? grouped_bytes : bytes_per_line / groups_per_line;
			//!! Actually, this alone handles the even case, too:
			//!!return (bytes_per_line + groups_per_line - 1) / groups_per_line;
			//!! ...but haven't bothered to think through if the `groups_per_line && !grouped_bytes`
			//!! precondition still applies unchanged. Now LERPing anyway, not using this fn; see _group_div_pos!
		}
		!!*/
		constexpr bool split_hex_view()    const { return grouped_bytes || groups_per_line > 1; }
		constexpr bool split_text_view()   const { return split_hex_view() && group_text_too; }
		constexpr auto hex_sep_pad_len()   const { return split_hex_view()  ? _strlen(hex_group_sep)  : 0; }
		constexpr auto text_sep_pad_len()  const { return split_text_view() ? _strlen(text_group_sep) : 0; }
		constexpr auto _hex_sep_pad_len()  const { return _strlen(hex_group_sep); }
		constexpr auto _text_sep_pad_len() const { return _strlen(text_group_sep); }
	} cfg;

protected:
	struct TextView
	{
		constexpr static Size _BUF_SIZE = 4 * 16 + 1; // Ample room for UTF-8 overhead
		char data[_BUF_SIZE];
		Size pos = 0; // UTF-8 code units written into the buffer
		constexpr void append(const char* utf8) {
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
		unsigned dec_offset_max_width = _count_dec_digits(bytes_to_go > 0 ? bytes_to_go - 1 : 0);
		auto _hex_sep_blanks = cfg.hex_sep_pad_len();
		auto _txt_sep_blanks = cfg.text_sep_pad_len();
		while (bytes_to_go)
		{
			TextView txt;

			// Start chunk of bytes_per_line...

			printf("%s", cfg.line_prefix);
			if (cfg.show_pos) {
				cfg.pos_as_addr ? printf("%p", (void*)addr) : printf("%08X", offset);
				if (cfg.show_dec_offset)
				printf("(%*u)", dec_offset_max_width, offset);
				printf(":  ");
			}

			unsigned i = 0;
			for (; i < cfg.bytes_per_line && bytes_to_go; ++i, ++addr, ++offset, --bytes_to_go)
			{
				Byte byte = *addr;
				// Hex...
				if (cfg.show_hex) {
					if (cfg.split_hex_view() && _group_div_pos(i)) printf("%s", cfg.hex_group_sep);
					if (byte) { printf("%02X ", byte); }
					else      { printf(cfg.hex_zero_00 ? "00 " : " 0 "); // " 0" is easier to spot!
						++stat_zeros_seen; }
				}
				// Text...
				if (cfg.show_text) {
					if (cfg.split_text_view() && _group_div_pos(i)) txt.append(cfg.text_group_sep);
					// Give a chance to translation first (because printables can be remapped too!),
					// then display as vanilla ASCII, with '.' for nonprs...
					if (auto glyph = cfg.glyph_map[byte]) { txt.append(glyph); }
					else if (byte >= 32 && byte <= 126) { char tmp[2] = {(char)byte, 0}; txt.append(tmp); }
					else { glyph = cfg.glyph_map[GlyphMap::Nonprinting]; txt.append(glyph ? glyph : "."); }
				}
			} // loop: line

			// Pad hex & text line segments if too short (for alignment)
			for (; i < cfg.bytes_per_line; ++i)
			{
				if (cfg.show_hex) {
					if (cfg.split_hex_view() && _group_div_pos(i)) { printf("%*s", _hex_sep_blanks, ""); }
					printf("   "); // "00 "
				}
				if (cfg.show_text) {
					if (cfg.split_text_view() && _group_div_pos(i)) {
						for (auto p = _txt_sep_blanks; p; --p) txt.append(" ");
					}
					txt.append(" "); // 1 char
				}
			} // loop: padding after last line fragment

			txt.append("\0");
			printf(" %s%s%s\n", cfg.text_edge_l ? cfg.text_edge_l : cfg.text_edge,
			                        txt.data,
			                    cfg.text_edge_r ? cfg.text_edge_r : cfg.text_edge);
		} // loop: total size

		if (cfg.show_stats) {
			printf("Stats: %u zeros / %u bytes (%.1f%%)\n",
		               stat_zeros_seen, offset,
		               offset ? 100.0 * stat_zeros_seen / offset : 0.0);
		}
	} // of

	// ------------------------------------------------------------------- Helpers...
	bool _group_div_pos(unsigned i) {
	//!!OLD, relies on group_size(), which doesn't ensure optimally even groups:
	//!!	return i && !(i % cfg.group_size());
		assert(cfg.split_hex_view());
		if (!i) return false;
		if (cfg.grouped_bytes) return i % cfg.grouped_bytes == 0;

	#if 0 // "Front-load" (largest group first):
		auto prev_group = (int)(((i - 1) * cfg.groups_per_line) / (float)cfg.bytes_per_line);
		auto curr_group = (int)(( i      * cfg.groups_per_line) / (float)cfg.bytes_per_line);
	#else // "Back-load" (— tested OK, but haven't scrutinized for off-by-one edge cases!):
		auto prev_group = (int)(( (cfg.bytes_per_line - i)     * cfg.groups_per_line) / (float)cfg.bytes_per_line);
		auto curr_group = (int)(( (cfg.bytes_per_line - i - 1) * cfg.groups_per_line) / (float)cfg.bytes_per_line);
	#endif
		return curr_group != prev_group && curr_group < cfg.groups_per_line;
	}

	static constexpr auto _strlen = [](const char* s) { unsigned len = 0; while (s[len]) ++len; return len; };

	static constexpr unsigned _count_dec_digits(Size n) {
		if (n == 0) return 1;
		unsigned digits = 0;
		while (n) { n /= 10; ++digits; }
		return digits;
	}
}; // MemDump

} // namespace sz

#endif // _DUMPEUBG2420C6Y357890YYYYYYYYYYYYYYYYY35M708DNFVUUIV_
