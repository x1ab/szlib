#ifndef _COV9487NYG0879ER6Y07798ETU98TJNH6R8970NHJ9F_
#define _COV9487NYG0879ER6Y07798ETU98TJNH6R8970NHJ9F_


#include "../base.hpp"


namespace VEC_NAMESPACE::VEC_DIRECT_COORD_SUBNAMESPACE {

// "Live" vector (class)
//----------------------------------------------------------------------------
//!!
//!! Split this into a generic part shared by both the "app vector" and the "vector adapter",
//!! and a specific part for app-vectors only!
//!!
template <class V>
concept VectorClass = VEC_NAMESPACE::VectorClass<V>
	&& UniformVectorData<V> //!! NOT anything stricter here though, like InternalVectorData<V>!
	&& std::copyable<V> // Includes copy_constructible!
	&& requires {
		typename V::number_type;
		{ Vector_traits<V>::dim };// -> std::convertible_to<unsigned>;
	}
	&& HasNArgCtor<V, V::dim, typename V::number_type>
	&& requires(typename V::number_type n) {
		requires Vector_traits<V>::dim >= 2;

		// Named coord. access at low dimensions
		requires (Vector_traits<V>::dim == 2 ? requires(V v) {
			{ v.x };// -> std::same_as<typename V::number_type&>;
			{ v.y };// -> std::same_as<typename V::number_type&>;
		} : true);
		requires (Vector_traits<V>::dim == 3 ? requires(V v) {
			{ v.x };// -> std::same_as<typename V::number_type&>;
			{ v.y };// -> std::same_as<typename V::number_type&>;
			{ v.z };// -> std::same_as<typename V::number_type&>;
		} : true);
		requires (Vector_traits<V>::dim == 4 ? requires(V v) {
			{ v.x };// -> std::same_as<typename V::number_type&>;
			{ v.y };// -> std::same_as<typename V::number_type&>;
			{ v.z };// -> std::same_as<typename V::number_type&>;
			{ v.w };// -> std::same_as<typename V::number_type&>;
		} : true);
	};


} // namespace VEC_NAMESPACE::VEC_DIRECT_COORD_SUBNAMESPACE


#endif // _COV9487NYG0879ER6Y07798ETU98TJNH6R8970NHJ9F_
