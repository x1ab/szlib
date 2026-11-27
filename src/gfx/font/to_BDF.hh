// 0.4.0

#ifndef FCM478EHTW78O4HY3C479HY6RM9U4E59T6N
#define FCM478EHTW78O4HY3C479HY6RM9U4E59T6N

#include "font_def.hh"
#include <string>

namespace sz {
namespace font {

// NOTE: constexpr vars have internal linkage, and constexpr functions are `inline`, so ODR should be fine.

//----------------------------------------------------------------------------
std::string to_BDF(const font_def& def);

} // namespace font
} // namespace sz


#endif // FCM478EHTW78O4HY3C479HY6RM9U4E59T6N
