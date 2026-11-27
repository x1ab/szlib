Basic backendless graphics/geometry toolkit for a survival kit

IMPORT ONE, DO NOT NIH!... E.g.:

HAGL — https://github.com/tuupola/hagl
	Lovely, simple, put_pixel/bitmap output

Olive.c — https://github.com/tsoding/olive.c
	C, framebuffer, stb-style minimalist

ezdib — https://github.com/wheresjames/ezdib
	C, buffer or callback, stdlib
	> A small, self-contained C library for creating and drawing into BMP images.
	> It is designed to be easy to drop into any project — embedded, desktop, or
	> anywhere in between — with zero mandatory external dependencies.

TSR — https://github.com/mstanimirovic/tsr
	C, framebuffer, not even uses the stdlib!
	> A minimal, zero-dependency 2D software renderer written in C99 for freestanding environments.
	> No standard library required. Single-header. Just a framebuffer and a compiler.

canvas_ity — https://github.com/a-e-k/canvas_ity
	Astonishing HTML canvas impl. TTF decoder! Andrew Kensler! (C++ stdlib)
	-> https://fabiensanglard.net/postcard_pathtracer/index.html

-----------------------------------------------------
1. HAGL — Hardware Agnostic Graphics Library

Architecturally the closest match to noGFX so far.

Its README explicitly states that a backend need only implement pixel plotting. All higher-level primitives can work through that single function. More efficient backend operations are optional.
- C, no dynamic allocation.
- Lines, rectangles, circles, ellipses, polygons, etc.
- Bitmap fonts, including three bundled fonts.
- Clipping, blitting, scaling.
- No mandatory graphics backend.
It does, however, have a display abstraction, color types and backend callbacks. So it's somewhat more opinionated than our deliberately impoverished interface.


2. Olive.c — Tsoding

Possibly the best match in spirit.

A dependency-free, stb-style, single-file C graphics library. It draws into caller-provided memory and doesn't concern itself with displaying the result.

The author even explicitly advocates copying the source directly into your project rather than introducing elaborate build integration.

The main architectural difference is that it assumes a particular raster representation instead of delegating pixel plotting to the client. Also, the author explicitly warns that the API is unstable.

I checked Olive.c's actual source, too:
- 961 lines, including the font data.
- MIT license.
- Canvas representation: pixel pointer, width, height, stride. Exactly our earlier RasterView idea.
- Rectangles, circles, ellipses, lines, triangles, sprites, even textured triangles.
- An embedded 6×6 bitmap font, but amusingly, the uppercase glyphs are still unimplemented! 
  raw.githubusercontent.com+1
3. ezdib — Easy DIB Drawing Library
An interesting, obscure little C library.
- Just one .c and one .h.
- BSD-style license.
- Two embedded bitmap fonts.
- Basic shapes, flood filling, scaling.
- Optional BMP reading and writing.
- Caller-provided framebuffer or pixel callback without a framebuffer.
- Optional integer-only trigonometry using CORDIC.
More image-oriented than geometry-oriented, but remarkably close to our original requirements.


4. TSR — Tiny Software Renderer

Even the C standard library is unnecessary.
- Single-header C99.
- Integer arithmetic only.
- Embedded 8×8 printable ASCII font.
- Basic drawing primitives.
- Caller-provided framebuffer with stride.
- Compile-time selection of RGBA8888 or RGB565.
Very modest indeed, although apparently an embryonic project with only three commits and no explicit license visible.


5. canvas_ity

For completeness, something considerably more sophisticated, yet surprisingly compact:
- Single-header C++03, ISC license.
- Approximately 2,300 effective lines of code.
- Antialiasing, Bézier curves, gradients, transformations, compositing.
- Built-in TrueType parsing.
- No dependencies beyond the standard library.
- Reported 100% line coverage by its test suite.
Essentially a standalone software implementation of HTML5 Canvas. Probably excessive for diagnostics, but quite an impressive engineering specimen.
