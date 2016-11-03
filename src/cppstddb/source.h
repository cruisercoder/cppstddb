#ifndef SOURCE_H
#define SOURCE_H

#include <string>

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
}

#endif

