#ifndef UTIL_H
#define UTIL_H

#include "source.h"

namespace cppstddb {

    inline source uri_to_source(const std::string& uri) {
        auto pos = uri.find("://");
        source s;
        s.protocol.assign(uri,0,pos);
        return s;
    }

}

#endif

