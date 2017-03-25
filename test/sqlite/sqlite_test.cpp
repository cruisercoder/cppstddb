#include <iostream>
#include <cppstddb/sqlite/database.h>
#include <cppstddb/test_suite.h>

using namespace std;

int main() {
    try {
        string uri = "file://testdb.sqlite";
        //auto db = cppstddb::sqlite::database(uri);
        //cppstddb::create_score_table(db);
        cppstddb::test_all<cppstddb::sqlite::database>(uri);

        /*
        auto db = cppstddb::sqlite::database(uri);
        //cppstddb::create_score_table(db);
        auto con = db.connection();
        auto stmt = con.statement("select 1").query();
        */

    } catch (cppstddb::database_error &e) {
        cppstddb::vertical_print(cout, e);
    } catch (exception &e) {
        cout << "exception: " << e.what() << endl;
    }
    return 0;
}

