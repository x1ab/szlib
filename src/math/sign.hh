// v0.2.0
#ifndef SZMSGNDFCSMD8F702N45T24875GYCHV
#define SZMSGNDFCSMD8F702N45T24875GYCHV

namespace sz::math {

template <typename T> constexpr
T sign(T x)
// Note: sign(NaN) == 0.
{
	if (x > T(0)) return T(1);
	if (x < T(0)) return T(T(0) - T(1)); // Not -T(1): avoids MSVC C4146 for unsigned & int-promotion for small types
	return T(0);
}

}; // namespace sz::math

#endif // SZMSGNDFCSMD8F702N45T24875GYCHV
