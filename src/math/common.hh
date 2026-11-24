#ifndef ZMSZXCV204924T9BNUJM0X9CKVLNMJTIUC45
#define ZMSZXCV204924T9BNUJM0X9CKVLNMJTIUC45


#include "_builtins.hh"
#include "sign.hh"
#include "minmax.hh" // This could bite if someone included Windows.h in
                     // the TU without NOMINMAX, and uses math::min/max...

//!!#include "clamp.hh" //!! Not yet; still in O2N (Szim/Math/clamp.hpp)!


namespace sz::math {


// Unsigned-safe |a-b| (unlike just abs(a-b)!)
template <typename T> T absdiff(T a, T b) { return a > b ? a - b : b - a; }


} // namespace sz::math

#endif // ZMSZXCV204924T9BNUJM0X9CKVLNMJTIUC45
