// 0.6.1
//#define DEBUG
/*
Stable-index, stable-pointer, O(1) add/delete, "free-list-like" container of POD-like items

	Clients can obtain opaque handles (keys) of "locker slots" (cells)
	from a managed pool of a fixed number of contiguous cells (as long
	as there are free ones left), and store a value of type T there.

	T must be `trivially destructible` (POD-like): no actual destructor
	call is made on remove()! (-> #25)
	!! BUT: The byte-poisoning in DEBUG mode also implicitly imposes
	!! `trivially_constructible`, or in fact even more restrictions! :-o

	Each cell represents a resource, and the key of the cell is
	meant to be used as a handle to that resource. Each key is unique
	in the context of the container (among currently stored items;
	extending uniqueness beyond the current container state is also
	planned -> #25, #27).

	In practice, often proxy resources (OS handles, (smart) pointers,
	(weak) references, indexes, etc. would be stored in the cells.

	The intended use case is to model resource allocation (i.e. "locked"
	exclusive use — not in terms of concurrency, but ownership).
	Once a key has been obtained, another key to the same slot can't
	be obtained.

	Later, when done using the slot, its key should be "returned" to
	the container, so the cell can be freed for reuse.
	(Subsequent allocation of the same cell is not guaranteed to assign
	the same key to it as before; actually, the notion of "same cell"
	isn't even defined: it's always just one isolated cell that can be
	accessed via a key, and there is no defined relation between cells
	or keys.)

	Note: upon returning the key, the content of the cell will be
	implicitly discarded: "deemed invalid", but not ~destructed.
	This means that actually destroying (destructing, deallocating)
	a tracked (stored) resource is completely out of scope for this
	container.

	Accessing a cell's content (i.e. to store or retrieve a T value)
	is done via the usual key-value map semantics. There is no ordering
	defined among keys or elements. There isn't even *direct* iterator
	support either on the `lockers` class itself (so, in the STL sense,
	this isn't even a container), but see `lockers::each` later!

	Copying the container (considered to be a resource manager) is
	disabled, too.
	!! BUT: the "POD-like T" restriction goes directly against the "resource manager" assumption!...

	Otherwise, the usual STL-like semantics applies: default-constructed
	(but otherwise uninitialized!) empty slots with garbage in them,
	requiring add() or set() to initialize them (by-value, via op=),
	with the usual STL-like queries (empty, size, capacity etc.), no
	checks by default (making it the user's responsibility to keep
	things valid).

	No exceptions are thrown by the container itself (but T's ctor or
	op= may do so — discouraged; it might lead to inconsistent state;
	also trivially_constructible and/or trivially_copyable may get added
	to the constraints on T). Instead, the enum value ERROR is returned,
	when trying to obtain a slot while none is available.

NOTES:

 -  Iteration:

    - Snapshots:

	In the target use case keys are opaque IDs to individual, unrelated
	elements that don't form an inherently meaningful set that would
	naturally be manipulated as a coherent whole. Instead, views,
	snapshots, samples, etc. would be collected from the original set,
	as semantically meaningful collections for further processing.

	The `lockers::each` class implements those snapshots.

	In the target use case deletes are also expected to be rare, allowing
	these snapshots to be fairly stable (not rebuilt often).

	Also, there's configurable rudimentary checking (via asserts) against
	the upstream container getting out of sync by potentially having
	collected items deleted (or new relevant ones added). See the `each`
	class (and its ctor) for details! (Better support, including per-cell
	delete tracking, is planned after 1.0. -> #27, #25)

    - Iterators:

	Without extra bookkeeping, iterating over allocated slots could be
	expensive, needing O(n) linear search *at every ++/-- operation* to
	find out if the next/prev. cell is in use, *and* also repeating that
	to ignore empty slots — in the worst case also ~n times, so we'd be
	near O(n*n) territory... This prevented adding iterators to the API
	in the past.

	To avoid the extra per-slot linear searches, an occupancy bitmask
	has been added in 0.6.0; that keeps cell state lookup O(1).
	This made snapshot creation significantly faster, and cleared the
	way for adding iteration support to the locker API directly (after
	1.0; see #25).

	(Keeping track of only the *used* slots instead of only the *free* ones
	could've made iteration const-time per step, but at the cost of degrading
	the performance of claim() and remove() (to linear time), or an extra
	back-ref (key) per cell (+ swap-with-last) to avoid that. The trade-off
	of tracking *both*, but now only as 1 bit per cell, buys back const.
	state-check perf. at the cost of <~3% extra bookkeeping storage.)

    -	Sloppy client-side ownership management may lead to classic dangling-
	pointer mistakes (storing cell references (keys), while other code paths
	might accidentally call remove() for the same resource).
	By using handles, instead of direct T element references (pointers,
	STL-like iterators), the explicit indirection via keys (rather than
	indexes):

	  a) triggers conscious effort ("thinking") when accessing elements,

	  b) offers a checkpoint in the container to catch invalid references
	     (only via asserts (without NDEBUG), and only until a slot gets reused).
*/

