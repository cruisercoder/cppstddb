# cppstddb
A reference implementation for a modern C++ database client interface.

### Status
This is an early stage project and it needs a lot of work before it is ready for
serious use. Only the Mysql driver is present, but more will appear shortly.

### Planned Design Highlights (WIP)
- A database and driver neutral interface specification
- Reference counted value objects provide ease of use
- A header only design
- Support for direct and polymorphic interfaces 
- Range-based for loop support
- Range V3 support
- A [fluent](http://en.wikipedia.org/wiki/Fluent_interface) style interface
- URL style connection strings
- Reference implementations: ODBC, sqlite, mysql, oracle, postgres, freetds (MS SQL)
- Scalar & array input binding
- Internally managed output binding 
- Connection pooling

## Usage

This is a header only library.  Just add the src directory to your include path.
The only requirements are a C++ 17 compiler and any dependences that the underlying
database driver requires.  You will also need to ensure that the underlying database client library
is installed (libmysql for mysql, for example) in your environment.

Here is a simple Unix GCC compile/run example:

```bash
g++ -c db_demo.cpp -std=c++1y -I$HOME/src/cppstddb/src
g++ -o db_demo db_demo.o -lpthread -lmysqlclient
./db_demo
```
The runtime logging level is warn (which includes ERROR and WARN levels), but can be raised to
include higher logging levels (INFO,DEBUG,TRACE) by setting the CPPSTDDB_LOG_LEVEL environment variable.  Example:

```bash
export CPPSTDDB_LOG_LEVEL=TRACE
```

## Examples

#### simple query write to stdout

```cpp
using namespace cppstddb::mysql;

auto db = cppstddb::mysql::create_database();
db
    .statement("select * from score")
    .query()
    .rows()
    .write(cout);
```

#### simple range-based for loop example 

```cpp
auto db = cppstddb::mysql::create_database();
auto rowset = db.connection().statement("select * from score").query().rows();
for(auto row : rowset) {
    auto f = row[0];
    std::cout << f << "\n";
}
```

#### use of standard algorithm / lambda

```cpp
auto db = cppstddb::mysql::create_database();
auto r = db.statement("select name,score from score").query().rows();

auto key = "b";
auto i = find_if(r.begin(), r.end(), [key](auto row) { 
        return row[0].str() == key; });
assert(i != r.end());
auto row = *i;
cout << row["name"] << ":" << row["score"] << "\n";

```

## The Test Suite

The test suite is a set of templated test cases for use in testing the
the direct drivers.  Currently, it does not use a test framework just to keep
the dependency factor low.  It currently uses the ninja build system but
this depencency may also be removed.

Running the test suite (mysql example):

```bash
ninja -C test/mysql
```

