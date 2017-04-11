#ifndef CPPSTDDB_TEST_SUITE_H
#define CPPSTDDB_TEST_SUITE_H

#include <cppstddb/sql_util.h>
#include <ostream>
#include <stdexcept>
#include <numeric>

/*
   A really basic test framework & content to start with,
   keeping it light and without external frameworks for now
 */

namespace cppstddb {

    inline auto test_uri(
            const std::string& protocol,
            const std::string& host = "localhost",
            const std::string& database = "cppstddb",
            const std::string& username = "test",
            const std::string& password = "test") {
        return get_uri(protocol,host,database,username,password);
    }

    inline void assertion(bool b) {
        if (!b) throw std::runtime_error("assertion failed");
    }

    inline void assertion(bool b, const char *msg) {
        if (!b) throw std::runtime_error(msg);
    }

    inline void test_header(const char *name) {
        using namespace std;
        cout << endl;
        cout << "============================================= " << endl; 
        cout << name << endl;
        cout << "============================================= " << endl; 
    }

    template<class database> void simple_test(const std::string& uri) {
        test_header("simple_test");
        auto db = database(uri);
        db
            .statement("select * from score")
            .query()
            .rows()
            .write(std::cout);
    }

    template<class database> void simple_classic_test(const std::string& uri) {
        test_header("simple_classic_test");
        // classic style, nothing fancy
        //auto db = typename database::database();
        auto db = database(uri);
        auto con = db.connection();
        auto stmt = con.statement("select * from score");
        stmt.query();
        stmt.query(1,2);
        auto rowset = stmt.rows();
        for(auto i = rowset.begin(); i != rowset.end(); ++i) {
            auto row = *i;
            for(auto c = 0; c != row.width(); ++c) {
                if (c) std::cout << ",";
                std::cout << row[c] << " ";
            }
            std::cout << "\n";
        }
    }

    template<class database> void simple_date_test(const std::string& uri) {
        test_header("simple_date_test");

        auto db = database(uri);
        auto rowset = db
            .statement("select * from score")
            .query()
            .rows();

        for(auto&& row : rowset) {
            auto name = row[0], score = row[1], d = row[2];
            std::cout
                << "name: " << name
                << ",score: " << score
                << ", date: " << d.template as<date_t>() << "\n";
        }
    }

    template<class database> void for_loop_1_test(const std::string& uri) {
        test_header("for_loop_1_test");
        auto db = database(uri);
        auto rowset = db.connection().statement("select * from score").query().rows();
        for(auto row : rowset) {
            auto f = row[0];
            //std::cout << "=== VALUE: " << f.template as<std::string>() << "\n";
            std::cout << "-- VALUE: " << f << "\n";
        }
    }

    template<class database> void for_loop_2_test(const std::string& uri) {
        test_header("for_loop_2_test");
        auto db = database(uri);
        auto rowset = db.connection().statement("select * from score").query().rows();
        for(auto&& row : rowset) {
            auto f = row[0];
            std::cout << "value: " << f << "\n";
        }
    }

    template<class database> void iterator_1_test(const std::string& uri) {
        test_header("iterator_1_test");
        auto db = database(uri);
        auto rowset = db.connection().statement("select * from score").query().rows();
        for(auto i = rowset.begin(); i != rowset.end(); ++i) {
            auto row = *i;
            auto f = row[0], g = row[1];
            //cout << "VALUE: " << f.as<string>() << endl;
            std::cout << "value: " << f << ": " << g << "\n";
        }
    }

    template<class database> void stl_find_if_test(const std::string& uri) {
        test_header("stl_find_if_test");
        using namespace std;

        auto db = database(uri);
        auto r = db.statement("select name,score from score").query().rows(); // fix auto exec

        auto key = "Dijkstra";
        auto i = find_if(r.begin(), r.end(), [key](auto row) { 
                return row[0].str() == key; });

        assertion(i != r.end());
        auto row = *i;
        cout << "row: " << row[0] << ":" << row[1] << "\n";
    } 

    template<class database> void stl_accumulate_test(const std::string& uri) {
        test_header("stl_accumulate_test");
        using namespace std;

        auto db = database(uri);
        auto r = db.statement("select name,score from score").query().rows(); // fix auto exec

        auto sum = std::accumulate(r.begin(), r.end(), 0, [](int sum, auto row) { 
                return sum + row[1].template as<int>();});

        assertion(sum == 194);
    }

    template<class database> void test_all(const std::string& uri) {
        {
            auto db = database(uri);
            recreate_score_table(db);
        }

        simple_test<database>(uri);
        simple_classic_test<database>(uri);
        simple_date_test<database>(uri);
        for_loop_1_test<database>(uri);
        for_loop_2_test<database>(uri);
        iterator_1_test<database>(uri);
        stl_find_if_test<database>(uri);
        stl_accumulate_test<database>(uri);
    }


    template<class database> void recreate_score_table(database& db, bool data = true) { //recreate
        auto con = db.connection();
        std::string table = "score";
        const static int sz = 3;
        const char *names[sz] = {"Knuth", "Hopper", "Dijkstra"};
        const int scores[sz] = {62, 48, 84};
        const char *dates[sz] = {"2016-01-01", "2016-02-02", "2016-03-03"};

        std::stringstream query;
        query
            << "create table score (name varchar(10), score integer"
            << ",d " << db.date_column_type()
            << ")";

        drop_table(db, table);
        con.query(query.str());

        if (!data) return;
        for(int i = 0; i != sz; ++i) {
            std::stringstream ss;
            ss
                << "insert into score values("
                << "'" << names[i] << "'"  << ","
                << scores[i]
                << ",'" << dates[i] << "'"
                << ")";
            DB_TRACE(ss.str());
            con.query(ss.str());
        }
    }


}

#endif

