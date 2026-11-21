// 0.3.0
//#define DEBUG
/*
Stable-index, stable-pointer, O(1) add/delete, "free-list-like" POD container

	Clients can obtain opaque handles (keys) of "locker slots" (cells)
	from a managed pool of a fixed number of contiguous cells (as long
	as there are free ones left), and store a value of type T there.

	T must be `trivially destructible` (POD-like): no actual destructor
	call is made on free()! (-> #25)
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
	empty cells, by-value copy on 'store' (via op=), the usual STL-like
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
	but only at the cost of degrading the performance of free() to
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

	  a) triggers conscious effor (a.k.a "thinking") when accessing elements,

	  b) offers a checkpoint in the container to catch invalid references
	     (in DEBUG mode).
*/

#ifndef LKR3986GH78F786D102D75426748GB
#define LKR3986GH78F786D102D75426748GB

#include <cstddef> // size_t
#include <cassert>

namespace sz {

template <typename T, std::size_t MAX>
class lockers
{
public:
	using key_t = std::size_t; //!! Could be some other type (likely still unsigned)...

	enum : key_t { ERROR = key_t(-1) };

	//--------------------------------------------------------------------
	// Reserve/Store/Release...

	//!! Can't call this "reserve", STL "owns" that for a diff. concept... :-(
	[[nodiscard]] key_t alloc() { // Or return ERROR
		if (full()) return ERROR; // Not assert, unlike for double free!
		++gen_;
		return _pop_free();
	}
	key_t store(const T& cargo) {
		if (key_t slot; ERROR == (slot = alloc())) return ERROR;
		store(slot, cargo);
		return slot;
	}
	T& store(key_t slot, const T& cargo) {
		assert(slot != ERROR);
		assert(slot < capacity_);
		assert(!_is_free(slot));
		return slots_[slot] = cargo; } // Support method-chaining
	void free(key_t slot) {
		assert(!empty());
		assert(slot != ERROR);
		assert(slot < capacity_);
		assert(!_is_free(slot)); // Double free?
		_push_free(slot);
		++gen_;
	}

	// Alloc/free synonyms (until I can decide which set to keep...):
	[[nodiscard]] key_t get()                { return alloc(); }
	void                release(key_t slot)  { free(slot); }

	//--------------------------------------------------------------------
	// Element access...

	const T& operator [](key_t key) const {
		assert(!empty());
		assert(key != ERROR);
		assert(key < capacity_);
		assert(!_is_free(key));
		return slots_[key]; }
	T& operator [](key_t key) {
		assert(!empty());
		assert(key != ERROR);
		assert(key < capacity_);
		assert(!_is_free(key));
		return slots_[key]; }

	//--------------------------------------------------------------------
	// Queries...

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

	// Default ctor.
	lockers()
	{
		// We're working backwards from a "fake" full state:
		assert(full()); // "full" here just means we don't have the free items pool (stack) init'd yet.

		// Put all the keys we manage into the free-stack, rendering the container state "empty":
		for (key_t k = 0; !empty(); ++k)
			_push_free(k);

		assert(empty());
		assert(size() == 0);
		assert(end_free_i_ == capacity_);

#ifdef DEBUG
		for (std::size_t i = 0; i < capacity_; ++i) {
			assert(free_stack_[i] == i);
		}
		// Note: this'll undo _push_free's '~' poisoning:
		for (std::size_t i = 0; i < capacity_; ++i) {
			*reinterpret_cast<char*>(&slots_[i]) = '_';
		}
#endif		
	}

	lockers(const lockers&) = delete; // No copy!

