#include "listvals.hh"
#include "Args.hpp" // (from test/_src)
#include <iostream>
using namespace std;

using namespace sz;

#define TRY_SET_OR_DEFAULT(var, expr, dflt) {try {var = expr;} catch(...) {var = dflt;}}

int main(int argc, char** argv)
{
	Args args(argc, argv);

	if (!args) {
		cerr << "Usage: " << args.exename() << " [REAL ARGS...] --flags=<UNSIGNED | xHEX>" << '\n';
		cerr << "- default flags: " << args.flags << " (hex: x" << hex << args.flags << ")\n";
		return 1;
	}

//!!NOPE: listvals can't handle a map iterator!...
//	cout << listvals(args.named(), "NAMED: ");
//	cout << listvals(args.positional(), "POSITIONAL: ");

	// Apply new flags, if any, and reparse...
	string flags_s = args("flags");
	unsigned flags;
	if (flags_s[0] != 'x') {// dec
		TRY_SET_OR_DEFAULT(flags, stoul(flags_s), args.flags);
	} else { // hex
		TRY_SET_OR_DEFAULT(flags, stoul(flags_s.c_str() + 1, nullptr, 16), args.flags);
	}
//	cout << args.exename() << " "; dumpargs(args); cout << '\n';
	cout << "Applying flags: " << flags << " (hex: x" << hex << flags << ")\n";

	args.reparse(flags);

	// Dump reparsed state...
	cout << "-------- NAMED ("<< args.named().size() <<"): \n";
	for (auto& [name, val] : args.named()) {
		cout << name << listvals(val, " = ") << '\n'; // val may itself be a list!
	}
	cout << "-------- POSITIONAL ("<< args.positional().size() <<"): \n"
	     << listvals(args.positional(), "", "\n"); // only \n it if non-empty

	if (args["?"] || args["h"] || args["help"]) {
		cout << "Usage: [-V] [--moons n]" << '\n';
		return 0;
	}

	// Generated command-line...
	// Named:
	for (auto& [name, val] : args.named()) {
		if (name.length() == 1)
			cout << '-' << name << listvals(val, " ", "", " ");
		else
			cout << "--" << name << listvals(val, "=", "", " ");
		cout << " ";
	}
	// Positional:
	cout << listvals(args.positional(), "", "", " ");

	return 0;
}
