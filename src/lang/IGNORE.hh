// The "perfect unused-ignorer" hack:
// Usage: f(A unused1, B unused2) { IGNORE unused1, unused2; }
// Sexy points over other solutions:
// - No need to mess with the function's signature
// - No parentheses (they're just annoying to type)
// - No ridiculous [[maybe_unused]] for every var
// - No relying on multi-GB IDEs to do the above for ya
// Collisions? Be more careful whom you befriend next time...
// This header can be re-#included freely to reapply the macro,
// should a villain try to re-#define IGNORE away from you.

#ifdef IGNORE
// We're kinda losing against winbase.h here...
# ifndef _MSC_VER
#  warning Oh no, someone beat us to it: IGNORE is already taken! ;) But never fear, let's just #undef it like a boss...
# else
#  define _sz__STR_2(x) #x
#  define _sz__STR(x) _sz__STR_2(x)
#  pragma message( "Warning: Oh no, someone beat us to it: IGNORE is already taken! ;) But never fear, let's just #undef it like a boss..." \
	" at " __FILE__ ":" _sz__STR(__LINE__)  )
#  undef _sz__STR
#  undef _sz__STR_2
# endif // _MSC_VER
#
# undef IGNORE
#
#endif // IGNORE

#define IGNORE  _sz__object_sinkhole_{},
#ifndef         _sz__object_sinkhole__defined_ // Only guard this bit, not the whole header!
#define         _sz__object_sinkhole__defined_
  struct        _sz__object_sinkhole_{ template<typename T> _sz__object_sinkhole_& operator,(const T&) { return *this; } };
#endif
