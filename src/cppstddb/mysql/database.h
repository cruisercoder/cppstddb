#ifndef CPPSTDDB_DATABASE_MYSQL_H
#define CPPSTDDB_DATABASE_MYSQL_H

#include <cppstddb/front.h>
#include <cppstddb/util.h>
#include <vector>
#include <mysql/mysql.h>
#include <cstring>

namespace cppstddb { namespace mysql {

    namespace impl {

        template<class P> class database;
        template<class P> class connection;
        template<class P> class statement;
        template<class P> class rowset;
        template<class P> class bind_type;
        template<class P,class T> class field;

        template<class P> using cell_t = cppstddb::front::cell<database<P>>;

        bool is_error(int ret) {
            return 
                !(ret == 0 ||
                        ret == MYSQL_NO_DATA ||
                        ret == MYSQL_DATA_TRUNCATED);
        }

        template<class S> void raise_error(const S& msg) {
            throw database_error(msg);
        }

        template<class S> void raise_error(const S& msg, int ret) {
            throw database_error(msg, ret);
        }

        template<class S> void raise_error(const S& msg, MYSQL_STMT* stmt, int ret) {
            throw database_error(msg, ret, mysql_stmt_error(stmt));
        }

        template<class S> void check(const S& msg) {
            DB_TRACE(msg);
        }

        template<class S, class T> T* check(const S& msg, T* ptr) {
            DB_TRACE(msg << ": " << static_cast<void*>(ptr));
            if (!ptr) raise_error(msg);
            return ptr;
        }

        template<class S> int check(const S& msg, MYSQL_STMT* stmt, int ret) {
            DB_TRACE(msg << ":" << ret);
            if (is_error(ret)) raise_error(msg,stmt,ret);
            return ret;
        }

        template<class P> class database {
            public:
                using policy_type = P;
                using string = typename policy_type::string;
                using connection = connection<policy_type>;
                using statement = statement<policy_type>;
                using rowset = rowset<policy_type>;
                using bind_type = bind_type<policy_type>;
                template<typename T> using field_type = field<policy_type,T>;

                database() {
                    DB_TRACE("mysql client info: " << mysql_get_client_info());
                }
                ~database() {
                }

                string date_column_type() const {return "date";}
        };

        template<class P> class connection {
            public:
                using policy_type = P;
                using database = database<policy_type>;
                MYSQL *mysql;
            public:
                database& db;

                connection(database& db_, const source& src):db(db_) {
                    DB_TRACE("con");
                    mysql = check("mysql_init", mysql_init(nullptr));

                    int port = 0;
                    const char *unix_socket = nullptr;
                    unsigned long clientflag = 0L;

                    check("mysql_real_connect", mysql_real_connect(
                                mysql,
                                src.server.c_str(),
                                src.username.c_str(),
                                src.password.c_str(),
                                src.database.c_str(),
                                port,
                                unix_socket,
                                clientflag));
                }

                ~connection() {
                    DB_TRACE("~con");
                    if (mysql) mysql_close(mysql);
                }

        };

        template<class P> class statement {
            public:
                using policy_type = P;
                using string = typename policy_type::string;
                using connection = connection<policy_type>;
                using rowset = rowset<policy_type>;
                MYSQL_STMT *stmt;
                string sql;
                int binds;
            public:
                statement(connection& con, const string& sql_):sql(sql_),binds(0) {
                    DB_TRACE("stmt: " << sql);
                    stmt = check("mysql_stmt_init", mysql_stmt_init(con.mysql));
                }

                ~statement() {
                    DB_TRACE("~stmt");
                    if (stmt) mysql_stmt_close(stmt);
                }

                void prepare() {
                    DB_TRACE("prepare sql: " << sql);
                    check("mysql_stmt_prepare", stmt, mysql_stmt_prepare(
                                stmt,
                                sql.c_str(),
                                sql.size()));

                    binds = mysql_stmt_param_count(stmt);
                }

                statement& query() {
                    check("mysql_stmt_execute", stmt, mysql_stmt_execute(stmt));
                    return *this;
                }

        };

        template<class P> struct describe_type {
            using policy_type = P;
            using string = typename policy_type::string;
            int index;
            string name;
            MYSQL_FIELD *field;
        };

        template<class P> struct bind_type {
            value_type type;
            int mysql_type;
            int alloc_size;
            void* data;
            unsigned long length; // check type
            my_bool is_null;
            my_bool error;
        };

        template<class P> struct bind_context {
            using bind_type = bind_type<P>;
            using describe_type = describe_type<P>;
            bind_context(const describe_type& describe_, bind_type& bind_):
                row_array_size(1),describe(describe_),bind(bind_) {}
            int row_array_size;
            const describe_type& describe;
            bind_type& bind;
        };

