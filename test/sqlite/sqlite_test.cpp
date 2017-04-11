#include <iostream>
#include <cppstddb/sqlite/database.h>
#include <cppstddb/test_suite.h>

using namespace std;

int main() {
    try {
		using namespace cppstddb;
        string uri = "file://testdb.sqlite";
        test_all<sqlite::database>(uri);
    } catch (cppstddb::database_error &e) {
        cppstddb::vertical_print(cout, e);
    } catch (exception &e) {
        cout << "exception: " << e.what() << endl;
    }
    return 0;
}

