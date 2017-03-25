#ifndef H_SQLITE_DATABASE
#define H_SQLITE_DATABASE

#include <cppstddb/front.h>
#include <cppstddb/util.h>
#include <vector>
#include <sstream>
#include <sqlite3.h>
//#include <sqlite3ext.h>
#include <cstring>

namespace cppstddb { namespace sqlite {

    using namespace front;

    /*
       template<typename T> struct convert {};

       template<> struct convert<int> {
       static auto operator()(rowset_t& r, cell_t &cell) {
       return *static_cast<int*>(cell.bind.data);
       }
       };

       template<> struct convert<string> {
       static string operator()(rowset_t& r, cell_t &cell) {
       return static_cast<const char *>(cell.bind.data);
       }
       };
     */

    template<class P> class driver {
        using string = std::string;
        using cell_t = cell<driver,P>;

        public:
        typedef P policy_type;

        static bool is_error(int ret) {
            return ret != SQLITE_OK;
        }

        static int check(const string& msg, int ret) {
            DB_TRACE(msg << ":" << ret);
            if (is_error(ret)) raise_error(msg,ret);
            return ret;
        }

        static int check(const string& msg, sqlite3* sq, int ret) {
            DB_TRACE(msg << ":" << ret);
            if (is_error(ret)) raise_error(msg,sq,ret);
            return ret;
        }

        static int check_nothrow(const string& msg, int ret) {
            if (is_error(ret)) {
                std::stringstream s;
                s << msg << ":" << ret;
                DB_ERROR(s.str());
            }
            return ret;
        }

        static void raise_error(const string& msg) {
            throw database_error(msg);
        }

        static void raise_error(const string& msg, int ret) {
            throw database_error(msg, ret);
        }

        static void raise_error(const string& msg, sqlite3* sq, int ret) {
            throw database_error(msg, ret, sqlite3_errmsg(sq));
        }

        class database {
            private:
            public:
                database() {
                    DB_TRACE("sqlite header version: " << SQLITE_VERSION);
                    DB_TRACE("sqlite version: " << sqlite3_libversion());
                }
                ~database() {
                }
        };

        class connection {
            public:
                sqlite3* sq;
                string path;
            public:
                database& db;

                connection(database& db_, const source& src):db(db_) {
                    if (src.protocol == "sqlite")
                        raise_error("uri protocol: use file instead of sqlite");
                    else if (src.protocol != "file")
                        raise_error("uri protocol must be file");

                    path = src.server;  // file is server for moment FIX

                    DB_TRACE("con: sqlite opening file: " << path);

                    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
                    check("sqlite3_open_v2", sq, sqlite3_open_v2(path.c_str(), &sq, flags, nullptr));

                }

                ~connection() {
                    DB_TRACE("~con: sqlite closing " << path);
                    if (sq) check_nothrow("sqlite3_close", sqlite3_close(sq));
                }
        };

        class statement {
            public:
                enum state_type {
                    state_init,
                    state_execute,
                };

                connection& con;
                string sql;
                state_type state;
                sqlite3* sq;
                sqlite3_stmt *st;
                bool has_rows;
                int binds;

            public:
                statement(connection& con_, const string& sql_):
                    con(con_),
                    sql(sql_),
                    sq(con_.sq),
                    state(state_init),
                    st(nullptr),
                    has_rows(false),
                    binds(0) {
                        DB_TRACE("stmt: " << sql);
                    }

                ~statement() {
                    DB_TRACE("~stmt");
                    if (st) check_nothrow("sqlite3_finalize", sqlite3_finalize(st));
                }

                void prepare() {
                    if (!st) { 
                        DB_TRACE("prepare sql: " << sql);
                        check("sqlite3_prepare_v2", sqlite3_prepare_v2(
                                    sq, 
                                    sql.c_str(), 
                                    sql.size() + 1, 
                                    &st, 
                                    nullptr));
                        binds = sqlite3_bind_parameter_count(st);
                    }
                }

                statement& query() {
                    if (state == state_execute) return *this;
                    state = state_execute;
                    int status = sqlite3_step(st);
                    DB_TRACE("sqlite3_step: status: " << status);
                    if (status == SQLITE_ROW) {
                        has_rows = true;
                    } else if (status == SQLITE_DONE) {
                        reset();
                    } else {
                        //raise_error(sq, "step error", status);
                        raise_error("step error", status);
                    }
                    return *this;
                }

                void reset() {
                    check("sqlite3_reset", sqlite3_reset(st));
                }


        };

        struct bind_type {
            value_type type;
            int idx;
            bind_type():type(value_undef),idx(0) {}
        };

        class rowset {
            public:
                statement& stmt;
                sqlite3_stmt *st;
                int columns;
                int status;

                // artifical bind array (for now)
                using bind_vector = std::vector<bind_type>;
                bind_vector binds;

            public:
                rowset(statement& stmt_, int rowArraySize_):
                    stmt(stmt_),
                    st(stmt.st),
                    columns(sqlite3_column_count(st)),
                    status(SQLITE_OK) {
                        DB_TRACE("rowset" << ", columns: " << columns);

                        // artificial bind setup
                        binds.reserve(columns);
                        for(int i = 0; i < columns; ++i) {
                            binds.push_back(bind_type());
                            auto& b = binds.back();
                            b.type = value_string;
                            b.idx = i;
                            //DB_TRACE("bind: idx: " << b.idx << ", type: " << b.type);
                        }

                    }

                //bool hasResult() {return result_metadata != null;}

                int fetch() {
                    // step already done by execute
                    return 1; // check
                }

                int next() {
                    status = sqlite3_step(st);
                    if (status == SQLITE_ROW) return 1;
                    if (status == SQLITE_DONE) {
                        stmt.reset();
                        return 0;
                    }
                    raise_error("sqlite3_step", status);
                    return 0;
                }

                auto name(size_t idx) {
                    auto ptr = sqlite3_column_name(st, idx);
                    return string(ptr,strlen(ptr));
                }

                // value getters

                void as(const cell_t& cell, string& value) const {
                    auto ptr = reinterpret_cast<const char*>(sqlite3_column_text(st, cell.bind_.idx));
                    value.assign(ptr,strlen(ptr));
                }

                void as(const cell_t& cell, int& value) const {
                    value = sqlite3_column_int(st, cell.bind_.idx);
                }

                void as(const cell_t& cell, date& value) const {
                    value = date(2017,2,3);
                }

        };

    };

    using database = basic_database<driver<default_policy>,default_policy>;

    inline auto create_database() {
        return database();
    }


}}

#endif



