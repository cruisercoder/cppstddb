#ifndef CPPSTDDB_DATABASE_SQLITE_H
#define CPPSTDDB_DATABASE_SQLITE_H

#include <cppstddb/front.h>
#include <cppstddb/util.h>
#include <cppstddb/date_parse.h>
#include <vector>
#include <sstream>
#include <sqlite3.h>
//#include <sqlite3ext.h>
#include <cstring>

namespace cppstddb { namespace sqlite {

	namespace impl {

		template<class P> class database;
		template<class P> class connection;
		template<class P> class statement;
		template<class P> class rowset;
		template<class P> class bind_type;
		template<class P,class T> class field;

		template<class P> using cell_t = cppstddb::front::cell<database<P>>;

		bool is_error(int ret) {
			return ret != SQLITE_OK;
		}

		template<class S> void raise_error(const S& msg) {
			throw database_error(msg);
		}

		template<class S> void raise_error(const S& msg, int ret) {
			throw database_error(msg, ret);
		}

		template<class S> void raise_error(const S& msg, sqlite3* sq, int ret) {
			throw database_error(msg, ret, sqlite3_errmsg(sq));
		}

		template<class S> int check(const S& msg, int ret) {
			DB_TRACE(msg << ":" << ret);
			if (is_error(ret)) raise_error(msg,ret);
			return ret;
		}

		template<class S> int check(const S& msg, sqlite3* sq, int ret) {
			DB_TRACE(msg << ":" << ret);
			if (is_error(ret)) raise_error(msg,sq,ret);
			return ret;
		}

		template<class S> int check_nothrow(const S& msg, int ret) {
			if (is_error(ret)) {
				std::stringstream s;
				s << msg << ":" << ret;
				DB_ERROR(s.str());
			}
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

                string date_column_type() const {return "text";}

			public:
				database() {
					DB_TRACE("sqlite header version: " << SQLITE_VERSION);
					DB_TRACE("sqlite version: " << sqlite3_libversion());
				}
				~database() {
				}
		};

		template<class P> class connection {
			public:
				using policy_type = P;
				using string = typename policy_type::string;
				using database = database<policy_type>;
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

		template<class P> class statement {
			public:
				using policy_type = P;
				using string = typename policy_type::string;
				using connection = connection<policy_type>;
				using rowset = rowset<policy_type>;

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

		template<class P> struct bind_type {
			value_type type;
			int idx;
			bind_type():type(value_undef),idx(0) {}
		};

		template<class P> class rowset {
			public:
				using policy_type = P;
				using string = typename policy_type::string;
				using statement = statement<policy_type>;
				using bind_type = bind_type<policy_type>;
				using cell_t = cell_t<policy_type>;
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
		};

		template<class P, typename T> struct field {};

		template<class P> struct field<P,std::string> {
			static std::string as(const rowset<P>& r, const cell_t<P>& cell) {
				auto ptr = reinterpret_cast<const char*>(sqlite3_column_text(r.st, cell.bind_.idx));
				return std::string(ptr,strlen(ptr));
			}
		};

		template<class P> struct field<P,int> {
			static int as(const rowset<P>& r, const cell_t<P>& cell) {
				return sqlite3_column_int(r.st, cell.bind_.idx);
			}
		};

		template<class P> struct field<P,date_t> {
			static date_t as(const rowset<P>& r, const cell_t<P>& cell) {
				auto ptr = reinterpret_cast<const char*>(sqlite3_column_text(r.st, cell.bind_.idx));
				auto d = std::string(ptr,strlen(ptr));
				return cppstddb::impl::date_parse(d);
			}
		};

	}


	using database = cppstddb::front::basic_database<impl::database<default_policy>>;

	inline auto create_database() {
		return database();
	}


}}

#endif


