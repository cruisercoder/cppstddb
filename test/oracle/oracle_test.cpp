#include <iostream>
#include <cppstddb/oracle/database.h>
#include <cppstddb/test_suite.h>

using namespace std;

int main() {
    try {
		using namespace cppstddb;
        //test_all<oracle::database>(test_uri("oracle://localhost:1521"));
        test_all<oracle::database>(test_uri("oracle://localhost"));
    } catch (cppstddb::database_error &e) {
        cppstddb::vertical_print(cout, e);
    } catch (exception &e) {
        cout << "exception: " << e.what() << endl;
    }
    return 0;
}


