// 0.7.1
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
	defined among keys or elements. Sequential access to all the stored
	items is available either directly (`for (auto& x : pool)`, or
	`pool.keys()`; forward-only), or via `lockers::each` snapshots
	(stable, indexable). See the Iteration notes below!

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
	way for adding iteration support to the locker API directly. That
	was done in 0.7.0: `lockers::iterator` / `const_iterator` (forward-
	only), plus `keys()`. These are for one-off passes; see the notes at
	the iterator impl. for what changes in the container are allowed (i.e.
	removing the current element is OK; adding/removing others needs `each`.)

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

protected: // (Declared early, as the iterators below need them.)
	// Occupancy bitmap word type & geometry (see occ_ below):
	using occ_word_t = std::size_t;
	static constexpr std::size_t OCC_BITS  = sizeof(occ_word_t) * 8; // CHAR_BIT would need <climits>...
	static constexpr std::size_t OCC_WORDS = (MAX + OCC_BITS - 1) / OCC_BITS;
public:

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
	const T& cget(key_t slot) const { // A sad dup. of `get() const` just for the asserts!... :-/
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
	auto capacity() const { assert(capacity_ == MAX); //!! For now...
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
	// Direct iteration over the occupied cells...
	//
	// Usage: for (auto& x : pool) ...             // T& (const T& on a const pool)
	//        for (auto k : pool.keys()) ...       // key_t
	//        for (auto it = pool.begin(); it != pool.end(); ++it)
	//            use(it.key(), *it);              // Both
	//
	//! The iter. sequence following an ascending slot index order is
	//! just an implem. detail, not guranteed by the API! It may change
	//! internally (e.g. for various policies later)!
	//
	// Some extra cost over O(1) is skipping empty bitmap words (OCC_BITS
	// cells at a time) between elements.
	//
	// (The walk ends when each live element has been visited, skipping
	// any trailing empty cells.)
	//
	// Changing the container while iterating:
	//
	//    - Updating element *values* (via *it, set(), [] etc.) is fine.
	//
	//    - remove()-ing the *current* element is also fine: ++ doesn't
	//      consult the freed cell, as the iterator has already captured
	//      everything it needs to advance (its own copy of the current
	//      bitmap word, and a count of the elements still to visit).
	//
	//    - Adding or removing any *other* element is not supported: the
	//      iterator would then skip, or try to visit, the wrong cells.
	//      This is only caught where it's cheap to do so (via asserts):
	//      dereferencing a freed cell, or running out of bitmap words
	//      before visiting all the elements counted at begin(). (In the
	//      latter case, with NDEBUG, the iterator just becomes end().)
	//      Take an `each` snapshot instead, if you need to do that!
	//
	// Comparison:
	//
	//    - Iterators of the same container compare equal when they have
	//      the same number of elements *left* to visit (so that the last
	//      one would be equal to end(), which is defined as "0 left").
	//      (This is to support removing the current element without
	//      remembering the original count.)
	//
	//    - Comparison is only meaningful between iterators of the same 
	//      iteration pass.
	//
protected:
	template <typename L, typename V> // L: [const] lockers, V: [const] T
	class _iterator_impl {
		friend class lockers;

		L*          c_;
		std::size_t w_    = 0; // Index of the current bitmap word
		occ_word_t  bits_ = 0; // Not-yet-visited set bits of word w_ (the lowest is the current element)
		std::size_t left_;     // Number of elements still to visit, including the current one (0: end)

		_iterator_impl(L* c, std::size_t left) : c_(c), left_(left) {
			if (left_) { bits_ = c_->occ_[0]; _seek(); }
		}

		// Move on to the next nonempty bitmap word, if the current one is used up:
		void _seek() {
			while (!bits_) {
				if (++w_ == OCC_WORDS) {
					assert(!"lockers iteration: ran out of elements (some removed while iterating?)");
					left_ = 0; // Become end()
					return;
				}
				bits_ = c_->occ_[w_];
			}
		}

		// Slot index of the current element. (The iterator works with indexes
		// internally, and only converts to keys at the API boundary; see #27.)
		std::size_t _index() const {
			assert(left_ && "lockers iteration: accessing end()");
			return w_ * OCC_BITS + _ctz(bits_);
		}

	public:
		key_t key() const { return _index_to_key(_index()); }

		V& operator *() const {
			auto i = _index();
			assert(c_->occ_[w_] >> (i % OCC_BITS) & 1 && "lockers iteration: cell freed while iterating?");
			return c_->slots_[i];
		}
		V* operator ->() const { return &**this; }

		_iterator_impl& operator ++() {
			assert(left_ && "lockers iteration: ++ on end()");
			if (--left_) {
				bits_ &= bits_ - 1; // Drop the current (lowest set) bit
				_seek();
			}
			return *this;
		}

		bool operator ==(const _iterator_impl& other) const { assert(c_ == other.c_); return left_ == other.left_; }
		bool operator !=(const _iterator_impl& other) const { return !(*this == other); }
	};

public:
	using iterator       = _iterator_impl<lockers, T>;
	using const_iterator = _iterator_impl<const lockers, const T>;

	iterator       begin()        { return {this, size()}; }
	iterator       end()          { return {this, 0}; }
	const_iterator begin()  const { return {this, size()}; }
	const_iterator end()    const { return {this, 0}; }
	const_iterator cbegin() const { return begin(); }
	const_iterator cend()   const { return end(); }

	//--------------------------------------------------------------------
	// Snapshots...
	//
	// Usage: for (auto& x : sz::lockers<...>::each(l)) ...
	//        Or use snapshot.key(i) to get the slot handles instead.
	//        Also, for iterating over *keys* instead of T refs, see .keys() below!
	//
	// Note:
	//
	//    - To support static construction (and trivial copyability), the key list
	//      is not dynamically allocated to size() size, but rather a fixed-size
	//      array of MAX keys! (Which means much of the array size will be likely
	//      unused, and copying would also be wasteful/expensive with huge cell
	//      counts (i.e. capacity, i.e. MAX).)
	//

	// Change-tracking policy: which changes of the source container
	// invalidate the snapshot (asserted on access)? -> #27
	enum Invalidate {
		Never,         // Trust the app
		On_Any_Change, // Claim/add/remove (default)
		On_Remove      // Only bail on new removes (newly added items just won't be in the snapshot)
	};

	// Keys of occupied cells
	struct key_range {
		const lockers* c_;
		struct iterator {
			const_iterator it_;
			key_t operator *() const { return it_.key(); }
			iterator& operator ++() { ++it_; return *this; }
			bool operator ==(const iterator& rhs) const { return it_ == rhs.it_; }
			bool operator !=(const iterator& rhs) const { return it_ != rhs.it_; }
		};
		iterator begin() const { return {c_->begin()}; }
		iterator end()   const { return {c_->end()}; }
	};
	key_range keys() const { return {this}; }

	// Snapshot copy of the currently allocated keys
	struct each
	{
		each(lockers& src, Invalidate mode = On_Any_Change)
			: src_(&src), mode_(mode), gen_(src.gen_), del_gen_(src.del_gen_)
		{
			assert(src_);
			for (auto k : src.keys()) keys_[size_++] = k;
			assert(size_ == src_->size());
		}

		std::size_t size() const { return size_; }

		key_t key(std::size_t i) const {
			assert(i < size_);
		#ifndef NDEBUG // The empty call does make a difference in hot loops!
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
		std::size_t size_ = 0;
		key_t       keys_[MAX];
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
	occ_word_t   occ_[OCC_WORDS] = {};
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
		return !(occ_[slot / OCC_BITS] >> (slot % OCC_BITS) & 1);
	}
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

#endif // LKR3986GH78F786D102D75426748GB
