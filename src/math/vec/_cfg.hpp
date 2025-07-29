#ifndef _DFO8ER7NYDH0M85FYNFPNIKYUHM8R067NI_
#define _DFO8ER7NYDH0M85FYNFPNIKYUHM8R067NI_

//============================================================================
// Build options -- keep them consistent across all linked translation units!
// Prefer setting them during the build process, and passing them e.g. via -D!
//============================================================================
#define VEC_NAMESPACE vec
#define VEC_DIRECT_COORD_SUBNAMESPACE dc
#define VEC_IMPLICIT_NUM_CONV //!! This being a conf. macro is a PITA, but yet another API split
                              //!! after splitting for VEC_NO_DIRECT_COORDS?!... :-/
//#define VEC_NO_IOS // Define this before including ios.hpp to disable >>, << etc.!
//#define VEC_NO_CHECK_DIV0 // E.g. in normalize()
//#define VEC_CPP_IMPORT_STD
//!!#define VEC_NO_DIRECT_COORDS -- no longer used; versions with .x, ... are in a sep. namespace!
//!!TBD: #define VEC_NO_NAMED_COORDS // Not even .x(), ... (not just .x, ... of DIRECT_COORDS)
//!!TBD: #define VEC_STRICT_NO_DIRECT_COORDS //!! Forgot what the hell I meant by this...
                                             //!! Pro'ly just an older version of VEC_NO_NAMED_COORDS.

#define _TODO_VEC_NO_OPEQ_FOREIGN
	//!! Re-evaluate & comment why was this needed: for the "only 1 user conv." rule? -> #20!
	//!! (The current tests, mycoGUI, and OON compile & run apparently fine without it,
	//!! at least with GCC! But I may just not have an actual use case yet...)

#endif // _DFO8ER7NYDH0M85FYNFPNIKYUHM8R067NI_
