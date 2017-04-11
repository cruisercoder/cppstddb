#ifndef CPPSTDDB_DATE_H
#define CPPSTDDB_DATE_H
#include <iostream>

namespace cppstddb {

    // named date_t to avoid conflicts with postgres

    class date_t {
        private:
            int year_,month_,day_;

        public:
            date_t():year_(0),month_(0),day_(0) {}
            date_t(int y,int m,int d):year_(y),month_(m),day_(d) {}

            auto year() const {return year_;}
            auto month() const {return month_;}
            auto day() const {return day_;}
    };

    inline std::ostream& operator<<(std::ostream &os, const date_t& d) {
        // very crude
        os << d.year() << '-' << d.month() << '-' << d.day();
        return os;
    }

}

#endif


