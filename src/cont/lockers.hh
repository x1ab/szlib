// 0.6.0
//#define DEBUG
/*
Stable-index, stable-pointer, O(1) add/delete, "free-list-like" POD container

	Clients can obtain opaque handles (keys) of "locker slots" (cells)
	from a managed pool of a fixed number of contiguous cells (as long
	as there are free ones left), and store a value of type T there.

	T must be `trivially destructible` (POD-like): no actual destructor
	call is made on remove()! (-> #25)
	!! BUT: The byte-poisoning in DEBUG mode also implicitly imposes
	!! `trivially_constructible`, or in fact even more restrictions! :-o

	Each cell represents a resource, and the key of the cell is
	meant to be used as a handle to that resource. Each key is unique
	in the context of the container.
	(In practice, often proxy resources (real OS handles, (smart)
	pointers, (weak) references, indexes etc.) would be stored in
	the cells.)

	The intended use case is to model resource allocation (i.e. "locked"
	exclusive use -- not in terms of concurrency, but ownership).
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

	Accessing the cell's content (i.e. to store or retrieve a T value)
	is done via the usual key-value map semantics. There is no ordering
	defined among the elements. There isn't even iterator support either
	(so, e.g. in the STL sense, this isn't even a container; see later).
	Copying the container (considered to be a resource manager) is
	disabled, too.
	!! BUT: the "POD-like T" restriction goes directly against the "resource manager" assumption! :-o

	Otherwise, the usual STL-like semantics applies: default-constructed
	empty cells, by-value copy on 'set' (via op=), the usual STL-like
	queries (empty, size, capacity etc.), no checks by default (making
	it the user's responsibility to keep things valid).

	No exceptions are thrown currently. Instead, the enum value ERROR
	is returned, when trying to obtain a slot while none is available.

NOTES:

 -  Rationale for not adding iterators:

    -	The keys are opaque, and the elements are not ordered, because in
	the target use case those elements live entirely independently, not
	forming a set that would (often) need to be manipulated as a whole.

    -	Iterating over the *allocated* slots could be expensive. Note:
	just going through every *existing* (possibly free) cell makes
	little sense, and selecting the actually allocated ones would need
	an O(n) linear search *at every ++/-- operation* to find out
	if the next/prev. cell , *and* also repeating that to ignore
	empty slots -- in the worst case also ~n times, so we're somewhere
	near O(n*n) territory...
	(Keeping track of used slots, too, could make that constant time,
	but only at the cost of degrading the performance of remove() to
	linear time (if I'm not mistaken, because removing a key from the
	'used' list requires another linear search, unlike allocation).
	A better approach could be an occupancy bit mask: that would keep
	the constness of the container ops., while also allowing const-time
	iterator inc./dec.)

    -	Sloppy client-side ownership management may lead to classic dangling-
	pointer mistakes (storing direct references, while missing that other
	code paths can free the same resource).
	By using handles, instead of direct T element references (pointers,
	STL-like iterators), the explicit indirection

	  a) triggers conscious effort (a.k.a "thinking") when accessing elements,

	  b) offers a checkpoint in the container to catch invalid references
	     (in DEBUG mode; but only until the slot gets reused).
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
	static_assert(MAX < key_t(-1), "MAX doesn't fit KeyT (its top value is reserved for ERROR)");

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
		return get(key); }
	T& operator [](key_t key) {
		assert(!empty());
		assert(key != ERROR);
		assert(key < capacity_);
		assert(!_is_free(key));
		return get(key); }

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
		for (key_t k = capacity_; !empty(); )
			_push_free(--k);

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

	lockers(const lockers&) = delete; // No copy!


	//--------------------------------------------------------------------
	// Iterable snapshot copy of the currently allocated keys.
	// Any claim/add/remove on the source container invalidates it (asserted on access).
	// Usage: for (auto& x : sz::lockers<...>::each(l)) ..., or
	// s.key(i) to get the slot handles (keys).
	struct each
	{
		each(lockers& src) : src_(src), gen_(src.gen_)
		{
			// Skip empty words wholesale; walk the set bits of the rest:
			for (std::size_t w = 0; size_ < src.size() && w < sizeof src.occ_ / sizeof src.occ_[0]; ++w)
				for (auto bits = src.occ_[w]; bits; bits &= bits - 1) //! Drop lowest set bit -> Wegner's popcount (also: bits & -bits...)
					keys_[size_++] = key_t(w * OCC_BITS + _ctz(bits));
			assert(size_ == src.size());
		}

		std::size_t size() const { return size_; }
		key_t key(std::size_t i) const { assert(i < size_); return keys_[i]; }
		T& operator [](std::size_t i) { _check(); assert(i < size_); return src_.slots_[keys_[i]]; }

		struct iterator {
			each* s; std::size_t i;
			T& operator *() const { return (*s)[i]; }
			iterator& operator ++() { ++i; return *this; }
			bool operator !=(const iterator& other) const { return i != other.i; }
		};
		iterator begin() { return {this, 0}; }
		iterator end()   { return {this, size_}; }

	protected:
		void _check() const { assert(gen_ == src_.gen_ && "lockers::each used after claim/add/remove!"); }

		lockers&    src_;
		std::size_t gen_;
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
	static constexpr std::size_t OCC_BITS = sizeof(std::size_t) * 8; // CHAR_BIT would need <climits>...
	std::size_t  occ_[(MAX + OCC_BITS - 1) / OCC_BITS] = {};
	std::size_t  gen_ = 0;        // Generation counter to track container-level structural changes
	                              // (add/remove, later: realloc), to support iterator validity checking

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
	void _occ_set(key_t slot)   { occ_[slot / OCC_BITS] |=  (std::size_t(1) << (slot % OCC_BITS)); }
	void _occ_clear(key_t slot) { occ_[slot / OCC_BITS] &= ~(std::size_t(1) << (slot % OCC_BITS)); }

	// Index of the lowest set bit (bits must be != 0)
	static unsigned _ctz(std::size_t bits) {
		assert(bits);
#if defined(_MSC_VER)
		unsigned long i;
#  if defined(_WIN64)
		_BitScanForward64(&i, bits);
#  else
		_BitScanForward(&i, bits);
#  endif
		return (unsigned)i;
#else
		return (unsigned)__builtin_ctzll(bits);
#endif
	}

/*!! This unfinished draft would iterate the free slots -- which I forgot is utterly useless! :))

	struct safe_forward_iterator
	{
		safe_forward_iterator operator ++()    { return ++freeslot_ptr_; }
		safe_forward_iterator operator ++(int) { return freeslot_ptr_++; }
		safe_forward_iterator operator --()    { return --freeslot_ptr_; }
		safe_forward_iterator operator --(int) { return freeslot_ptr_--; }
		T& operator *()  { return container_->slots_[freeslot_ptr_*]; }
		T& operator ->() { return container_->slots_ + freeslot_ptr_*; }

		bool operator ==(const safe_forward_iterator& other) { return freeslot_ptr_ == other.freeslot_ptr_; }
		safe_forward_iterator& operator =(const safe_forward_iterator& other)
			{	container_ = other.container_;
				freeslot_ptr_ = other.freeslot_ptr_;
				return *this; }

		// "Forward" in not really any useful sense though, I guess!
		safe_forward_iterator(lockers* cont, lockers::key_t_* start = nullptr):
			container_(cont),
			freeslot_ptr_(start ? start : container_->free_stack_) // ... + container_->end_free_i_ with reversed ++/--
			                                                       // could respect the stack ordering, but... pointless?
			{ assert(container_ != nullptr); }
		safe_forward_iterator() = default;
	private:
		// Order matters!
		lockers* container_ = nullptr;
		lockers::key_t_* freeslot_ptr_ = nullptr;
	};

	using iterator = safe_forward_iterator;
!!*/
}; // class lockers
} // namespace sz


