#ifndef CPPSTDDB_DATABASE_POSTGRES_H
#define CPPSTDDB_DATABASE_POSTGRES_H

#include <cppstddb/front.h>
#include <cppstddb/util.h>
#include <cppstddb/endian.h>
#include <vector>
#include <libpq-fe.h>
#include <pgtypes_date.h>
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

	namespace impl {

		template<class P> class database;
		template<class P> class connection;
		template<class P> class statement;
		template<class P> class rowset;
		template<class P> class bind_type;
		template<class P,class T> class field;

		template<class P> using cell_t = cppstddb::front::cell<database<P>>;


		template<class S> void raise_error(const S& msg) {
			throw database_error(msg);
		}

		template<class S> void raise_error(PGconn *con, const S& msg) {
			std::string s; // revisit type
			s += msg;
			s += ", ";
			s += PQerrorMessage(con);
			DB_ERROR("raise_error: " << s);
			throw database_error(s);
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
					DB_TRACE("db");
					DB_TRACE("PQlib version: " << PQlibVersion());
				}
				~database() {
					DB_TRACE("~db");
				}

				string date_column_type() const {return "date";}
		};

		template<class P> class connection {
			public:
				using policy_type = P;
				using string = typename policy_type::string;
				using database = database<policy_type>;


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

		template<class P> class statement {
			public:
				using policy_type = P;
				using string = typename policy_type::string;
				using connection = connection<policy_type>;
				using rowset = rowset<policy_type>;

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
					int resultFormat = 1; // results in binary format

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

		template<class P> struct describe_type {
			using policy_type = P;
			using string = typename policy_type::string;
			int dbType;
			int format;
			string name;
		};

		template<class P> struct bind_type {
			value_type type;
			int idx;
			//int format;
			//int len;
			//int is_null; 
		};

		template<class P> class rowset {
			public:
				using policy_type = P;
				using string = typename policy_type::string;
				using statement = statement<policy_type>;
				using bind_type = bind_type<policy_type>;
				using cell_t = cell_t<policy_type>;

				statement& stmt;
				PGconn *con;
				PGresult *res;
				unsigned int columns;
				int status;
				int row;
				int rows;
				bool hasResult_;
			public:
				using describe_type = describe_type<policy_type>;
				using describe_vector = std::vector<describe_type>;
				using bind_vector = std::vector<bind_type>;

				describe_vector describes;
				bind_vector binds;

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
						d.format = PQfformat(res, col);
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
						DB_TRACE("dbType: " << d.dbType << ", type: " << b.type);
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

				const void* data(int col) const {return PQgetvalue(res, row, col);}
				bool is_null(int col) const {return PQgetisnull(res, row, col) != 0;}
				int type(int col) const {return describes[col].dbType;}
				int format(int col) const {return describes[col].format;}
				int len(int col) const {return PQgetlength(res, row, col);}
		};

		inline void check_type(int a, int b) {
			if (a != b) throw database_error("type mismatch");
		}

		template<class P, typename T> struct field {};

		template<class P> struct field<P,std::string> {
			static std::string as(const rowset<P>& r, const cell_t<P>& cell) {
				return static_cast<const char *>(r.data(cell.bind_.idx));
			}
		};

		template<class P> struct field<P,int> {
			static int as(const rowset<P>& r, const cell_t<P>& cell) {
				return big4_to_native(r.data(cell.bind_.idx));
			}
		};

		template<class P> struct field<P,date_t> {
			static date_t as(const rowset<P>& r, const cell_t<P>& cell) {
				check_type(r.type(cell.bind_.idx),DATEOID);

				// need to clarify the right endian approach
				//date d = big4_to_native(r.data(cell.bind_.idx));

				// this works
				auto a = static_cast<const unsigned char *>(r.data(cell.bind_.idx));
				date d = (a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3];

				int mdy[3];
				PGTYPESdate_julmdy(d, &mdy[0]);
				return date_t(mdy[2],mdy[0],mdy[1]);
			}
		};

	}

	using database = cppstddb::front::basic_database<impl::database<default_policy>>;

	inline auto create_database() {
		return database();
	}


}}

#endif


