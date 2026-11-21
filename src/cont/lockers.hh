// 0.2.0
//#define DEBUG
/*
Stable-index, stable-pointer, O(1) add/delete, "free-list-like" POD container

	Clients can obtain opaque handles (keys) of "locker slots" (cells)
	from a managed pool of a fixed number of contiguous cells (as long
	as there are free ones left), and store a value of type T there.

	T must be "trivially destructible" (POD-like): no actual destructor
	call is made on free()! (-> #25)

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
	(Subsequent use of the same cell is not guaranteed to have the same
	key assigned to it as before; actually, the notion of "same cell"
	isn't even defined: it's always just one isolated cell that can be
	accessed via a key, and there is no defined relation between cells
	or keys.)

	Note: upon returning the key, the content of the cell will be
	implicitly discarded: "deemed invalid", but not ~destructed.
	This means that actually destroying (destructing, deallocating)
	a tracked (stored) resource is completely out of scope for this
	of the container.

	Accessing the cell's content (i.e. to store or retrieve a T value
	value) is done via the usual key-value map semantics. There is
	no ordering defined among the elements. There isn't even iterator
	support either (so, e.g. in the STL sense, this isn't even a
	container; see later). Copying the container (considered to be
	a resource manager) is disabled, too.

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

	The owner of the container may still need to iterate through the
	elements occasionally, which can be done "manually", using the
	pool() query, and the guarantee that it's actually a sequential,
	contiguous storage of a known number of cells.

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

#include <cstdint> // size_t
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
		if (full()) return ERROR; // No assert, unlike for double free!
		else return _pop_free();
	}
	key_t store(const T& cargo) {
		if (key_t slot; ERROR == (slot = alloc())) return ERROR;
		else { store(slot, cargo); return slot; }
	}
	T& store(key_t slot, const T& cargo) {
		assert(slot != ERROR);
		assert(!_is_free(slot));
		return slots_[slot] = cargo; } // Support method-chaining
	void free(key_t slot) {
		assert(!empty());
		assert(slot != ERROR);
		assert(!_is_free(slot)); // Double free!
		_push_free(slot);
	}

	// Alloc/free synonyms (until I can decide which set to keep...):
	[[nodiscard]] key_t get()                { return alloc(); }
	void                release(key_t slot)  { free(slot); }

	//--------------------------------------------------------------------
	// Element access...

	const T& operator [](key_t key) const {
		assert(!empty());
		assert(key != ERROR);
		assert(!_is_free(key));
		return slots_[key]; }
	T& operator [](key_t key) {
		assert(!empty());
		assert(key != ERROR);
		assert(!_is_free(key));
		return slots_[key]; }

	//--------------------------------------------------------------------
	// Queries...

	bool empty()    const { assert(end_free_i_ <= capacity_); return end_free_i_ == capacity_; }
	bool full()     const { return end_free_i_ == 0; }
	auto size()     const { return capacity_ - end_free_i_; }
	auto capacity() const { assert(capacity_ == MAX); // a) for now..., b) Unbelievable: fails with MSVC /DBDEBUG! :-o
	                        return capacity_; }

	const T* pool() const { return slots_; }
	      T* pool()       { return slots_; }

	bool is_free(key_t slot) const {
//std::cerr << "- checking key at is_free: " << slot << endl;
		assert(slot != ERROR);
		assert(slot >= 0);
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
		for (auto i = 0; i < capacity_; ++i) {
			assert(free_stack_[i] == i);
		}
		for (auto i = 0; i < capacity_; ++i) {
			slots_[i] = (T)'_';
		}
#endif		
	}

	lockers(const lockers&) = delete; // No copy!

/*
	// This is a limited, not-very-useful "iterator offering" for now, just
	// to support range-based for to go through *each existing* storage slot
	// (i.e. not each *actually used* one)!
	T*       begin()       { return &slots_[0]; }
	T*       end()         { return begin() + capacity_; }
	const T* begin() const { return &slots_[0]; }
	const T* end()   const { return begin() + capacity_; }
	const T* cbegin()      { return begin(); }
	const T* cend()        { return end(); }
	// Convert auto iterators provided by a range-for back to keys:
	key_t key(const T* it) const {
//std::cerr << "- key(): " << hex << (void*) it << " -> " << dec << (it - &slots_[0]) << endl;
		return it - &slots_[0]; }
*/


protected:
	//--------------------------------------------------------------------
	// Internals - Data...

	std::size_t  capacity_ = MAX; //!! A DYNAMIC-GROWTH OPTION NOT YET IMPL.!
	                              //!! FTR, that would break the "stable addresses" trait too!
	T            slots_[MAX];
	key_t        free_stack_[MAX];
	std::size_t  end_free_i_ = 0; // Not key_t, as this is an internal iterator/index of keys! (Nice "illiteration", eh? ;) )

	//--------------------------------------------------------------------
	// Internals - Utilities...

	void _push_free(key_t k) {
		assert(!empty());
		free_stack_[end_free_i_++] = k;
#ifdef DEBUG
		slots_[k] = (T)'~'; // Mark the freed element (with some luck with T... :) )
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
		for (auto free_i = 0; free_i < end_free_i_; ++free_i) //! Not free_i < size()!...
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
#ifdef UNIT_TEST
#include <iostream>
using namespace std;
using namespace sz;

template <class T>
auto dump_state = [](const T& lockers) {
	for (auto slot = 0; slot < lockers.capacity(); ++slot) {
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

	a = slots.get();      assert(a != L::ERROR);
	b = slots.store('B'); assert(b != L::ERROR);
	c = slots.get();      assert(c != L::ERROR);
	d = slots.get();      assert(d != L::ERROR);

	assert(slots.full());
	err = slots.get();      assert(err == L::ERROR); // Now fail!
	err = slots.store('X'); assert(err == L::ERROR); // Ditto

	cout << slots.store(d, 'd') << '\n';
	cout << (slots[a] = 'a') << '\n';
	cout << "capacity: " << slots.capacity() << '\n';
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
}
#endif // UNIT_TEST
#endif // LKR3986GH78F786D102D75426748GB