        template<class P> struct bind_info {
            using bind_context = bind_context<P>;
            static const bind_info info[];
            int mysql_type;
            void (*bind)(bind_context& ctx);
        };

        template<class P> void binder(bind_context<P>& ctx) {
            auto type = ctx.describe.field->type;
            const bind_info<P> *info = &bind_info<P>::info[0];
            for (auto i = &info[0]; i->mysql_type; ++i){
                if (i->mysql_type == type) {
                    i->bind(ctx);
                    return;
                }
            }

            /*
            std::stringstream s;
            s << "binder: type not found: " << ctx.describe.field->type;
            throw database_error(s.str());
            */

            DB_WARN("type not found, binding to string for now: " << type);
            bind_string(ctx);
        }

        template<class P> void bind_long(bind_context<P>& ctx) {
            ctx.bind.mysql_type = ctx.describe.field->type;
            ctx.bind.type = value_int;
            ctx.bind.alloc_size = ctx.describe.field->length; // ???
        }

        template<class P> void bind_date(bind_context<P>& ctx) {
            ctx.bind.mysql_type = ctx.describe.field->type;
            ctx.bind.type = value_date;
            ctx.bind.alloc_size = sizeof(MYSQL_TIME);
        }

        template<class P> void bind_string(bind_context<P>& ctx) {
            ctx.bind.mysql_type = ctx.describe.field->type;
            ctx.bind.type = value_string;
            ctx.bind.alloc_size = ctx.describe.field->length + 1;
        }

        template<class P> const bind_info<P> bind_info<P>::info[] = {
            {MYSQL_TYPE_TINY, bind_long<P>},
            {MYSQL_TYPE_SHORT, bind_long<P>},
            {MYSQL_TYPE_LONG, bind_long<P>},
            {MYSQL_TYPE_LONGLONG, bind_long<P>},
            {MYSQL_TYPE_DATE, bind_date<P>},
            // MYSQL_TYPE_DATETIME
            {MYSQL_TYPE_STRING, bind_string<P>},
            {0,nullptr}
        };

        template<class P> class rowset {
            public:
                using policy_type = P;
                using cell_t = cell_t<policy_type>;
                using statement = statement<policy_type>;
                using bind_type = bind_type<policy_type>;
                using bind_context = bind_context<policy_type>;
                statement& stmt;
                //Allocator *allocator;
                unsigned int columns;

                MYSQL_RES *result_metadata;
                int status;

                using describe_type = describe_type<policy_type>;
                using describe_vector = std::vector<describe_type>;
                using bind_vector = std::vector<bind_type>;
                using mysql_bind_vector = std::vector<MYSQL_BIND>;

                describe_vector describes;
                bind_vector binds;
                mysql_bind_vector mysql_binds;

                //static const maxData = 256;

            public:
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

                    for(auto&& b : binds) {
                        //DB_TRACE("free: " << ", data: " << b.data << ", size: " << b.alloc_size);
                        free(b.data);
                    }

                    if (result_metadata) {
                        check("mysql_free_result");
                        mysql_free_result(result_metadata);
                    }
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

                        bind_context ctx(d,b);
                        binder(ctx);

                        //b.data = allocator.allocate(b.alloc_size);

                        b.data = malloc(b.alloc_size);
                        //DB_TRACE("malloc: " << i << ", data: " << b.data << ", size: " << b.alloc_size);
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
                        mb.buffer_length = b.alloc_size;
                        mb.length = &b.length;
                        mb.is_null = &b.is_null;
                        mb.error = &b.error;
                    }
                }

                //bool hasResult() {return result_metadata != null;}

                int fetch() {
                    return next();
                }

                int next() {
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

                auto name(size_t idx) {
                    return describes[idx].name;
                }

        };


        template<class P, typename T> struct field {};

        template<class P> struct field<P,std::string> {
            static std::string as(const rowset<P>& r, const cell_t<P>& cell) {
                return static_cast<const char *>(cell.bind_.data);
            }
        };

        template<class P> struct field<P,int> {
            static int as(const rowset<P>& r, const cell_t<P>& cell) {
                return *static_cast<int*>(cell.bind_.data);
            }
        };

        template<class P> struct field<P,date_t> {
            static date_t as(const rowset<P>& r, const cell_t<P>& cell) {
                auto& t = *static_cast<MYSQL_TIME*>(cell.bind_.data);
                return date_t(t.year, t.month, t.day);
            }
        };

    }

    using database = cppstddb::front::basic_database<impl::database<default_policy>>;

    inline auto create_database() {
        return database();
    }


}}

#endif