#ifndef LKR3986GH78F786D102D75426748GB
#define LKR3986GH78F786D102D75426748GB

#include <cstddef> // size_t
#include <cassert>
#ifdef _MSC_VER
#  include <intrin.h> // _BitScanForward[64]
#endif

namespace sz {

template <typename T, std::size_t MAX, typename KeyT = unsigned>
class lockers
{
public:
	using key_t = KeyT;
	static_assert(key_t(-1) > 0, "KeyT must be unsigned");
	static_assert(MAX <= key_t(-1), "MAX doesn't fit KeyT (its top value is reserved for ERROR)");

	enum : key_t { ERROR = key_t(-1) };

	//--------------------------------------------------------------------
	// Structural changes...

	// Reserve a slot, without initializing it
	// (Can't call it `reserve`, as STL "owns" that for a diff. concept! :-/ )
	[[nodiscard]] key_t claim() { // Or return ERROR
		if (full()) return ERROR; // Not assert, unlike for double free!
		++gen_;
		return _pop_free();
	}
	// Claim + set
	key_t add(const T& cargo) { // Or return ERROR
		key_t slot = claim();
		if (slot == ERROR) return ERROR;
		set(slot, cargo);
		return slot;
	}

	// "Unclaim" (discard, free, ...)
	void remove(key_t slot) {
		assert(!empty());
		assert(slot != ERROR);
		assert(slot < capacity_);
		assert(!_is_free(slot)); // Double free?
		_push_free(slot);
		++gen_;
		++del_gen_;
	}

	//--------------------------------------------------------------------
	// Element access...

	// Set
	T& set(key_t slot, const T& cargo) {
		assert(!empty());
		assert(slot != ERROR);
		assert(slot < capacity_);
		assert(!_is_free(slot));
		return slots_[slot] = cargo; // Support method-chaining on T&
	}

	// Get (various flavors)
	T& get(key_t slot) {
		assert(!empty());
		assert(slot != ERROR);
		assert(slot < capacity_);
		assert(!_is_free(slot));
		return slots_[slot];
	}
	const T& get(key_t slot) const {
		assert(!empty());
		assert(slot != ERROR);
		assert(slot < capacity_);
		assert(!_is_free(slot));
		return slots_[slot];
	}
	const T& cget(key_t slot) const {
		assert(!empty());
		assert(slot != ERROR);
		assert(slot < capacity_);
		assert(!_is_free(slot));
		return slots_[slot];
	}
	const T& operator [](key_t key) const {
		assert(!empty());
		assert(key != ERROR);
		assert(key < capacity_);
		assert(!_is_free(key));
		return get(key);
	}
	T& operator [](key_t key) {
		assert(!empty());
		assert(key != ERROR);
		assert(key < capacity_);
		assert(!_is_free(key));
		return get(key);
	}

	// Old names (to be removed):
	[[deprecated("use claim()")]]  [[nodiscard]] key_t alloc()   { return claim(); }
	[[deprecated("use add()")]]    key_t store(const T& cargo)   { return add(cargo); }
	[[deprecated("use set()")]]    T& store(key_t slot, const T& cargo) { return set(slot, cargo); }
	[[deprecated("use remove()")]] void  free(key_t slot)        { remove(slot); }
	[[deprecated("use remove()")]] void  release(key_t slot)     { remove(slot); }

	//--------------------------------------------------------------------
	// Misc. queries...

