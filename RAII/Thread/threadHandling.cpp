#include "threadHandling.hpp"

MutexLock::MutexLock(std::mutex& m): mtx(m)
{
    mtx.lock();
    // mtx.lock();
    locked = true;
}

//QUESTION : What does MutexLock do if the same thread locks twice? 
//SOLUTION:
// Think of a mutex like a **bathroom with one key**:
// Thread wants the key → takes it 
// Thread wants the key AGAIN → 
//     looks for the key →
//     key is in its own pocket →
//     waits for someone to return the key →
//     but IT is the one holding it →
//     waits forever => DEADLOCK

//FIX: Counting AND Recursive mutex

MutexLock::~MutexLock()
{
    if(locked){
        mtx.unlock();
        locked = false;
        std::cout<<"unlocking mutex\n";
    }
}

//move constructor
MutexLock::MutexLock(MutexLock&& other):mtx(other.mtx),locked(other.locked)
{
    other.locked = false;
}

MutexLock& MutexLock::operator=(MutexLock&& other)
{
    if(this != &other){
        if(locked) mtx.unlock();
        locked = other.locked;
        other.locked = false;
    }
    return *this;
}

/*
this class is made as to avoid the following example for deadlock
*/
void noUnlocking(std::mutex& m){
    try
    {
        m.lock();
        std::cout<<"locking\n";
        throw std::runtime_error("leaving a function\n");
        m.unlock();
        std::cout<<"unlocking\n";
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

//mutexLock (mutex RAII solving the issue)
void unlockingSolved(std::mutex& m){
    try
    {
        MutexLock mutex(m);
        std::cout<<"locking\n";
        throw std::runtime_error("leaving a function\n");
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void moveProblem(std::mutex& m){
    try
    {
        MutexLock mutex(m);
        //to see move problem remove the next line and remove rule 2 and 3
        //MutexLock mutex2 = mutex;
        // calls auto-generated copy constructor
        //The compiler-generated copy does this:
        //lock2.mtx = mutex.mtx   
        // both now reference the same mutex
        std::cout<<"locking\n";
        throw std::runtime_error("leaving a function\n");
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}


// int main(){
//     std::mutex m;
//     unlockingSolved(m);
//     // noUnlocking(m);

//     //move Problem
//     moveProblem(m);
// }
