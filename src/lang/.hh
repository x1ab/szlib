/*****************************************************************************
  C++ "dialect add-ons" and other simple extensions
  0.0.5

  (Shims for ground-levelling across platforms/compilers, syntax-sugar types,
  utilities, etc.)
*****************************************************************************/

#ifndef LL90RKLJHGT45M764V6B5NC2GHDNXK86Y35J
#define LL90RKLJHGT45M764V6B5NC2GHDNXK86Y35J

#include "atom/int.hh"

//!!#include "a proper enum..."

//!! Preprocessor:
//!! Add preprocessor warnings/errors for MSVC legacy PP without __VA_OPT__ etc...
//!! Shim for missing #warning...
//!! ...


//
// I was sooooo hesitating to define these macros with lower-case names,
// and then just let the world burn... :) But..., well, I'm still kinda
// a human being (unfortunately); too much existing code probably uses
// variables etc. with these names. I'd torture the wrong people.
//


// The definition of `IGNORE`... (Had to make it separately reincludable
// for our trench warfare for it against winbase.h!... :-o :)
#include "IGNORE.hh"


#define AUTO_CONST constexpr static auto
//#define auto_const AUTO_CONST // #daredef


#define OUT   // ⍄
#define INOUT // ⍃⍄
//#define IN  // ⍃ <-- Whoever itches to explicitly mark an arg. as `in` should seek medical help.

//
// `fallthrough` (macroless version of the non-existant control-flow statement):
//
//static volatile int FALLTHROUGH [[maybe_unused]]; // `volatile` is redundant though
//#define fallthrough FALLTHROUGH // #daredef -- NOPE: There's [[fallthrough]] since c++17...

// Synonyms for true/false
AUTO_CONST  ON  = true;
AUTO_CONST  OFF = false;
// Legacy:
//auto_const On = ON, Off = OFF;
// #daredef
//auto_const on = ON, off = OFF;


//----------------------------------------------------------------------------
// CRUFT...

//!!??enum { UseDefault = -1 };

#endif // LL90RKLJHGT45M764V6B5NC2GHDNXK86Y35J
