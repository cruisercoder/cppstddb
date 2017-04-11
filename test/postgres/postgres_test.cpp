#include <iostream>
#include <cppstddb/postgres/database.h>
#include <cppstddb/test_suite.h>

using namespace std;

int main() {
	try {
		using namespace cppstddb;
		test_all<postgres::database>(test_uri("postgres"));
	} catch (exception &e) {
		cout << "exception: " << e.what() << endl;
	}
	return 0;
}