	// Iterable snapshot copy of the currently allocated keys.
	// Any alloc/free on the source container invalidates it (asserted on access).
	// Usage: for (auto& x : sz::lockers<...>::each(l)) ..., or
	// s.key(i) to get the slot handles (keys).
	struct each
	{
		each(lockers& src) : src_(src), gen_(src.gen_)
		{
			for (key_t k = 0; k < src.capacity_; ++k)
				if (!src._is_free(k)) keys_[size_++] = k;
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
		void _check() const { assert(gen_ == src_.gen_ && "lockers::snapshot used after alloc/free!"); }

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
	std::size_t  gen_ = 0;        // Generation counter to track container-level structural changes
	                              // (add/remove, later: realloc), to support iterator validity checking

	//--------------------------------------------------------------------
	// Internals - Utilities...

	void _push_free(key_t k) {
		assert(!empty());
		free_stack_[end_free_i_++] = k;
#ifdef DEBUG
		// Poison/mark the freed item as "discarded".
		// (Gambling on T's op= not being confused by the junk there.)
		*reinterpret_cast<char*>(&slots_[k]) = '~'; // All I can offer instead of a real ~T() for now... ;)
#endif
	}
#ifndef DEBUG
	key_t _pop_free() {
		//!!assert(!full()); //!! Already returns ERROR at run-time! Check for that at the call sites instead!
		return full() ? ERROR : free_stack_[--end_free_i_];
	}
#else
	key_t _pop_free() { if (full()) return ERROR;
		auto slot = free_stack_[--end_free_i_];
		free_stack_[end_free_i_] = (key_t)ERROR; // Mark the unused index slot
		return slot;
	}
#endif
	bool _is_free(key_t slot) const {
		for (std::size_t free_i = 0; free_i < end_free_i_; ++free_i) //! Not free_i < size()!...
			if (slot == free_stack_[free_i]) return true;
		return false;
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

	a = slots.get();      assert(a != L::ERROR); // alloc only, no init!
	b = slots.store('B'); assert(b != L::ERROR);
	c = slots.get();      assert(c != L::ERROR); // alloc only, no init!
	d = slots.get();      assert(d != L::ERROR); // alloc only, no init!

	assert(slots.full());
	err = slots.get();      assert(err == L::ERROR); // Now fail!
	err = slots.store('X'); assert(err == L::ERROR); // Ditto

	cout << slots.store(d, 'd') << '\n';
	cout << (slots[a] = 'a') << '\n';
	cout << "capacity: " << slots.capacity() << '\n';
	// Note: [c] has not yet been assigned here! Should show '_' in DEBUG mode
	//!! (but actually uninitialized UB/err in non-DEBUG!):
	dump_state<L>(slots);

	cout << (slots[b] = 'b') << '\n';
	cout << (slots[c] = 'c') << '\n';

	cout << "Keys obtained (a, b, c, d, err): "<< a <<", "<< b <<", "<< c <<", "<< d << ", "<< err << '\n';
	dump_state<L>(slots);

	slots.free(a);
	cout << "free(a) -> is_free(a)? " << slots.is_free(a) << '\n';
	dump_state<L>(slots);

//	cout << (slots[a] = 'A') << '\n'; // DEBUG check should fail
//	dump_state<L>(slots);

	slots.store('Z'); // Should replace 'a'
	dump_state<L>(slots);

	//--------------------------------------------------------------------
	cout << "Iteration via lockers::each...\n";

	// Iteration via snapshot
	slots.free(b);
	{	L::each s(slots);
		assert(s.size() == slots.size());
		cout << "each (allocated): ";
		for (auto& x : s) { cout << x; x &= ~0x20; } // Print, then uppercase in place
		cout << '\n';
		for (std::size_t i = 0; i < s.size(); ++i) assert(slots[s.key(i)] == s[i]);
		dump_state<L>(slots); // Changes went to the locker itself

		slots.store(s.key(0), '!'); // Content change: snapshot stays valid
		assert(s[0] == '!');

		slots.free(s.key(0));
//		s[0]; // Structural change: should fail the gen. check!
	}
	dump_state<L>(slots);
}
#endif // UNIT_TEST
#endif // LKR3986GH78F786D102D75426748GB
