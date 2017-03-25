#ifndef CPPSTDDB_FRONT_H
#define CPPSTDDB_FRONT_H
#include <string>
#include <experimental/string_view>
#include <memory>
#include <exception>
#include "log.h"
#include "database_error.h"
#include <iostream>
#include <cppstddb/util.h>
#include <cppstddb/date.h>


namespace cppstddb { namespace front {

    enum value_type {
        value_undef,
        value_int,
        value_string,
        value_date,
        value_variant,
    };

    class default_policy {
    };


    template<class D, class P> class connection;
    template<class D, class P> class statement;
    template<class D, class P> class rowset;
    template<class D, class P> class rowset_iterator;
    template<class D, class P> class row;
    template<class D, class P> class field;


    template<typename T>
        void raise_error(const std::string& msg, const T& t) {
            std::stringstream s;
            s << msg << ": " << t;
            throw database_error(s.str());
        }


    template<class D, class P> class basic_database {
        public:
            using driver_type = D;
            using policy_type = P;
            using string = std::string;
            using string_view = std::experimental::string_view;
            using database_type = typename driver_type::database;
            using connection_t = connection<driver_type,policy_type>;
            using rowset_t = rowset<driver_type,policy_type>;

            struct data_t {
                database_type db;
                string uri;
                data_t() {}
                data_t(const string& uri_):uri(uri_) {}
            };

            //private:
            using shared_ptr_type = std::shared_ptr<data_t>;
            shared_ptr_type data_;

        public:
            basic_database():
                data_(std::make_shared<data_t>()) {
                }

            basic_database(const string& uri):
                data_(std::make_shared<data_t>(uri)) {
                }

            auto uri() const {return data_->uri;}

            auto connection() {return connection_t(*this,false);}
            auto connection(const string& uri) {return connection_t(*this,uri,false);}
            auto create_connection() {return connection_t(*this,true);}
            auto statement(const string &sql) {return connection().statement(sql);}

            auto query(const string& sql) {
                return statement(sql).query();
            }
    };

    template<class D, class P> class connection {
        public:
            using driver_type = D;
            using policy_type = P;
            using string = std::string;
            using database_t = basic_database<driver_type,policy_type>;
            using statement_t = statement<driver_type,policy_type>;
            using connection_type = typename driver_type::connection;

            //private:
            using shared_ptr_type = std::shared_ptr<connection_type>;
            database_t database_;
            shared_ptr_type data_; // data_ -> ptr?

        public:
            connection(database_t& database, bool create):
                database_(database),
                data_(std::make_shared<connection_type>(database_.data_->db, get_source(database_))) {
                }

            connection(database_t& database, const string& uri, bool create):
                database_(database),
                data_(std::make_shared<connection_type>(database_.data_->db, get_source(database_, uri))) {
                }

            auto statement(const string &sql) {return statement_t(*this,sql);}
            auto database() {return database_;}

            auto query(const string& sql) {
                return statement(sql).query();
            }

        private:

            static source get_source(const database_t& db) {
                return uri_to_source(db.uri());
            }

            static source get_source(const database_t& db, const string& uri) {
                return uri_to_source(uri.empty() ? db.uri() : uri);
            }

    };

    template<class D, class P> class statement {
        public:
            using driver_type = D;
            using policy_type = P;
            using string = std::string;
            using connection_t = connection<driver_type,policy_type>;
            using rowset_t = rowset<driver_type,policy_type>;
            using statement_type = typename driver_type::statement;

            //private:

            enum state_type {
                state_undef,
                state_prepared,
                state_executed
            };

            typedef std::shared_ptr<statement_type> shared_ptr_type;
            connection_t connection_;
            string sql_;
            shared_ptr_type data_;
            state_type state_;

        public:
            statement(connection_t& connection, const string &sql):
                connection_(connection),
                sql_(sql),
                data_(std::make_shared<statement_type>(*connection.data_, sql_)),
                state_(state_undef) {
                    prepare();
                }

            auto connection() {return connection_;}
            auto database() {return connection_.database();}

            void prepare() {
                data_->prepare();
                state_ = state_prepared;
            }

            auto query() {
                data_->query();
                state_ = state_executed;
                return *this;
            }

            template<typename... Args> statement& query(Args... args) {
                //info("HERE: ",args...);
                return *this;
            }


            auto rows() {return rowset_t(*this,1);}
    };

