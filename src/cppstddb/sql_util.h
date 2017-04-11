#ifndef CPPSTDDB_SQL_UTIL_H
#define CPPSTDDB_SQL_UTIL_H

#include <string>
#include <cppstddb/log.h>
#include <cppstddb/database_error.h>

namespace cppstddb {

    template<class database> void drop_table(database& db, const std::string& table) {
        try {
            std::string s;
            s += "drop table ";
            s += table;
            db.query(s);
        } catch (database_error& e) {
            DB_WARN("drop table error (ignored): " << e.what());
        }
    }

}

#endif


