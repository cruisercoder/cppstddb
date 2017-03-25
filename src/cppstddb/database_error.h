#ifndef CPPSTDDB_DATABASE_ERROR_H
#define CPPSTDDB_DATABASE_ERROR_H

#include <string> 
#include <exception> 
#include <sstream> 

namespace cppstddb {

    class database_error : public std::exception {
        public:
            typedef std::string string;

            //database_error(const db_source &source);
            database_error(const string &message);
            database_error(const string &message, int retcode);
            database_error(const string &message, int retcode, const string &driver_message);

            ~database_error() throw();

            const string& message() const {return message_;}
            int retcode() const {return retcode_;}
            const string& driver_message() const {return driver_message_;}

            const char* what() const throw();

        private:
            string message_;
            int retcode_;
            string driver_message_;
            string what_;

            void build();
    };

    inline database_error::database_error(const string &message):
        message_(message),
        retcode_(0)
    {
        build();
    }

    inline database_error::database_error(
            const string &message,
            int retcode):
        message_(message),
        retcode_(retcode)
    {
        build();
    }

    inline database_error::database_error(
            const string &message,
            int retcode,
            const string &driver_message):
        message_(message),
        retcode_(retcode),
        driver_message_(driver_message)
    {
        build();
    }


    void database_error::build() {
        std::stringstream s;
        s << "message: " << message_;
        if (retcode_) s << ", retcode: " << retcode_;
        if (!driver_message_.empty()) s << ", driver_message: " << driver_message_;
        what_ = s.str();
    }

    inline database_error::~database_error() throw() {
    }

    inline const char* database_error::what() const throw() {
        return what_.c_str();
    }

    inline void vertical_print(std::ostream &os, const database_error& e) {
        os
            << "+-- database error -------------------------+\n"
            << "message: " << e.message() << "\n"
            << "driver return code: " << e.retcode() << "\n"
            << "driver error message: " << e.driver_message() << "\n"
            << "+-------------------------------------------+\n";
        os.flush();
    }

}

#endif


