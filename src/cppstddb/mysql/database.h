#ifndef H_MYSQL_DATABASE
#define H_MYSQL_DATABASE

#include <cppstddb/front.h>
#include <cppstddb/util.h>

#include <mysql/mysql.h>
#include <cstring>

#include <vector>
#include <cstring>

namespace cppstddb { namespace mysql {

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
                return
                    !(ret == 0 ||
                            ret == MYSQL_NO_DATA ||
                            ret == MYSQL_DATA_TRUNCATED);
            }

            template<class T> static T* check(const std::string &msg, T* ptr) {
                DB_TRACE(msg << ": " << static_cast<void*>(ptr));
                if (!ptr) raise_error(msg);
                return ptr;
            }

            static int check(const string& msg, MYSQL_STMT* stmt, int ret) {
                DB_TRACE(msg << ":" << ret);
                if (is_error(ret)) raise_error(msg,stmt,ret);
                return ret;
            }

            static void raise_error(const string& msg) {
                //throw database_error("mysql error: " ~ msg);
                throw database_error("mysql error: ");
            }

            static void raise_error(const string& msg, int ret) {
                //throw database_error("mysql error: status: " ~ to!string(ret) ~ ":" ~ msg);
                throw database_error("mysql error status: ");
            }

            static void raise_error(const string& msg, MYSQL_STMT* stmt, int ret) {
                auto err = mysql_stmt_error(stmt);
                string s;
                s += "mysql error: ";
                s += err;
                throw database_error(s.c_str());
            }

            class database {
                public:
                    database() {
                        DB_TRACE("mysql client info: " << mysql_get_client_info());
                    }
                    ~database() {
                    }
            };

            class connection {
                public:
                    connection() {
                        DB_TRACE("con");
                        mysql = check("mysql_init", mysql_init(nullptr));

                        //auto source = cppstddb::uri_to_source("mysql://127.0.0.1");

                        string server = "127.0.0.1";
                        string database = "test";
                        string username = "test";
                        string password = "test";

                        int port = 0;
                        const char *unix_socket = nullptr;
                        unsigned long clientflag = 0L;

                        check("mysql_real_connect", mysql_real_connect(
                                    mysql,
                                    server.c_str(),
                                    username.c_str(),
                                    password.c_str(),
                                    database.c_str(),
                                    port,
                                    unix_socket,
                                    clientflag));
                    }
                    ~connection() {
                        DB_TRACE("~con");
                        if (mysql) mysql_close(mysql);
                    }
                    //private:
                    MYSQL *mysql;
            };

            class statement {
                public:
                    statement(connection& con, const string& sql):sql_(sql) {
                        DB_TRACE("stmt: " << sql);
                        stmt = check("mysql_stmt_init", mysql_stmt_init(con.mysql));
                    }
                    ~statement() {
                        DB_TRACE("~stmt");
                        if (stmt) mysql_stmt_close(stmt);
                    }


                    void prepare() {
                        DB_TRACE("prepare sql: " << sql_);
                        check("mysql_stmt_prepare", stmt, mysql_stmt_prepare(
                                    stmt,
                                    sql_.c_str(),
                                    sql_.size()));

                        //binds = cast(uint) mysql_stmt_param_count(stmt);
                    }

                    statement& query() {
                        check("mysql_stmt_execute", stmt, mysql_stmt_execute(stmt));
                        return *this;
                    }

                    //private:
                    MYSQL_STMT *stmt;
                    string sql_;
            };

            struct describe_type {
                int index;
                string name;
                MYSQL_FIELD *field;
            };

            struct bind_type {
                value_type type;
                int mysql_type;
                int allocSize;
                void* data;
                unsigned long length; // check type
                my_bool is_null;
                my_bool error;
            };

            class rowset {
                public:
                    statement& stmt;
                    //Allocator *allocator;
                    unsigned int columns;

                    //using describe_type = ::describe_type;
                    //using bind_type = ::bind_type;

                    MYSQL_RES *result_metadata;
                    int status;

                    using describe_vector = std::vector<describe_type>;
                    using bind_vector = std::vector<bind_type>;
                    using mysql_bind_vector = std::vector<MYSQL_BIND>;

                    describe_vector describes;
                    bind_vector binds;
                    mysql_bind_vector mysql_binds;

                    //static const maxData = 256;

                    rowset(statement& stmt_, int rowArraySize_):
                        stmt(stmt_) {
                            //allocator = stmt.allocator;

                            result_metadata =
                                check("mysql_stmt_result_metadata",
                                        mysql_stmt_result_metadata(stmt.stmt));

                            if (!result_metadata) return; // check this
                            columns = mysql_num_fields(result_metadata);
                            DB_TRACE("columns: " << columns);

                            build_describe();
                            build_bind();
                        }