    template<class D, class P> class rowset {
        public:
            using driver_type = D;
            using policy_type = P;
            using string = std::string;
            using statement_t = statement<driver_type,policy_type>;
            //using rowset_t = rowset<driver_type,policy_type>;
            using row_t = row<driver_type,policy_type>;
            using rowset_type = typename driver_type::rowset;
            //alias Row = BasicRow!(Driver,Policy);
            //alias ColumnSet = BasicColumnSet!(Driver,Policy);
            using shared_ptr = std::shared_ptr<rowset_type>;
            using iterator = rowset_iterator<driver_type,policy_type>;

            //private:
            using shared_ptr_type = std::shared_ptr<rowset_type>;
            statement_t statement_;
            int row_array_size_;
            shared_ptr_type data_;
            int rows_fetched_;
            int row_idx_;

        public:
            rowset(statement_t& statement, int row_array_size):
                statement_(statement),
                row_array_size_(row_array_size),
                row_idx_(0),
                data_(std::make_shared<rowset_type>(*statement_.data_, row_array_size_)) {
                    //if (!stmt_.hasRows) throw new DatabaseException("not a result query");
                    rows_fetched_ = data_->fetch();
                }

            int width() {return data_->columns;}

            // length will be for the number of rows (if defined)
            int length() {
                //throw new Exception("not a completed/detached rowSet");
                return 0;
            }

            bool next() {
                DB_TRACE("next: " << row_idx_ << ":" << rows_fetched_);
                if (++row_idx_ == rows_fetched_) {
                    rows_fetched_ = data_->next();
                    if (!rows_fetched_) return false;
                    row_idx_ = 0;
                }
                return true;
            }

            /*
               auto into(A...) (ref A args) {
               if (!result_.rowsFetched()) throw new DatabaseException("no data");

               auto row = front();
               foreach(i, ref a; args) {
               alias T = A[i];
               static if (is(T == string)) {
               a = row[i].as!T.dup;
               } else {
               a = row[i].as!T;
               }
               }

            // don't consume so we can do into at row level
            // range.popFront;
            //if (!range.empty) throw new DatabaseException("result has more than one row");

            return this;
            }

            // into for output range: experimental
            //if (isOutputRange!(R,E))
            auto into(R) (R range) 
            if (hasMember!(R, "put")) {
            // Row should have range
            // needs lots of work
            auto row = Row(this);
            for(int i=0; i != width; i++) {
            auto f = row[i];
            switch (f.type) {
            case ValueType.Int: put(range, f.as!int); break;
            case ValueType.String: put(range, f.as!string); break;
            //case ValueType.Date: put(range, f.as!Date); break;
            default: throw new DatabaseException("switch error");
            }
            }
            }

            auto columns() {
            return ColumnSet(result_);
            }

             */

            //bool empty1() const {return !rows_fetched_;} // what is wrong here?
            bool empty() const {return rows_fetched_ == 0;}
            auto front() {return row_t(*this);}
            void pop_front() {next();}

            iterator begin() {return iterator(this);}
            iterator end() {return iterator(nullptr);}

            template<class OS> void write(OS &os) {
                os << "+--" << "\n";
                for(auto row : *this) {
                    for(auto c = 0; c != row.width(); c++) {
                        if (c) os << ",";
                        os << row[c];
                    }
                    os << "\n";
                }
                os << "+--" << "\n";
            }
    };


    template<class D, class P> class rowset_iterator {
        public:
            using driver_type = D;
            using policy_type = P;
            using rowset_t = rowset<driver_type,policy_type>;
            using row_t = row<driver_type,policy_type>;

            typedef std::ptrdiff_t difference_type;
            typedef rowset_t value_type;
            typedef rowset_t reference;
            typedef rowset_t* pointer;
            typedef std::input_iterator_tag iterator_category;

        private:
            rowset_t* rowset_;
        public:
            rowset_iterator(rowset_t* rowset):rowset_(rowset) {}
            auto operator*() {return row_t(*rowset_);}
            rowset_iterator& operator ++() {
                rowset_->next();
                return *this;
            }
            bool operator==(const rowset_iterator& rhs) const {
                return
                    (rowset_ && !rowset_->empty()) ==
                    (rhs.rowset_ && !rhs.rowset_->empty());
            }
            bool operator!=(const rowset_iterator& rhs) const {return !operator==(rhs);}
    };

