#include "../File/FileHandling.hpp"
#include "../Thread/threadHandling.hpp"
#include "../Timer/Timer.hpp"

using Clock = std::chrono::steady_clock;
using time_point = Clock::time_point;
using duration = Clock::duration;

void printTimer(duration d){
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
    std::cout << "Time ended: " << ns << " ns\n";
}

void stressTest(){
    std::mutex m;
    for (size_t i = 0; i < 10000; i++)
    {
        File f("file.txt");
        MutexLock mtx(m);
        MyTimer t(printTimer);
    }  
}


// int main(){
//     stressTest();
// }

// to see debug flag put : -g
// then use one of the following:
//
// 1. valgrind
// first make then run valgrind --leak-check=full ./myProgram
//
// 2.using addressSanitizer:
// adding this into the flags also : -fsanitize=address,undefined
