#include <iostream>
#include <cppstddb/mysql/database.h>
#include <cppstddb/test_suite.h>

using namespace std;

int main() {
    try {
		using namespace cppstddb;
        test_all<mysql::database>(test_uri("mysql"));
    } catch (cppstddb::database_error &e) {
        cppstddb::vertical_print(cout, e);
    } catch (exception &e) {
        cout << "exception: " << e.what() << endl;
    }
    return 0;
}