    template<class D, class P> struct cell {
        public:
            using driver_type = D;
            using policy_type = P;
            using string = std::string;
            using rowset_t = rowset<driver_type,policy_type>;
            using row_t = row<driver_type,policy_type>;
            using bind_type = typename driver_type::bind_type;

            bind_type& bind_;
            int row_idx_;
            size_t idx_;

            cell(row_t& r, bind_type& b, size_t idx):
                bind_(b),
                idx_(idx),
                row_idx_(r.rows_.row_idx_) {}

            //auto bind() {return bind_;}
            //auto rowIdx() {return rowIdx_;}
    };

    template<class D,class P> class row {
        public:
            using driver_type = D;
            using policy_type = P;
            using string = std::string;
            using rowset_t = rowset<driver_type,policy_type>;
            using cell_t = cell<driver_type,policy_type>;
            using field_t = field<driver_type,policy_type>;
            //alias Cell = BasicCell!(Driver,Policy);
            ////alias Value = BasicValue!(Driver,Policy);
            //alias Column = BasicColumn!(Driver,Policy);

            rowset_t rows_;

            row(rowset_t rows):rows_(rows) {
            }

            auto width() {return rows_.width();}

            /*

               auto into(A...) (ref A args) {
               rows_.into(args);
               return this;
               }

               auto opIndex(Column column) {return opIndex(column.idx);}

            // experimental
            auto opDispatch(string s)() {
            return opIndex(rows_.result_.index(s));
            }
             */

            field_t operator[](size_t idx) {
                // needs work
                // sending a ptr to cell instead of reference (dangerous)
                auto cell = cell_t(*this, rows_.data_->binds[idx], idx);
                return field_t(*this, cell);
            }
    };


    template<class D, class P> class field {
        public:
            using driver_type = D;
            using policy_type = P;
            using string = std::string;
            using row_t = row<driver_type,policy_type>;
            using cell_t = cell<driver_type,policy_type>;
            using bind_type = typename driver_type::bind_type;

            row_t& row_; // reference currently, but watch out
            cell_t cell_;
            //alias Converter = .Converter!(Driver,Policy);

            field(row_t& row, cell_t cell):row_(row),cell_(cell) {}

            auto& rowset() const {return *row_.rows_.data_;}

            auto type() const {return cell_.bind_.type;}

            template<class T> T as() const {
                T value;
                //r.template as<T>(cell_, value);
                rowset().as(cell_, value);
                return value;
            }

            auto str() const {return as<string>();}

            friend inline std::ostream& operator<<(std::ostream &os, const field& f) {
                //os << "hello"; // problem at -O3
                // improve

                switch(f.type()) {
                    case value_int: os << f.as<int>(); break;
                    case value_string: os << f.as<string>(); break;
                    case value_date: os << f.as<date>(); break;
                    default: raise_error("unsupported type", f.type());
                }
                //os << f.as<string>();
                return os;
            }

            /*
               template<class T=int> auto as<T>() {
               return rowset.as!T(cell_);
               }
             */

            /*
               auto as(T:int)() {return Converter.convert!T(resultPtr, cell_);}
               auto as(T:string)() {return Converter.convert!T(resultPtr, cell_);}
               auto as(T:Date)() {return Converter.convert!T(resultPtr, cell_);}
               auto as(T:Nullable!T)() {return Nullable!T(as!T);}

            // should be nothrow?
            auto as(T:Variant)() {return Converter.convert!T(resultPtr, cell_);}

            bool isNull() {return false;} //fix

            string name() {return resultPtr.name(cell_.idx_);}

            auto get(T)() {return Nullable!T(as!T);}

            // experimental
            auto option(T)() {return Option!T(as!T);}

            // not sure if this does anything
            const(char)[] chars() {return as!string;}

            string toString() {return as!string;}
             */


    };

}}

#endif