                    ~rowset() {
                        DB_TRACE("~rowset");
                        //foreach(b; bind) allocator.deallocate(b.data);
                        if (result_metadata) mysql_free_result(result_metadata);
                    }

                    void build_describe() {
                        columns = mysql_stmt_field_count(stmt.stmt);

                        describes.reserve(columns);

                        for(int i = 0; i != columns; ++i) {
                            describes.push_back(describe_type());
                            auto& d = describes.back();

                            d.index = i;
                            d.field = check("mysql_fetch_field", mysql_fetch_field(result_metadata));
                            d.name = d.field->name;

                            //DB_TRACE("describe: name: ", d.name, ", mysql type: ", d.field.type);
                            DB_TRACE("describe: name: " << d.name);
                        }
                    }

                    void build_bind() {

                        binds.reserve(columns);

                        for(int i = 0; i != columns; ++i) {
                            auto& d = describes[i];
                            binds.push_back(bind_type());
                            auto& b = binds.back();

                            // let in ints for now
                            /*
                               if (d.field->type == MYSQL_TYPE_LONG) {
                               b.mysql_type = d.field->type;
                               b.type = value_int;
                               } else if (d.field->type == MYSQL_TYPE_DATE) {
                               b.mysql_type = d.field->type;
                               b.type = value_date;
                               } else {
                               b.mysql_type = MYSQL_TYPE_STRING;
                               b.type = value_string;
                               }
                             */

                            b.mysql_type = MYSQL_TYPE_STRING;
                            b.type = value_string;

                            b.allocSize = d.field->length + 1;
                            //b.data = allocator.allocate(b.allocSize);
                            b.data = malloc(b.allocSize);
                        }

                        setup(binds, mysql_binds);
                        my_bool result = mysql_stmt_bind_result(stmt.stmt, &mysql_binds[0]);
                    }


                    static void setup(bind_vector& binds, mysql_bind_vector& mysql_binds) {
                        // make this efficient
                        mysql_binds.assign(binds.size(), MYSQL_BIND());
                        for(int i=0; i!=binds.size(); ++i) {
                            auto& b = binds[i];
                            auto& mb = mysql_binds[i];
                            memset(&mb, 0, sizeof(MYSQL_BIND)); //header?
                            mb.buffer_type = static_cast<enum_field_types>(b.mysql_type); // fix
                            mb.buffer = b.data;
                            mb.buffer_length = b.allocSize;
                            mb.length = &b.length;
                            mb.is_null = &b.is_null;
                            mb.error = &b.error;
                        }
                    }

                    //bool hasResult() {return result_metadata != null;}

                    int fetch() {
                        status = check("mysql_stmt_fetch", stmt.stmt, mysql_stmt_fetch(stmt.stmt));
                        if (!status) {
                            return 1;
                        } else if (status == MYSQL_NO_DATA) {
                            //rows_ = row_count_;
                            return 0;
                        } else if (status == MYSQL_DATA_TRUNCATED) {
                            raise_error("mysql_stmt_fetch: truncation", status);
                        }

                        raise_error("mysql_stmt_fetch", stmt.stmt, status);
                        return 0;
                    }

                    // value getters

                    /*
                       auto name(size_t idx) {
                       return describe[idx].name;
                       }
                     */

                    /*
                       template<typename T> void as(cell_t& cell, T& value) {
                       value = static_cast<const char *>(cell.bind_.data);
                       }
                     */
                    void as(const cell_t& cell, string& value) const {
                        value.assign(static_cast<const char *>(cell.bind_.data));
                    }

                    void as(const cell_t& cell, int& value) const {
                        value = *static_cast<int*>(cell.bind_.data);
                    }

                    /*
                       template<class T=int> auto as(cell_t* cell) {
                       return *static_cast<X*>(cell.bind.data);
                       }
                     */

                    /*
                       auto get(X:Date)(Cell* cell) {
                    //return Date(2016,1,1); // fix
                    MYSQL_TIME *t = cast(MYSQL_TIME*) cell.bind.data.ptr;
                    //t.year,t.month,t.day,t.hour,t.minute,t.second
                    return Date(t.year,t.month,t.day);
                    }

                    static void checkType(T)(Bind *b) {
                    int x = TypeInfo!T.type();
                    int y = b.mysql_type;
                    if (x == y) return;
                    warning("type pair mismatch: ",x, ":", y);
                    throw new DatabaseException("type mismatch");
                    }

                    // refactor as a better 1-n bind mapping
                    struct TypeInfo(T:int) {static int type() {return MYSQL_TYPE_LONG;}}
                    struct TypeInfo(T:string) {static int type() {return MYSQL_TYPE_STRING;}}
                    struct TypeInfo(T:Date) {static int type() {return MYSQL_TYPE_DATE;}}

                    }
                     */

        };

    };

    using database = basic_database<driver<default_policy>,default_policy>;

    inline auto create_database() {
        return database();
    }


}}

#endif