//----------------------------------------------------------------------------
#ifdef UNIT_TEST // g++ -x c++ lockers.hh -DUNIT_TEST -DDEBUG -Wall -Wextra -pedantic
#include <iostream>
using namespace std;
using namespace sz;

template <class T>
auto dump_state = [](const T& lockers) {
	for (std::size_t slot = 0; slot < lockers.capacity(); ++slot) {
#ifdef DEBUG
//		cout << (char)lockers[slot]; // The DEBUG extensions have already marked the free items.
		                             // Umm... But... [] access is disallowed for freed items, so:
		cout << (lockers.is_free(slot) ? '_' : (char)(lockers[slot] > 31 ? lockers[slot] & 127 : '?')); // Yuck, is_free means another loop internally!
#else		
		cout << (lockers.is_free(slot) ? '_' : (char)(lockers[slot] > 31 ? lockers[slot] & 127 : '?')); // Yuck, is_free means another loop internally!
#endif
	}
	cout << " - occupied: " << lockers.size() << endl;
};

int main()
{
	using L = lockers<char, 4>;

	L slots;
	L::key_t a = -1, b = -2, c = -3, d = -4, err = -666;

//	slots.is_free('?'); // Not a key; should fail!

	assert(slots.empty());
	dump_state<L>(slots);

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
	// Note: [c] has not yet been assigned here! Should show '_' in DEBUG mode
	//!! (but actually uninitialized UB/err in non-DEBUG!):
	dump_state<L>(slots);

	cout << (slots[b] = 'b') << '\n';
	cout << (slots[c] = 'c') << '\n';

	cout << "Keys obtained (a, b, c, d, err): "<< a <<", "<< b <<", "<< c <<", "<< d << ", "<< err << '\n';
	dump_state<L>(slots);

	slots.remove(a);
	cout << "remove(a) -> is_free(a)? " << slots.is_free(a) << '\n';
	dump_state<L>(slots);

//	cout << (slots[a] = 'A') << '\n'; // DEBUG check should fail
//	dump_state<L>(slots);

	slots.add('Z'); // Should replace 'a'
	dump_state<L>(slots);

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
		dump_state<L>(slots); // Changes went to the locker itself

		slots.set(s.key(0), '!'); // Content change: snapshot stays valid
		assert(s[0] == '!');

		slots.remove(s.key(0));
//		s[0]; // Structural change: should fail the gen. check!
	}
	dump_state<L>(slots);
}
#endif // UNIT_TEST
#endif // LKR3986GH78F786D102D75426748GB