	bool empty()    const { assert(end_free_i_ <= capacity_); return end_free_i_ == capacity_; }
	bool full()     const { return end_free_i_ == 0; }
	auto size()     const { return capacity_ - end_free_i_; }
	auto capacity() const { assert(capacity_ == MAX); //!! a) for now..., b) Unbelievable: fails with MSVC /DDEBUG! :-o
	                        return capacity_; }

	bool is_free(key_t slot) const {
//std::cerr << "- checking key at is_free: " << slot << endl;
		assert(slot != ERROR);
		// "Always true" warning (with GCC), if key_t is unsigned... which it is:
		//assert(slot >= 0);
		assert(slot < capacity_);
		//assert(!empty()); // Well, every key is free if there's nothing allocated... :)
		return _is_free(slot); }

	//--------------------------------------------------------------------
	// Setup & C++ boilerplate
	lockers()
	{
		// We're working backwards from a "fake" full state:
		assert(full()); // "full" here just means we don't have the free items pool (stack) init'd yet.

		// Put all the keys we manage into the free-stack, rendering the container state "empty".
		// Pushed in reverse, so claim() hands out 0, 1, 2... (keeps elements low, for each() & locality):
		for (std::size_t i = capacity_; !empty(); )
			_push_free(_index_to_key(--i));

		assert(empty());
		assert(size() == 0);
		assert(end_free_i_ == capacity_);

#ifdef DEBUG
		for (std::size_t i = 0; i < capacity_; ++i) {
			assert(free_stack_[i] == capacity_ - 1 - i);
		}
		// Note: this'll undo _push_free's '~' poisoning:
		for (std::size_t i = 0; i < capacity_; ++i) {
			*reinterpret_cast<char*>(&slots_[i]) = '_';
		}
#endif		
	}

	// No copy:
	lockers(const lockers&) = delete;
	lockers& operator=(const lockers&) = delete;


	//--------------------------------------------------------------------
	// Iterable snapshot copy of the currently allocated keys
	//

		// Change tracking policy: which changes of the source container
		// invalidate the snapshot? (asserted on access)
		//!! This would be shared across multiple snapshot classes; see #27!
		enum Invalidate {
			Never,         // Trust the app
			On_Any_Change, // Claim/add/remove (default)
			On_Remove      // Only bail on new removes (newly added items just won't be in the snapshot)
		};

	//
	// To support static construction (and trivial copyability), the key list
	// is not dynamically allocated to size() size, but rather a fixed-size
	// array of MAX keys! (Which means much of the array size will be likely
	// unused, and copying would also be wasteful/expensive with huge cell
	// counts (i.e. capacity, i.e. MAX).)
	//
	// Usage: for (auto& x : sz::lockers<...>::each(l)) ...
	//        Or use snapshot.key(i) to get the slot handles instead.
	//        Also, for iterating over *keys* instead of T refs, see .keys() below!
	struct each
	{
		each(lockers& src, Invalidate mode = On_Any_Change)
			: src_(&src), mode_(mode), gen_(src.gen_), del_gen_(src.del_gen_)
		{
			assert(src_);
			// Skip empty words wholesale; walk the set bits of the rest:
			for (std::size_t w = 0; size_ < src_->size() && w < sizeof src_->occ_ / sizeof src_->occ_[0]; ++w)
				for (auto bits = src_->occ_[w]; bits; bits &= bits - 1) //! Drop lowest set bit -> Wegner's popcount (also: bits & -bits...)
					keys_[size_++] = _index_to_key(w * OCC_BITS + _ctz(bits));
			assert(size_ == src_->size());
		}

		std::size_t size() const { return size_; }

		key_t key(std::size_t i) const {
			assert(i < size_);
		#ifndef NDEBUG
			_assert_sync();
		#endif
			return keys_[i];
		}
		T& operator [](std::size_t i)  { assert(i < size_); return (*src_)[key(i)]; }

		struct iterator {
			each* s; std::size_t i;
			T& operator *() const { return (*s)[i]; }
			iterator& operator ++() { ++i; return *this; }
			bool operator !=(const iterator& other) const { assert(s == other.s); return i != other.i; }
		};
		iterator begin() { return {this, 0}; }
		iterator end()   { return {this, size_}; }

