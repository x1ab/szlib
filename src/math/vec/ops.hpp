//============================================================================
// Vector operations as free functions
//============================================================================

#ifndef _HCVN987STD80FT7NT7GMYIURTFTYGNJKLRV_
#define _HCVN987STD80FT7NT7GMYIURTFTYGNJKLRV_


#include "base.hpp"


namespace VEC_NAMESPACE {

// Dear Preprocessor, thanks for kinda saving the day from even more C++ horror!....
#define _VEC_STANDALONE_BIVECT_FN_(Name) \
	template <UniformVectorData V1, UniformVectorData V2> requires VectorData_SameDim<V1, V2> \
	constexpr auto Name (const V1& v1, const V2& v2)
// OMFG, this explicit bool version (instead of the auto above) is required for Clang to even compile the op==!
#define _VEC_STANDALONE_BIVECT_FN_BOOL_(Name) \
	template <UniformVectorData V1, UniformVectorData V2> requires VectorData_SameDim<V1, V2> \
	constexpr bool Name (const V1& v1, const V2& v2)
// Mixed vector vs. scalar op. helpers:
#define _VEC_STANDALONE_VECT_SCALAR_FN_(Name) template <UniformVectorData V, Scalar N> \
	constexpr auto Name (const V& v, N n)
#define _VEC_STANDALONE_SCALAR_VECT_FN_(Name) template <UniformVectorData V, Scalar N> \
	constexpr auto Name (N n, const V& v)


//!!!!! WARNING !!!!! WARNING !!!!! WARNING !!!!! WARNING !!!!! WARNING !!!!!
//!
//! When adding a new function, DON'T FORGET to also add a corresponding
//! `using VEC_NAMESPACE::...;` to the "import list" at the end of
//! _vector_core-dc.hpp, for ADL to work in dc:: too!
//!
//!! (Better yet: refactor the headers to not need a separate `using` list!...)
//!
//!!!!! WARNING !!!!! WARNING !!!!! WARNING !!!!! WARNING !!!!! WARNING !!!!!


_VEC_STANDALONE_BIVECT_FN_BOOL_(operator ==);

// Pairwise ops...
_VEC_STANDALONE_BIVECT_FN_(operator +);
_VEC_STANDALONE_BIVECT_FN_(operator -);
_VEC_STANDALONE_BIVECT_FN_(operator *);
_VEC_STANDALONE_BIVECT_FN_(operator /);

_VEC_STANDALONE_BIVECT_FN_(dot);

// Mixed vector vs. scalar ops....
_VEC_STANDALONE_VECT_SCALAR_FN_(operator +);
_VEC_STANDALONE_VECT_SCALAR_FN_(operator *);
// Also in reverse, for commutativity:
_VEC_STANDALONE_SCALAR_VECT_FN_(operator +);
_VEC_STANDALONE_SCALAR_VECT_FN_(operator *);
// These are not commutative:
_VEC_STANDALONE_VECT_SCALAR_FN_(operator -);
_VEC_STANDALONE_VECT_SCALAR_FN_(operator /);


} // namespace VEC_NAMESPACE


//============================================================================
// IMPL...
//============================================================================
#include "_ops.inl"


#undef _VEC_STANDALONE_SCALAR_VECT_FN_
#undef _VEC_STANDALONE_VECT_SCALAR_FN_
#undef _VEC_STANDALONE_BIVECT_FN_BOOL_
#undef _VEC_STANDALONE_BIVECT_FN_

#endif // _HCVN987STD80FT7NT7GMYIURTFTYGNJKLRV_
