#include <iostream>
#include <cppstddb/postgres/database.h>
#include <cppstddb/test_suite.h>

using namespace std;

int main() {
    try {
        string uri = "postgres://localhost/test?username=test&password=test";

        //auto db = cppstddb::postgres::database(uri);
        //cppstddb::create_score_table(db);

        //cppstddb::simple_test<cppstddb::postgres::database>(uri);
        cppstddb::test_all<cppstddb::postgres::database>(uri);


    } catch (exception &e) {
        cout << "exception: " << e.what() << endl;
    }
    return 0;
}