		// Support for iterating keys (not stored values directly, via T&)
		// Usage: locker::each snapshot(pool);
		//        for (auto& value : snapshot) process(value);       // T&
		//        for (auto key : snapshot.keys()) process_key(key); // key_t		
		struct key_range {
			const each* owner_;
			struct iterator {
				const each* owner_;
				std::size_t i;
					key_t operator*() const { return owner_->key(i); }
				iterator& operator++() { ++i; return *this; }
				bool operator!=(const iterator& rhs) const {
					assert(owner_ == rhs.owner_);
					return i != rhs.i;
				}
			};
			iterator begin() const { return {owner_, 0}; }
			iterator end()   const { return {owner_, owner_->size()}; }
		};

		key_range keys() const { return {this}; }

	protected:
		void _assert_sync() const { //!! May graduate to actual runtime checks later! -> #27
			if (mode_ == Invalidate::On_Any_Change) assert(gen_     == src_->gen_);     // && "lockers::each(Invalidate::On_Any_Change): used after claim/add/remove!");
			if (mode_ == Invalidate::On_Remove)     assert(del_gen_ == src_->del_gen_); // && "lockers::each(Invalidate::On_Remove): used after remove!");
		}

		lockers*    src_;
		Invalidate  mode_;
		std::size_t gen_;
		std::size_t del_gen_;
		key_t       keys_[MAX];
		std::size_t size_ = 0;
	};


protected:
	//--------------------------------------------------------------------
	// Internals - Data...

	std::size_t  capacity_ = MAX; //!! A DYNAMIC-GROWTH OPTION NOT YET IMPL.!
	                              //!! FTR, that would break the "stable addresses" trait though.
	T            slots_[MAX];
	key_t        free_stack_[MAX];
	std::size_t  end_free_i_ = 0; // Not key_t, as this is an internal iterator/index of keys! (Enjoy the "illiteration"... ;) )
	// Occupancy bitmap (1 = allocated), kept in sync with free_stack_:
	using occ_word_t = std::size_t;
	static constexpr auto OCC_BITS = sizeof(occ_word_t) * 8; // CHAR_BIT would need <climits>...
	occ_word_t   occ_[(MAX + OCC_BITS - 1) / OCC_BITS] = {};
	// Container-level generation counters to track structural changes:
	std::size_t  gen_ = 0;        // add/remove, to support iterator/snapshot validity checking
	std::size_t  del_gen_ = 0;    // only remove, for more lenient checking (-> Invalidate::On_Remove)
	//!!?? Later perhaps another one for realloc, to cover the "stable addresses possibly lost" case...

	//--------------------------------------------------------------------
	// Internals - Utilities...

	void _push_free(key_t k) {
		assert(!empty());
		free_stack_[end_free_i_++] = k;
		_occ_clear(k);
#ifdef DEBUG
		// Poison/mark the freed item as "discarded".
		// (Gambling on T's op= not being confused by the junk there.)
		*reinterpret_cast<char*>(&slots_[k]) = '~'; // All I can offer instead of a real ~T() for now... ;)
#endif
	}
#ifndef DEBUG
	key_t _pop_free() {
		//!!assert(!full()); //!! Already returns ERROR at run-time! Check for that at the call sites instead!
		if (full()) return ERROR;
		auto slot = free_stack_[--end_free_i_];
		_occ_set(slot);
		return slot;
	}
#else
	key_t _pop_free() { if (full()) return ERROR;
		auto slot = free_stack_[--end_free_i_];
		free_stack_[end_free_i_] = (key_t)ERROR; // Mark the unused index slot
		_occ_set(slot);
		return slot;
	}
#endif
	bool _is_free(key_t slot) const {
		bool result = !(occ_[slot / OCC_BITS] >> (slot % OCC_BITS) & 1);
#ifdef DEBUG
		assert(result == _is_free_slow(slot)); // Cross-check vs. the free-stack (O(n)!)
#endif
		return result;
	}
#ifdef DEBUG
	bool _is_free_slow(key_t slot) const {
		for (std::size_t free_i = 0; free_i < end_free_i_; ++free_i) //! Not free_i < size()!...
			if (slot == free_stack_[free_i]) return true;
		return false;
	}
#endif
	void _occ_set(key_t slot)   { occ_[slot / OCC_BITS] |=  (occ_word_t(1) << (slot % OCC_BITS)); }
	void _occ_clear(key_t slot) { occ_[slot / OCC_BITS] &= ~(occ_word_t(1) << (slot % OCC_BITS)); }

