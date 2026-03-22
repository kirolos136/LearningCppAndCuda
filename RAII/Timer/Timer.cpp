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

MyTimer::MyTimer(std::function<void(duration)> FinishAction)
{
    this->start = Clock::now();
    this->cancelled = false;
    this->FinishAction = FinishAction;
}

MyTimer::~MyTimer() noexcept
{
    if(!cancelled){
        if(FinishAction){
            try
            {
                this->end = Clock::now();
                duration elapsed = this->end - this->start;
                FinishAction(elapsed);
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
            }
        }
    }
}

void MyTimer::elapsed(){
    if(!this->cancelled){
        this->end = Clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(this->end - this->start).count();
        std::cout<<"Elapsed Time: "<< elapsed <<"ns\n";
    }
}

void MyTimer::cancel(){
    this->cancelled = true;
}

void seeTimeStack(){
    MyTimer time([](duration d){
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
        std::cout << "Timer Finished: " << ns << "ns\n";
    });

    for(int i=0; i<1000000000;i++);
    time.elapsed();
    for(int i=0; i<1000000000;i++);
    time.elapsed();
}

void seeTimeStackCancelled(){
    MyTimer time([](duration d){
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
        std::cout << "Timer Finished: " << ns << "ns\n";
    });

    for(int i=0; i<1000000000;i++);
    time.cancel();
}

int main(){
    std::cout << "=== Normal Timer ===\n";
    seeTimeStack();

    std::cout << "\n=== Cancelled Timer ===\n";
    seeTimeStackCancelled();
}