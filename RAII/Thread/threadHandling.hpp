#include<mutex>
#include<iostream>
#include<exception>

class MutexLock
{
private:
    std::mutex& mtx;
    bool locked;
public:
    MutexLock(std::mutex& m);
    ~MutexLock();

    // Rule 2 & 3 — copying a lock makes no sense, ban it
    MutexLock(const MutexLock&) = delete;
    MutexLock& operator=(const MutexLock&) = delete;

    // Rule 4 & 5 — moving ownership is fine
    MutexLock(MutexLock&& other);
    MutexLock& operator=(MutexLock&& other);
};