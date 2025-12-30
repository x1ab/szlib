// The (not) perfect unused-warning suppressor hack...
// Usage: f(A unused1, B unused2) { IGNORE unused1, unused2; }
// Sexy points over other solutions:
// - No parentheses (they're so annoying to type)
// - No need to mess with the function signature
// - No ridiculous [[maybe_unused]] for every var
// - No relying on a GB-sized IDE to do all these
// Macro collisions? Well, be more careful whom you befriend next time!... ;)
// But should a villain #define IGNORE away from you, this header can just
// be #included again freely to override, stealing the IGNORE macro back.
// (Windows.h is the archetypal enemy, and it almost never really needs
// that macro anyway.)

#ifdef IGNORE
// We're kinda losing against winbase.h here...
# ifndef _MSC_VER
#  warning                  "Oh no, someone beat us to it: IGNORE is already taken! :-/ But never fear, let's just #undef it and win the boss fight..."
# else                      // Without the quotes, GCC was freaking out about the apostrophe here..........^
#  define _sz__STR_2(x) #x
#  define _sz__STR(x) _sz__STR_2(x)
#  pragma message( "Warning: Oh no, someone beat us to it: IGNORE is already taken! :-/ But never fear, let's just #undef it and win the boss fight..." \
	" at " __FILE__ ":" _sz__STR(__LINE__)  )
#  undef _sz__STR
#  undef _sz__STR_2
# endif // _MSC_VER
#
# undef IGNORE
#
#endif // IGNORE

#define IGNORE  _sz__object_sinkhole_{}, // You'd think "hmm, tack a [[maybe_unused]] in front, to avoid the irony..."
                                         // — but no: C++ is not your friend. It's your enemy. It wouldn't compile. :-/
#ifndef         _sz__object_sinkhole__defined_ // Only guard this bit, not the whole header!
#define         _sz__object_sinkhole__defined_
  struct _sz__object_sinkhole_{ template<typename T> constexpr _sz__object_sinkhole_& operator,(const T&) { return *this; } };
#endif
