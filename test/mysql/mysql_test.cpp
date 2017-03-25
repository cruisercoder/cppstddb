#include <iostream>
#include <cppstddb/mysql/database.h>
#include <cppstddb/test_suite.h>

using namespace std;

int main() {
    try {
        string uri = "mysql://localhost/test?username=test&password=test";
        //auto db = cppstddb::mysql::database(uri);
        //cppstddb::create_score_table(db);
        cppstddb::test_all<cppstddb::mysql::database>(uri);
    } catch (cppstddb::database_error &e) {
        cppstddb::vertical_print(cout, e);
    } catch (exception &e) {
        cout << "exception: " << e.what() << endl;
    }
    return 0;
}

