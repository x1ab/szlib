#include "sign.hh"

//============================================================================
// [clan]g++ -x c++ -Wall -Wextra -Wconversion -Wsign-conversion -pedantic -DUNIT_TEST
// cl -EHsc -std:c++17 -TP -W4 -sdl -DUNIT_TEST

#include <iostream>

int main()
{
	using namespace sz::math;
	using namespace std;

	// Warning baseline reality-check: int -1 -> unsigned: should trigger C4245 or -Wsign-conversion:
	//unsigned chk(-3);

	unsigned z = sign(0);
		cout << z << " == 0?\n";
	unsigned u = sign(5u);
		cout << u << " == 1?\n";
	unsigned char c = sign((unsigned char)5);
		cout << (c == 1 ? "+" : "?!") << " == +?\n";
	int i = sign(-3);
		cout << i << " == -1?\n";
	float f = sign(-2.f);
		cout << f << " == -1?\n";
}
