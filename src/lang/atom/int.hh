// v0.2.2

#ifndef FY575H3E87BMH6C6RV7UUH9BH85MTFGJBBGY
#define FY575H3E87BMH6C6RV7UUH9BH85MTFGJBBGY


namespace sz::atom::unint {

#if defined(__GNUC__) || defined(__clang__)

	using u8  = __UINT8_TYPE__;
	using u16 = __UINT16_TYPE__;
	using u32 = __UINT32_TYPE__;
	using u64 = __UINT64_TYPE__;

	using i8  = __INT8_TYPE__;
	using i16 = __INT16_TYPE__;
	using i32 = __INT32_TYPE__;
	using i64 = __INT64_TYPE__;

#elif defined(_MSC_VER)

	using u8  = unsigned __int8;
	using u16 = unsigned __int16;
	using u32 = unsigned __int32;
	using u64 = unsigned __int64;

	using i8  = __int8;
	using i16 = __int16;
	using i32 = __int32;
	using i64 = __int64;
#else
	// Best-effort fallback for other compilers (assuming common 32/64-bit arch.):
	using u8  = unsigned char;
	using u16 = unsigned short;
	using u32 = unsigned int;
	using u64 = unsigned long long;

	using i8  = signed char;
	using i16 = short;
	using i32 = int;
	using i64 = long long;

	// Don't be ridiculous... ;)
	//static_assert(sizeof(u8)  == 1, "Couldn't define u8/i8 as 1 byte!");
	//static_assert(u8(-1) == 0xff, "char is > 8 bits on your platform?"); // At least 8 is guaranteed by the language, IIRC.
	static_assert(sizeof(u16) == 2, "Couldn't define u16/i16 as 2 bytes!");
	static_assert(sizeof(u32) == 4, "Couldn't define u32/i32 as 4 bytes!");
	static_assert(sizeof(u64) == 8, "Couldn't define u64/i64 as 8 bytes!");

#endif

	using usize = decltype(sizeof 0);            // size_t
	using isize = decltype((char*)0 - (char*)0); // ptrdiff_t

} // namespace sz::atom::unint


#endif // FY575H3E87BMH6C6RV7UUH9BH85MTFGJBBGY
