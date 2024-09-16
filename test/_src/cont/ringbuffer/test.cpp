#include "../ringbuffer.hh"

#include <iostream>
#include <string>
using namespace std;

typedef RingBuffer_Static_Int<int, 5, -1> Buffer;

void dump(Buffer& b, const char* note = "")
{
	cout << "------- " << note << " ------" << endl;

	for (int i=0; i<b.capacity(); ++i) {
		cout	<< i
			<< ":"
			<< (i == b._oldest ? " <" : "")
			<< (i == b._next ? " >" : "")
			<< "\t"
			<< b._items[i]
			<< endl;
	}

	cout << "capacity(): "	<< b.capacity() << endl;
	cout << "size(): "	<< b.size() << endl;
	cout << "empty(): "	<< (b.empty()?"true":"false") << endl;
	cout << "full(): "	<< (b.full() ?"true":"false") << endl;
}

void ifill(Buffer& b)
{
	for (int i=0; i<b.capacity(); ++i) {
		b.push_back(i);
	}
}


int main()
{
	Buffer b;
	dump(b);

	ifill(b);
	dump(b, "ifill");

	cout << "find(3): " << (b.find(3) ? "true":"false") << endl;
	cout << "find(-1): " << (b.find(-1) ? "true":"false") << endl;

	b.push_back(1000);
	b.push_back(2000);
	dump(b, "push_back 1000, 2000");

	cout << "pop_front [must be 2]: " << b.pop_front() << endl;
dump(b, "after pop_front");

	cout << "find(1000) [must be true]: " << (b.find(1000) ? "true":"false") << endl;
	cout << "find(2000) [must be true]: " << (b.find(2000) ? "true":"false") << endl;
	cout << "find(2) [must be false]: " << (b.find(2) ? "true":"false") << endl;


	cout << "pop_front: " << b.pop_front() << endl;
	cout << "pop_front: " << b.pop_front() << endl;
	cout << "pop_front: " << b.pop_front() << endl;
	cout << "pop_front: " << b.pop_front() << endl;
dump(b, "after 4 pop_front");
	cout << "pop_front: " << b.pop_front() << endl;
	cout << "pop_front: " << b.pop_front() << endl;
dump(b, "after 2 pop_front");

	ifill(b);
	dump(b, "ifill");

	cout << "pop_back: " << b.pop_back() << endl;
	cout << "pop_back: " << b.pop_back() << endl;
	cout << "pop_back: " << b.pop_back() << endl;
	cout << "pop_back: " << b.pop_back() << endl;
	cout << "pop_back: " << b.pop_back() << endl;
	cout << "pop_back: " << b.pop_back() << endl;
	dump(b, "after 6 pop_back");
}
