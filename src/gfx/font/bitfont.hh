// 0.5.0

#ifndef FDSN789C4MH89UYU958EJ9UKGU0V57KBH9045
#define FDSN789C4MH89UYU958EJ9UKGU0V57KBH9045


namespace sz {
namespace font {

// NOTE: constexpr vars have internal linkage, and constexpr functions are `inline`, so ODR should be fine.

//----------------------------------------------------------------------------
struct font_def
{
	//!! NOT A BINARY STABLE FORMAT (YET)! Not even the FORMAT_VERSION
	//!! property itself is binary-queryable!... :) :-o :-/

	// Zero-include *approximation* of sized integer types
	using u32       = unsigned int;
	using u16       = unsigned short;
//	using i16       = short;
	using u8        = unsigned char;
//	using i8        = signed char;

	// Semantic types
	using Index     = u16;
	using Byte      = u8;
	using Codepoint = u32;

	static constexpr Codepoint Invalid_Codepoint = (Codepoint)-1;

	//--------------------------------------------------------------------
	// Properties...

	// Housekeeping
	u16 FORMAT_VERSION = 2;
	u16 FONT_VERSION = 1;
	const char* NAME = ""; // For UI presentation, diagnostics, etc.

	// Geometry...
	u16 WIDTH;   //!! Max 8 for now! :-/
	u16 HEIGHT;
	u16 DESCENT; // Or baseline index from bottom (0-based)
	u16 ASCENT;

	// Code mapping...
	Index  GLYPH_COUNT;           // = sizeof(GLYPH_BITMAPS) / HEIGHT - 1;
	Index  FIRST_MAPPED_GLYPH_INDEX = 0;
	Index  LAST_MAPPED_GLYPH_INDEX;
	Index  INVALID_GLYPH_INDEX;   // I.e. index of glyph to use for unsupported codepoints.
	// Optional helpers used only in the default code mapping functions
	// that implement direct linear "bulk" mapping:
	Codepoint FIRST_GLYPH_CODEPOINT; // BOTH the lowest codepoint supported  AND the cp. of FIRST_GLYPH_INDEX!
	Codepoint LAST_GLYPH_CODEPOINT;  // BOTH the highest codepoint supported AND the cp. of LAST_GLYPH_INDEX!

	char SPACING_MODE = 'C';   // [C]ell-spacing (clipping!), [M]onospaced (overlaps!), [P]roportional
	                           // Note: M is suprior for visuals, but more costly than C.
	                           // Per-char custom spacing for P can only be auto-detected,
	                           // not manually specified in the current simplistic format!

	const void* GLYPH_BITMAPS; // Not Byte, so the font files can have whatever suits them.

	// Map codepoint to glyph index
	virtual Index glyph_index(Codepoint codepoint) const
	{	//!! This only makes sense for a contiguous set of code points
		//!! directly mapped to the set of glyphs.
		//!! Override for anything more complex!
		return codepoint >= FIRST_GLYPH_CODEPOINT
		    && codepoint <= LAST_GLYPH_CODEPOINT
			? FIRST_MAPPED_GLYPH_INDEX + Index(codepoint - FIRST_GLYPH_CODEPOINT)
			: INVALID_GLYPH_INDEX; // I.e. valid index of the invalid-codepoint glyph...
	}

	// Inverse mapping from glyph index to codepoint
	virtual Codepoint glyph_codepoint(Index index) const
	{	//!! This only makes sense for a contiguous set of code points
		//!! directly mapped to the set of glyphs.
		//!! Override for anything more complex!
		return index >= FIRST_MAPPED_GLYPH_INDEX
		    && index <= LAST_MAPPED_GLYPH_INDEX
			? FIRST_GLYPH_CODEPOINT + index - FIRST_MAPPED_GLYPH_INDEX
			: Invalid_Codepoint; //!! Should rather be an assert, in fact...
	}

}; // font_def

} // namespace font
} // namespace sz


#endif // FDSN789C4MH89UYU958EJ9UKGU0V57KBH9045
