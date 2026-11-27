// 0.3.3-from-MycoUI-0.105.0

#include "to_BDF.hh"

#include <stdint.h> // *Real* uint8_t, uint32_t, etc. font_def only has (internal) "approximations" of these!
#include <sstream>
#include <iomanip>
#include <iostream> // For warnings/errors
#include <assert.h>

namespace sz {
namespace font {

// NOTE: constexpr vars have internal linkage, and constexpr functions are `inline`, so ODR should be fine.

//----------------------------------------------------------------------------
std::string to_BDF(const font_def& def)
// BDF uses Y-down coordinates. (Baseline is indexed from TOP.)
// It seems to tolerate -1 for `ENCODING` at unmapped chars.
{
	const auto UNIODE_NONCHAR = 0xffff; // Most natural "legit" Unicode "Noncharacter"
	const auto DEFAULT_CHAR_CODEPOINT = UNIODE_NONCHAR; // Will be the value of "DEFAULT_CHAR"

	[[maybe_unused]] const auto MAX_WIDTH = 32;
	assert(def.WIDTH <= MAX_WIDTH); //!! Or a runtime check?!

	const auto BITMAP_BYTES_PER_LINE = (def.WIDTH + 7) >> 3; // `>> 3` for `/ 8`
	const auto BITMAP_BITS_PER_LINE = BITMAP_BYTES_PER_LINE * 8;
    
	auto ascent  = def.ASCENT;
	auto descent = def.DESCENT;

	if (def.HEIGHT != def.ASCENT + def.DESCENT) { // #743
		std::cerr << "[font::to_BDF] - Warning: HEIGHT ("<<def.HEIGHT
		     <<") != ASCENT ("<<def.ASCENT<<") + DESCENT ("<<def.DESCENT<<")! Expect problems!\n"
		     << "                 Font: \""<< (def.NAME ? def.NAME : "(unnamed)") <<"\""
		     << std::endl;
	}

	std::stringstream bdf;
	bdf
	<< "STARTFONT 2.2\n"
	//!! Fix the "Fixed" part!...:
        << "FONT -Custom-Fixed-Medium-R-Normal--" << def.HEIGHT << "-" << def.HEIGHT * 10
		<< "-75-75" // X11 defaults; hopefully ignored even by X (but for font *selection* it
		            // should still match its expected 75?), so should be OK not matching SIZE!...
		<< "-" << def.SPACING_MODE << "-"
		/*
		C (Character Cell / Fixed):

		- Every glyph occupies the same bounding box
		  Width is truly identical for all characters
		  (Like a terminal font - perfect grid alignment.)

		M (Monospace):

		- Every glyph has the same advance width (cursor movement)
		- But the actual glyph can be narrower and positioned within that space
		- Looks better than C because glyphs can have optical spacing
		  Example: 'i' might be 3px wide but sits in a 7px cell, centered

		P (Proportional):

		- Each glyph has its own custom advance width:
		  'i' advances 3px, 'W' advances 9px, etc.
		- Not aligned to a grid
		*/
		<< def.WIDTH * 10 //!! Should be avg. width!
		<< "-ISO10646-1\n"
        << "SIZE " << def.HEIGHT << " 72 72\n" //! Wow, critical! 75 here skewed the height of a 13-px-high font to 14! :-o
        << "FONTBOUNDINGBOX " << def.WIDTH << " " << def.HEIGHT << " 0 " << -descent << "\n"
        << "STARTPROPERTIES 5\n"
        << "FONT_ASCENT " << ascent << "\n"
        << "FONT_DESCENT " << descent << "\n"
        << "CHARSET_REGISTRY \"ISO10646\"\n"
        << "CHARSET_ENCODING \"1\"\n" // Note: tried 2 with ISO8859 above (assuming -> ISO 8859-2), but it killed the font.)

	<< "DEFAULT_CHAR " << DEFAULT_CHAR_CODEPOINT << "\n" //!! Alas, not an index, so INVALID_GLYPH_INDEX doesn't work here...
	                                                     //!! Also, font_def::Invalid_Codepoint (u32(-1)) didn't work either.
        << "ENDPROPERTIES\n"
        << "CHARS " << (int)def.GLYPH_COUNT << "\n";

	for (font_def::Index i = 0; i < def.GLYPH_COUNT; ++i) { // `auto i` is fine, but needs a cast below.
		auto codepoint = def.glyph_codepoint(i);

		// Assign BDF's fake "Default-Char" codepoint to our special "invalid" glyph (if defined):
		if (i == def.INVALID_GLYPH_INDEX || codepoint == font_def::Invalid_Codepoint)
			codepoint = UNIODE_NONCHAR;

		auto const glyph_index = codepoint != font_def::Invalid_Codepoint ? i : def.INVALID_GLYPH_INDEX;
		auto const glyph_start = glyph_index * def.HEIGHT * BITMAP_BYTES_PER_LINE;
        
		bdf
		<< "STARTCHAR U+" << std::hex << std::setw(4) << std::setfill('0') << codepoint << "\n"
		<< std::dec // CRITICAL: Reset to decimal!
		<< "ENCODING " << codepoint << "\n"
		<< "SWIDTH 1000 0\n"  // Scalable width, 1000 is arbitrary, but common. Nominally:
		                      // SWIDTH = DWIDTH * 72000 / (SIZE * x-resolution-from-SIZE)
		<< "DWIDTH " << def.WIDTH << " 0\n" //!! Same for each only if def.FIXED_WIDTH!
		<< "BBX " << def.WIDTH << " " << def.HEIGHT << " 0 " << -descent << "\n"
		<< "BITMAP\n";

		for (unsigned row = 0; row < def.HEIGHT; ++row) {
			auto line_start = glyph_start + row * BITMAP_BYTES_PER_LINE;

			uint32_t line_bits = 0;
			for (auto bytes = 0; bytes < BITMAP_BYTES_PER_LINE; ++bytes) {
				uint8_t byte = ((uint8_t*)def.GLYPH_BITMAPS)[line_start + bytes];
				// Little-endianness assumed:
				line_bits = line_bits | (uint32_t(byte) << bytes*8);
			}
			line_bits <<= BITMAP_BITS_PER_LINE - def.WIDTH; // BDF takes bits from the MSB!

			bdf << std::hex << std::setw(2 * BITMAP_BYTES_PER_LINE) << std::setfill('0') << line_bits << "\n";
		}
		bdf << "ENDCHAR\n";
//!!		bdf << std::dec << "ENDCHAR\n"; // Reset to decimal
	}
	bdf << "ENDFONT\n";

	return bdf.str();
}

} // namespace font
} // namespace sz
