#include <iostream>
#include <cppstddb/mysql/database.h>
#include <cppstddb/test_suite.h>

using namespace std;

int main() {
    try {
        cppstddb::test_all<cppstddb::mysql::database>("mysql");
    } catch (exception &e) {
        cout << "exception: " << e.what() << endl;
    }
}

