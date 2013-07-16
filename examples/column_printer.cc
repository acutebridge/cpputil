#include <iostream>
#include <string>

#include "include/column_printer.h"

using namespace cpputil;
using namespace std;

struct Double {
	string x;
	string y;
};

struct Triple {
	int x;
	int y;
	int z;
};

struct Sext {
	Triple t1;
	Triple t2;
};

ostream& operator<<(ostream& os, const Double& d) {
	os << d.x << endl;
	os << d.y;
	return os;
}

ostream& operator<<(ostream& os, const Triple& t) {
	os << t.x << endl;
	os << t.y << endl;
	os << t.z;
	return os;
}

ostream& operator<<(ostream& os, const Sext& s) {
	os << s.t1 << endl;
	os << s.t2;
	return os;
}

int main() {
	ColumnPrinter cp(cout);
	cp.set_vspace(3);
	
	Double d{"Hello", "World!!!"};
	Triple t{1,2,3};
	Sext s{{1,2,3},{4,5,6}};

	cp << col("Col 1", d) << col("Col 2", t) << col("Col 3", s) << endline;

	return 0;
}