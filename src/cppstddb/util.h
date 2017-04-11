#ifndef CPPSTDDB_UTIL_H
#define CPPSTDDB_UTIL_H

#include "source.h"

namespace cppstddb {

    template<class iterator, class sentinel>
        bool find_key(iterator& iter, sentinel end, const std::string& key) {
            // nothing fancy here yet
            if (iter == end || key.empty()) return false;
            auto c = key[0];
            do {
                auto it = iter;
                if (*it == c) {
                    auto i = key.begin(), e = key.end();
                    while (*it == *i && ++i != e && ++it != end);
                    if (i == e) return true;
                }
            } while (++iter != end);
            return false;
        }

    template<class iterator, class sentinel>
        bool get_token_and_key(iterator& iter, sentinel end, const std::string& key, std::string& value) {
            // get token followed by key. if key not found, return false but assign value anyway
            auto start = iter;
            if (!find_key(iter, end, key)) {
                value.assign(start, end);
                return false;
            }
            value.assign(start, iter);
            iter += key.size();
            return true;
        }

    template<class iterator, class sentinel, class string>
        void get_qs_key_value(
                iterator& i, 
                sentinel e, 
                string& key, 
                string& value) {
            auto s = i;
            while (i != e && *i != '=' ) ++i;
            if (i == e) return;
            key.assign(s,i);
            if (++i == e) return;
            s = i;
            while (i != e && *i != '&' ) ++i;
            value.assign(s,i);
            if (i != e) ++i;
        }

    inline void key_value_to_source(
            const std::string& key, 
            const std::string& value,
            source& src) {

        // better logic soon
        if (key == "username") {
            src.username = value;
        } else if (key == "password") {
            src.password = value;
        }
    }

    inline auto get_uri(
            const std::string& protocol,
            const std::string& host,
            const std::string& database,
            const std::string& username,
            const std::string& password) {
        std::stringstream s;
        s
            << protocol << "://"
            << host << "/"
            << database << "?"
            << "username=" << username
            << "&password=" << password;
        return s.str();
    }


    inline source uri_to_source(const std::string& uri) {
        // example: mysql://127.0.0.1";
        source s;

        auto iter = uri.begin(), end = uri.end();
        if (! get_token_and_key(iter, end, "://", s.protocol)) return s;
        if (! get_token_and_key(iter, end, "/", s.server)) return s;
        if (! get_token_and_key(iter, end, "?", s.database)) return s;

        while (iter != end) {
            std::string key,value;
            get_qs_key_value(iter, end, key, value);
            key_value_to_source(key, value, s);
        }

        return s;
    }

}

#endif

