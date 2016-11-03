#ifndef H_INFO
#define H_INFO

#include<iostream>

template<typename T1> 
void info(const T1& t1) {
    std::cout << t1 << std::endl;
}

template<typename T1, typename T2>
void info(const T1& t1, const T2& t2) {
    std::cout << t1 << t2 << std::endl;
}

template<typename T1, typename T2, typename T3>
void info(const T1& t1, const T2& t2, const T3& t3) {
    std::cout << t1 << t2 << t3 << std::endl;
}

#endif

