#ifndef H_POSTGRES_DATABASE
#define H_POSTGRES_DATABASE

#include <cppstddb/front.h>
#include <cppstddb/util.h>
#include <cppstddb/endian.h>
#include <vector>
#include <libpq-fe.h>
#include <cstring>

/* from catalog/pg_type.h,
   this header location appears to jump around so 
   avoiding include issues for now */

static const int BOOLOID = 16;
static const int BYTEAOI = 17;
static const int CHAROID = 18;
static const int NAMEOID = 19;
static const int INT8OID = 20;
static const int INT2OID = 21;
static const int INT2VECTOROID = 22;
static const int INT4OID = 23;
static const int REGPROCOID = 24;
static const int TEXTOID = 25;
static const int OIDOID = 26;
static const int TIDOID = 27;
static const int XIDOID = 28;
static const int CIDOID = 29;
static const int OIDVECTOROID = 30;
static const int VARCHAROID = 1043;
static const int DATEOID = 1082;


namespace cppstddb { namespace postgres {

    using namespace front;

    template<class P> class driver {
        using string = std::string;
        using cell_t = cell<driver,P>;

        public:
        typedef P policy_type;

        static bool is_error(int ret) {
            /*
               return 
               !(ret == 0 ||
               ret == MYSQL_NO_DATA ||
               ret == MYSQL_DATA_TRUNCATED);
             */
            return 0;
        }

        template<class T> static T* check(const std::string &msg, T* ptr) {
            /*
               DB_TRACE(msg << ": " << static_cast<void*>(ptr));
               if (!ptr) raise_error(msg);
               return ptr;
             */
            return 0;
        }

        /*
           static int check(const string& msg, PGconn* con, int result) {
           DB_TRACE(msg << ":" << result);
           if (is_error(result)) raise_error(msg,stmt,ret);
           return ret;
           }
         */

        static void raise_error(const string& msg) {
            string s;
            throw database_error(s);
        }

        static void raise_error(PGconn *con, const string& msg) {
            string s;
            s += msg;
            s += ", ";
            s += PQerrorMessage(con);
            DB_ERROR("raise_error: " << s);
            throw database_error(s);
        }

        /*
           static void raise_error(const string& msg, int result) {
        //throw database_error("postgres error: status: " ~ to!string(result) ~ ":" ~ msg);
        throw database_error("postgres error status: ");
        }

        static void raise_error(const string& msg, PGconn* con, int ret) {
        auto err = mysql_stmt_error(stmt);
        string s;
        s += "mysql error: ";
        s += err;
        throw database_error(s.c_str());
        }
         */

        class database {
            public:
                database() {
                    DB_TRACE("db");
                    //DB_TRACE("mysql client info: " << mysql_get_client_info());
                }
                ~database() {
                    DB_TRACE("~db");
                }
        };

        class connection {
            public:
                database& db;
                PGconn *con;

                connection(database& db_, const source& src):db(db_) {
                    DB_TRACE("con, source: " << src);

                    string conninfo;
                    conninfo += "host=";
                    conninfo += src.server;
                    conninfo += " dbname=";
                    conninfo += src.database;
                    DB_TRACE("conninfo:" << conninfo);
                    con = PQconnectdb(conninfo.c_str());
                    if (PQstatus(con) != CONNECTION_OK) raise_error(con, "login error");
                }

                ~connection() {
                    DB_TRACE("~con");
                    PQfinish(con);
                }
        };

        class statement {
            public:
                //private:
                PGconn *con;
                PGresult *prepare_res;
                PGresult *res;
                string sql_;
                string name;

                std::vector<char*> bindValue;
                std::vector<Oid> bindtype;
                std::vector<int> bindLength;
                std::vector<int> bindFormat;
            public:

                statement(connection& c, const string& sql):con(c.con), sql_(sql) {
                    DB_TRACE("stmt: " << sql);
                }

                ~statement() {
                    DB_TRACE("~stmt");
                }

                statement& query() {
                    if (!prepare_res) prepare();
                    auto n = bindValue.size();
                    int resultFormat = 1;

                    res = PQexecPrepared(
                            con,
                            name.c_str(),
                            n,
                            n ? static_cast<char **>(&bindValue[0]) : nullptr,
                            n ? static_cast<int*>(&bindLength[0]) : nullptr,
                            n ? static_cast<int*>(&bindFormat[0]) : nullptr,
                            resultFormat);
                    return *this;
                }

                void prepare()  {
                    const Oid* paramTypes;
                    prepare_res = PQprepare(
                            con,
                            name.c_str(),
                            sql_.c_str(),
                            0,
                            paramTypes);
                }

        };

        struct describe_type {
            int dbType;
            int fmt;
            string name;
        };

        struct bind_type {
            value_type type;
            int idx;
            //int fmt;
            //int len;
            //int isNull; 
        };

        class rowset {
            public:
                statement& stmt;
                PGconn *con;
                PGresult *res;
                unsigned int columns;
                int status;
                int row;
                int rows;
                bool hasResult_;
            public:
                using describe_vector = std::vector<describe_type>;
                using bind_vector = std::vector<bind_type>;
                //using mysql_bind_vector = std::vector<MYSQL_BIND>;

                describe_vector describes;
                bind_vector binds;
                //mysql_bind_vector mysql_binds;

                //static const maxData = 256;

                rowset(statement& stmt_, int rowArraySize_):
                    stmt(stmt_),
                    con(stmt_.con),
                    res(stmt.res),
                    columns(0),
                    row(0),
                    rows(0)
            {
                setup();
                build_describe();
                build_bind();
            }

                ~rowset() {
                    DB_TRACE("~rowset");
                    //foreach(b; bind) allocator.deallocate(b.data);
                    //if (result_metadata) mysql_free_result(result_metadata);
                }

                bool setup() {
                    if (!res) {
                        DB_TRACE("no result");
                        return false;
                    }
                    status = PQresultStatus(res);
                    rows = PQntuples(res);

                    // not handling PGRESS_SINGLE_TUPLE yet
                    if (status == PGRES_COMMAND_OK) {
                        close();
                        return false;
                    } else if (status == PGRES_EMPTY_QUERY) {
                        close();
                        return false;
                    } else if (status == PGRES_TUPLES_OK) {
                        return true;
                    } else raise_error("setup error");
                    return true;
                }

                void build_describe() {
                    // called after next()
                    columns = PQnfields(res);
                    DB_TRACE("build describe: columns: " << columns);

                    for (int col = 0; col != columns; col++) {
                        describes.push_back(describe_type());
                        auto& d = describes.back();

                        d.dbType = static_cast<int>(PQftype(res, col));
                        d.fmt = PQfformat(res, col);
                        d.name = PQfname(res, col);
                    }
                }

                void build_bind() {
                    // artificial bind setup
                    binds.reserve(columns);
                    for(int i = 0; i < columns; ++i) {
                        auto& d = describes[i];
                        binds.push_back(bind_type());
                        auto& b = binds.back();

                        b.type = value_string;
                        b.idx = i;
                        switch(d.dbType) {
                            case VARCHAROID: b.type = value_string; break;
                            case INT4OID: b.type = value_int; break;
                            case DATEOID: b.type = value_date; break;
                            default: throw new database_error("unsupported type");
                        }
                        //DB_TRACE("dbType: " << d.dbType << ", type: " << b.type);
                    }
                }

                int fetch() {
                    return 1;
                }

                int next() {
                    return ++row != rows ? 1 :0;
                }

                void close() {
                    if (!res) raise_error("couldn't close result: result was not open");
                    res = PQgetResult(con);
                    if (res) raise_error("couldn't close result: was not finished");
                    res = nullptr;
                }

                /*
                   template<typename T> void as(cell_t& cell, T& value) {
                   value = static_cast<const char *>(cell.bind_.data);
                   }
                 */
                void as(const cell_t& cell, string& value) const {
                    value.assign(static_cast<const char *>(data(cell.bind_.idx)));
                }

                void as(const cell_t& cell, int& value) const {
                    value = big4_to_native(data(cell.bind_.idx));
                }

                /*
                   template<class T=int> auto as(cell_t* cell) {
                   return *static_cast<X*>(cell.bind.data);
                   }
                 */

                void* data(int col) const {return PQgetvalue(res, row, col);}
                bool isNull(int col) const {return PQgetisnull(res, row, col) != 0;}
                int type(int col) const {return describes[col].dbType;}
                int fmt(int col) const {return describes[col].fmt;}
                int len(int col) const {return PQgetlength(res, row, col);}
        };

    };

    using database = basic_database<driver<default_policy>,default_policy>;

    inline auto create_database() {
        return database();
    }


}}

#endif


