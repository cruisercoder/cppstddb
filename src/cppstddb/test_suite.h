#ifndef TEST_SUITE_H
#define TEST_SUITE_H

#include <ostream>
#include <stdexcept>
#include <numeric>

/*
   A really basic test framework & content to start with,
   keeping it light and without external frameworks for now
 */

namespace cppstddb {

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

    template<class database> void simple_test(const std::string& source) {
        test_header("simple_test");
        auto db = database();
        db
            .statement("select * from score")
            .query()
            .rows()
            .write(std::cout);
    }

    template<class database> void simple_classic_test(const std::string& source) {
        test_header("simple_classic_test");
        // classic style, nothing fancy
        //auto db = typename database::database();
        auto db = database();
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

    template<class database> void for_loop_1_test(const std::string& source) {
        test_header("for_loop_1_test");
        auto db = database();
        auto rowset = db.connection().statement("select * from score").query().rows();
        for(auto row : rowset) {
            auto f = row[0];
            //std::cout << "=== VALUE: " << f.template as<std::string>() << "\n";
            std::cout << "-- VALUE: " << f << "\n";
        }
    }

    template<class database> void for_loop_2_test(const std::string& source) {
        test_header("for_loop_2_test");
        auto db = database();
        auto rowset = db.connection().statement("select * from score").query().rows();
        for(auto&& row : rowset) {
            auto f = row[0];
            std::cout << "value: " << f << "\n";
        }
    }

    template<class database> void iterator_1_test(const std::string& source) {
        test_header("iterator_1_test");
        auto db = database();
        auto rowset = db.connection().statement("select * from score").query().rows();
        for(auto i = rowset.begin(); i != rowset.end(); ++i) {
            auto row = *i;
            auto f = row[0], g = row[1];
            //cout << "VALUE: " << f.as<string>() << endl;
            std::cout << "value: " << f << ": " << g << "\n";
        }
    }

    template<class database> void stl_find_if_test(const std::string& source) {
        test_header("stl_find_if_test");
        using namespace std;

        auto db = database();
        auto r = db.statement("select name,score from score").query().rows(); // fix auto exec

        auto key = "b";
        auto i = find_if(r.begin(), r.end(), [key](auto row) { 
                return row[0].str() == key; });

        assertion(i != r.end());
        auto row = *i;
        cout << "row: " << row[0] << ":" << row[1] << "\n";
    } 

    template<class database> void stl_accumulate_test(const std::string& source) {
        test_header("stl_accumulate_test");
        using namespace std;

        auto db = database();
        auto r = db.statement("select name,score from score").query().rows(); // fix auto exec

        auto sum = std::accumulate(r.begin(), r.end(), 0, [](int sum, auto row) { 
                return sum + row[1].template as<int>();});

        assertion(sum == 150);
    }

    template<class database> void test_all(const std::string& source) {
        simple_test<database>(source);
        simple_classic_test<database>(source);
        for_loop_1_test<database>(source);
        for_loop_2_test<database>(source);
        iterator_1_test<database>(source);
        stl_find_if_test<database>(source);
        stl_accumulate_test<database>(source);
    }

}

#endif