	// Slot index -> key (opaque handle); currently the same value
	//!! Needs to be public, or friended, for the "external test member" dump_state:
	public: //!! C++ bullshit: still no way to qualify individual members... :-/
	static key_t       _index_to_key(std::size_t ndx) { assert(ndx < MAX); return key_t(ndx); }
//!!	static std::size_t _key_to_index(key_t slot)      { ... -> #27 }
	protected:

	// Index of the lowest set bit (bits must be != 0)
	static unsigned _ctz(occ_word_t bits) {
		assert(bits);
#if defined(_MSC_VER)
		unsigned long i;
#   if defined(_WIN64)
		_BitScanForward64(&i, bits);
#   else
		_BitScanForward(&i, bits);
#   endif
		return (unsigned)i;
#else
		return (unsigned)__builtin_ctzll(bits);
#endif
	}

}; // class lockers
} // namespace sz


//----------------------------------------------------------------------------
#ifdef UNIT_TEST // g++ -x c++ lockers.hh -DUNIT_TEST -DDEBUG -Wall -Wextra -pedantic
#include <iostream>
#include <chrono>
using namespace std;
using namespace sz;

// Not a locker:: member to avoid the <iostream> dep.!
auto dump_state = [](const auto& lockers) {
	for (std::size_t slot = 0; slot < lockers.capacity(); ++slot) {
		auto key = lockers._index_to_key(slot);
#ifndef DEBUG
		cout << (lockers.is_free(key) ? '_' : (char)(lockers[key] > 31 ? lockers[key] & 127 : '?'));
#else		
		// The DEBUG extensions have already marked/poisoned the free items,
		// but... [] access is still disallowed for freed items, so can't
		//cout << *reinterpret_cast<const char*>(&lockers[slot]);
		// ...so, just do the same as in non-DEBUG, for now:
		cout << (lockers.is_free(key) ? '_' : (char)(lockers[key] > 31 ? lockers[key] & 127 : '?'));
#endif
	}
	cout << " - occupied: " << lockers.size() << endl;
};
//!! #27: No sane conversion between key_t and size_t yet!
// Rough timing of the main ops on a big locker
void perf_test()
{
#if defined(DEBUG) && !defined(NDEBUG) //! The DEBUG _is_free cross-check is O(n) per access in asserts!
	constexpr std::size_t N = 10'000;
	cout << "\n*** WARNING: Cont. size reduced to "<< N <<" in DEBUG with (the slow) asserts enabled!\n";
#else
	constexpr std::size_t N = 1'000'000;
#endif
	using BigL = lockers<int, N>;
	using Clock = std::chrono::steady_clock;
	auto ms = [](Clock::time_point a, Clock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); };

	cout << "\nTiming (N = " << N << "):\n";
	auto t0 = Clock::now();
	auto& c = *new BigL;
	auto t1 = Clock::now();
	for (std::size_t i = 0; i < N; ++i) c.add(int(i));
	auto t2 = Clock::now();
	for (BigL::key_t k = 0; k < N; k += 2) c.remove(k);
	auto t3 = Clock::now();
	auto& e = *new BigL::each(c);
	auto t4 = Clock::now();
	long long sum = 0; for (auto& x : e) sum += x;
	auto t5 = Clock::now();
	long long sum2 = 0; for (std::size_t j = 0; j < e.size(); ++j) sum2 += c[e.key(j)];
	auto t6 = Clock::now();

	assert(c.size() == N / 2 && e.size() == N / 2);
	assert(sum == sum2 && sum == (long long)(N / 2) * (N / 2)); // Sum of the odd numbers below N (= (N/2)^2, for even N)

	cout << "  ctor:                 " << ms(t0, t1) << " ms\n"
	     << "  add    x N:           " << ms(t1, t2) << " ms\n"
	     << "  remove x N/2:         " << ms(t2, t3) << " ms\n"
	     << "  each build (N/2):     " << ms(t3, t4) << " ms\n"
	     << "  each walk:            " << ms(t4, t5) << " ms\n"
	     << "  op[]   x N/2:         " << ms(t5, t6) << " ms\n"
	     << "  TOTAL:                " << ms(t0, t6) << " ms\n"
	     << "  (checksums: " << sum << ", " << sum2 << ")\n"; // Also keeps the optimizer from dropping the loops
	delete &e;
	delete &c;
}

