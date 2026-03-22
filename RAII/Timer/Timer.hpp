#include<chrono>
#include<iostream>
#include<functional>

using Clock = std::chrono::steady_clock;
using time_point = Clock::time_point;
using duration = Clock::duration;

class MyTimer
{
private:
    time_point start;
    time_point end;
    bool cancelled;
    std::function<void(duration)> FinishAction;

public:
    MyTimer(std::function<void(duration)> FinishAction);
    ~MyTimer();
    void elapsed();
    MyTimer(const MyTimer&) = delete;
    MyTimer& operator=(const MyTimer&) = delete;
    void cancel();
};