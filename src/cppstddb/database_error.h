#ifndef H_DATABASE_ERROR
#define H_DATABASE_ERROR

#include <string> 
#include <exception> 
#include <sstream> 

class database_error : public std::exception {
	public:
		typedef std::string string;

		//database_error(const db_source &source);
		database_error(const string &message);
		~database_error() throw();

        const char* what() const throw();

    private:
        string message_;
        string what_;
};

inline database_error::database_error(const string &message)
{
    std::stringstream s;
    s << "message: " << message;
    what_ = s.str();
}

inline database_error::~database_error() throw() {
}

inline const char* database_error::what() const throw() {
    return what_.c_str();
}


#endif


