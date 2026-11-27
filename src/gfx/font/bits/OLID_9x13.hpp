//! Before including an sz-bitfont font source, include the type header first:
//! #include "sz/gfx/font/bitfont.hh"

#ifndef OLID9X13F92Y24789ERYM94H8C8VMOUJH39X58CRVJ
#define OLID9X13F92Y24789ERYM94H8C8VMOUJH39X58CRVJ

namespace SAL::gfx::font {
namespace OLID_9x13 {

//----------------------------------------------------------------------------
// Bitmap
using BitLine = sz::font::font_def::u16;
extern const BitLine glyph_bitmaps[];

//----------------------------------------------------------------------------
// Metadata
struct Data : sz::font::font_def
{
	// Not customized yet...
};

extern const Data data;

} // namespace OLID_9x13
} // namespace SAL::gfx::font


#endif // OLID9X13F92Y24789ERYM94H8C8VMOUJH39X58CRVJ
