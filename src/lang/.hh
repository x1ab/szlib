#ifndef _90RKLJHGT45M764V6B5NC2GHDNXK86Y35J_
#define _90RKLJHGT45M764V6B5NC2GHDNXK86Y35J_

// 0.0.4
/*****************************************************************************
  My personal C++ "dialect add-on" and simple extensions

  (Syntactic sugar, types, utilities etc. for ground-levelling across
  projects, platforms, even programming languages etc.)

*****************************************************************************/

#include "types.hh"
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


// The "perfect unused-ignorer" hack:
// f(unused1, unused2) { IGNORE unused1, unused2; }
#ifdef IGNORE
# error Oh no, someone beat us to it: IGNORE is already taken! ;)
#endif
#define IGNORE _sz_grumpy_ignorer_{},
        struct _sz_grumpy_ignorer_{ template<typename T> _sz_grumpy_ignorer_& operator,(const T&) { return *this; } };


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

#endif // _90RKLJHGT45M764V6B5NC2GHDNXK86Y35J_
