#ifndef SOURCE_H
#define SOURCE_H

#include <string>
#include <ostream>

namespace cppstddb {
    struct source {
        typedef std::string string;
        string protocol;
        string server;
        int port;
        string database;
        string username;
        string password;
    };

    std::ostream& operator<<(std::ostream& os, const source &s) {
        os << "(";
        os << "protocol: " << s.protocol;
        os << ", server: " << s.server;
        os << ", database: " << s.database;
        os << ", username: " << s.username;
        os << ", password: " << "*****";
        os << ")";
        return os;
    }
}

#endif