int main()
{
	using L = lockers<char, 4>;
	using K = L::key_t;

	L slots;
	{	L empty;
		const L::each s(empty);
		std::size_t n = 0;
		for (auto k : s.keys()) { (void)k; ++n; }
		assert(n == 0);
	}
	L::key_t a = K(-1), b = K(-2), c = K(-3), d = K(-4), err = K(-666); //! K(...) for signed->unsigned warnings

//	slots.is_free('?'); // Not a key; should fail!

	assert(slots.empty());
	dump_state(slots);

	a = slots.claim();  assert(a != L::ERROR); // claim only, no init!
	b = slots.add('B'); assert(b != L::ERROR);
	c = slots.claim();  assert(c != L::ERROR); // claim only, no init!
	d = slots.claim();  assert(d != L::ERROR); // claim only, no init!

	assert(slots.full());
	err = slots.claim();  assert(err == L::ERROR); // Now fail!
	err = slots.add('X'); assert(err == L::ERROR); // Ditto

	cout << slots.set(d, 'd') << '\n';
	cout << (slots[a] = 'a') << '\n';
	cout << "capacity: " << slots.capacity() << '\n';
	#ifndef DEBUG // Note: [c] has not yet been initialized here!
	              // Should show '_' in DEBUG, but is UB/EB in non-DEBUG!
		slots.set(c, '?');
	#endif
	dump_state(slots);

	cout << (slots[b] = 'b') << '\n';
	cout << (slots[c] = 'c') << '\n';

	cout << "Keys obtained (a, b, c, d, err): "<< a <<", "<< b <<", "<< c <<", "<< d << ", "<< err << '\n';
	dump_state(slots);

	slots.remove(a);
	cout << "remove(a) -> is_free(a)? " << slots.is_free(a) << '\n';
	dump_state(slots);

//	cout << (slots[a] = 'A') << '\n'; // DEBUG check should fail
//	dump_state(slots);

	slots.add('Z'); // Should replace 'a'
	dump_state(slots);

	//--------------------------------------------------------------------
	cout << "Iteration via lockers::each...\n";

	// Iteration via snapshot
	slots.remove(b);
	{	L::each s(slots);
		assert(s.size() == slots.size());
		cout << "each (allocated): ";
		for (auto& x : s) { cout << x; x &= ~0x20; } // Print, then uppercase in place
		cout << '\n';
		for (std::size_t i = 0; i < s.size(); ++i) assert(slots[s.key(i)] == s[i]);

		std::size_t i = 0;
		for (auto k : s.keys()) {
			assert(k == s.key(i));
			assert(&slots[k] == &s[i]);
			++i;
		}
		assert(i == s.size());

		const auto& cs = s;
		std::size_t j = 0;
		for (auto k : cs.keys()) { assert(k == cs.key(j)); ++j; }
		assert(j == cs.size());

		dump_state(slots); // Changes went to the locker itself

		slots.set(s.key(0), '!'); // Content change: snapshot stays valid
		assert(s[0] == '!');

		slots.remove(s.key(0));
//		s[0]; // Structural change: should fail the gen. check!
	}
	{	L::each s(slots, L::Invalidate::On_Remove);
		[[maybe_unused]] auto n = s.size();
		auto k = slots.add('n'); assert(k != L::ERROR);
		std::size_t count = 0;
		for (auto old_key : s.keys()) { assert(old_key != k); ++count; }
		assert(count == n);
		assert(s.size() == n && s[0] == slots[s.key(0)]); // Still valid after an add, just doesn't see 'n'
		slots.remove(k);
//		s[0]; // Removal: should fail the del. gen. check!
	}
	{	L::each s(slots, L::Invalidate::Never);
		auto k = slots.add('u'); slots.remove(k);
		(void)s[0]; // No check at all
	}
	dump_state(slots);

	perf_test();
}
#endif // UNIT_TEST
#endif // LKR3986GH78F786D102D75426748GB
