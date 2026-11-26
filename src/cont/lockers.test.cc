//
// For perf. regression checks use -DSKIP_DIRECT_ITER to compare the total-time
// benchmark results of 0.6.x and 0.7.x!
//
// Also, don't forget to measure with asserts disabled (-DNDEBUG)!
//

#include "lockers.hh"

//============================================================================
// [clan]g++ -x c++ -Wall -Wextra -pedantic -O2 -DNDEBUG
// cl -EHsc -std:c++17 -TP -W4 -O2 -DNDEBUG

#include <iostream>
#include <chrono>
#include <type_traits> // is_same_v
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
	constexpr std::size_t N = 1'000'000;
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
#ifndef SKIP_DIRECT_ITER
	long long sum3 = 0; for (auto& x : c) sum3 += x;
	auto t7 = Clock::now();
	auto t_end = t7;
#else
	auto t_end = t6;
#endif

	assert(c.size() == N / 2 && e.size() == N / 2);
#ifndef SKIP_DIRECT_ITER
	assert(sum == sum2 && sum == sum3 && sum == (long long)(N / 2) * (N / 2)); // Sum of the odd numbers below N (= (N/2)^2, for even N)
#else
	assert(sum == sum2 &&                sum == (long long)(N / 2) * (N / 2)); // Sum of the odd numbers below N (= (N/2)^2, for even N)
#endif

	cout << "  ctor:                 " << ms(t0, t1) << " ms\n"
	     << "  add    x N:           " << ms(t1, t2) << " ms\n"
	     << "  remove x N/2:         " << ms(t2, t3) << " ms\n"
	     << "  each build (N/2):     " << ms(t3, t4) << " ms\n"
	     << "  each walk:            " << ms(t4, t5) << " ms\n"
	     << "  op[]   x N/2:         " << ms(t5, t6) << " ms\n"
#ifndef SKIP_DIRECT_ITER
	     << "  direct walk (N/2):    " << ms(t6, t7) << " ms\n"
#endif
	     << "  TOTAL:                " << ms(t0, t_end) << " ms\n"
	     << "  (checksums: " << sum << ", " << sum2
#ifndef SKIP_DIRECT_ITER
			<< ", " << sum3 
#endif
			<< ")\n"; // Also keeps the optimizer from dropping the loops
	delete &e;
	delete &c;
}

int main()
{
	using L = lockers<char, 4>;
	using K = L::key_t;

	// Verifying a fix to 0.7:
	{
		lockers<char, 4> p;
		p.add('A');
		p.add('B');

		auto it = p.keys().begin();
		auto first  = *it;
		auto second = *++it;
		cout << "keys(): " << +first << ", " << +second << '\n'; // Was 0, 66! (Should be 0, 1.)

		static_assert(std::is_same_v<
			decltype(*++it), decltype(p)::key_t
		>, "++ must preserve key-iterator semantics");
		assert(first == 0 && second == 1);
	}


	L slots;

	{	L empty;
		const L::each s(empty);
		std::size_t n = 0;
		cout << "each.keys() (empty): ";
		for (auto k : s.keys()) { (void)k; ++n; }
		cout << n << " keys\n";
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

		cout << "each.keys() (key/value): ";
		std::size_t i = 0;
		for (auto k : s.keys()) {
			assert(k == s.key(i));
			assert(&slots[k] == &s[i]);
			cout << k << ':' << slots[k] << ' ';
			++i;
		}
		cout << '\n';
		assert(i == s.size());

		const auto& cs = s;
		cout << "each.keys() (const): ";
		std::size_t j = 0;
		for (auto k : cs.keys()) { assert(k == cs.key(j)); cout << k << ' '; ++j; }
		cout << '\n';
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
		cout << "each.keys() (On_Remove, after add): ";
		std::size_t count = 0;
		for (auto old_key : s.keys()) { assert(old_key != k); cout << old_key << ' '; ++count; }
		cout << "(new " << k << " excluded)\n";
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

	//--------------------------------------------------------------------
#ifndef SKIP_DIRECT_ITER
	cout << "Direct iteration...\n";
	{	L empty;
		assert(empty.begin() == empty.end());
		for ([[maybe_unused]] auto& x : empty) assert(!"empty: no elements");
		for ([[maybe_unused]] auto k : empty.keys()) assert(!"empty: no keys");
	}
	{	L::each s(slots); // Reference for comparison
		cout << "direct (T&): ";
		std::size_t i = 0;
		for (auto& x : slots) { assert(&x == &s[i]); cout << x; ++i; }
		cout << '\n';
		assert(i == slots.size());

		cout << "direct (keys): ";
		i = 0;
		for (auto k : slots.keys()) { assert(k == s.key(i)); cout << k << ' '; ++i; }
		cout << '\n';
		assert(i == slots.size());

		i = 0;
		for (auto it = slots.begin(); it != slots.end(); ++it, ++i) {
			assert(it.key() == s.key(i) && &*it == &slots[it.key()]);
			*it = char(*it | 0x20); // Write through: lowercase in place
		}
		const L& cl = slots;
		for (auto& x : cl) { static_assert(std::is_same_v<decltype(x), const char&>); assert(x >= 'a'); }
		for (auto it = cl.cbegin(); it != cl.cend(); ++it) (void)*it;
	}
	{	// Remove-current while iterating (the one supported structural change)
		lockers<int, 200> m; // Multi-word bitmap, with gaps and empty words in between
		for (int i = 0; i < 200; ++i) (void)m.add(i);
		for (unsigned k = 0; k < 200; ++k) if (!(k % 3 == 0 || (k >= 64 && k < 128))) m.remove(k);
		// Now: multiples of 3 below 64 and from 128 up, plus the whole word 1 (64..127)
		long long expected = 0; std::size_t n = 0;
		for (unsigned k = 0; k < 200; ++k) if (!m.is_free(k)) { expected += m[k]; ++n; }
		long long sum = 0; std::size_t visited = 0;
		for (auto& x : m) { sum += x; ++visited; }
		assert(sum == expected && visited == n && n == m.size());

		// Drop all odd values, in-loop:
		visited = 0;
		for (auto it = m.begin(); it != m.end(); ++it, ++visited)
			if (*it % 2) m.remove(it.key());
		assert(visited == n);
		for ([[maybe_unused]] auto& x : m) assert(x % 2 == 0);
		cout << "remove-current: " << n << " -> " << m.size() << " elements\n";

		// Empty words before the first element:
		lockers<int, 300> h;
		for (int i = 0; i < 300; ++i) (void)h.add(i);
		for (unsigned k = 0; k < 290; ++k) h.remove(k);
		int first = -1; std::size_t cnt = 0;
		for (auto& x : h) { if (first < 0) first = x; ++cnt; }
		assert(first == 290 && cnt == 10);
	}
	{
		// Remove-current via keys(), smoke check:
		lockers<int, 128> p;
		for (int i = 0; i < 128; ++i) p.add(i);
		for (auto k : p.keys()) p.remove(k);
		assert(p.empty());
	}

#endif // SKIP_DIRECT_ITER

	perf_test();
}
