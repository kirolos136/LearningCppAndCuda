// ===Problem=====
// why do we need a shared ptr?
// till now unique pointer only control a resource if we tried to copy a resource it will give compile (copy constructor from RAII) 
// this is the main problem that lead to shared ptr where a resource can be controlled by many pointers
// unique_ptr — only one owner allowed
// MyUniquePtr<int> a = new int(42);
// MyUniquePtr<int> b = a;   // compile error — deleted copy

// // shared_ptr — multiple owners allowed
// std::shared_ptr<int> a = std::make_shared<int>(42);
// std::shared_ptr<int> b = a;  

// but if there many pointers controlling the resource when it will be deleted ===solution===> last one will delete it
// so we need a counter ==> here comes count ref theory
// if we put the counter in the pointer class itself => every object will have a different counter and it will be difficult for synchronization
// solution ==> Control block for every resource where it contain 2 counters (strong counter for owners and weak counter for observation)

// ```
// shared_ptr A  ──┐
//                 ├──► [ Control Block ]  ◄──── one heap allocation
// shared_ptr B  ──┘    ┌─────────────┐
//                      │  T* ptr     │  ──► [ Your actual object ]
//                      │  strong: 2  │
//                      │  weak:   0  │
//                      └─────────────┘
// ```

// we will need atmoics for counters as mutex is expensive
// copy => more owners => increment counter | move => one leaves and one take => nothing happened to counter | Destructor => leaving resouce =>decrement counter

// strong: 0 → object gone, but block stays if weak > 0
// weak:   0 → now the block itself is freed (if strong is 0 and weak has value this tells the observers that the object isn't alive and it is dead)

// Two separate allocations
//shared_ptr<int> a(new int(42));
//  allocation 1 → the int
//  allocation 2 → the control block (we will do this)

// One single allocation
//shared_ptr<int> a = make_shared<int>(42);
//  one allocation → [ control block | int ] packed together
#include<atomic>

struct ControlBlock {
    std::atomic<int> strong_count;
    std::atomic<int>  weak_count;
};

template<typename T>
class MySharedPtr
{
private:
    T* ptr;
    ControlBlock* cb;
public:
    // construction
    MySharedPtr(); //empty null 
    MySharedPtr(T* p);

    // copy
    MySharedPtr(const MySharedPtr& other);
    MySharedPtr& operator=(const MySharedPtr& other);

    // move
    MySharedPtr(MySharedPtr&& other);
    MySharedPtr& operator=(MySharedPtr&& other);

    //observers
    T* get();
    int use_count();
    explicit operator bool() const{
        return ptr != nullptr;
    }


    //access
    T& operator*(){
        return *ptr;
    }

    T* operator->(){
        return ptr;
    }

    ~MySharedPtr();
};

template<typename T>
MySharedPtr<T>::MySharedPtr():ptr(nullptr) , cb(nullptr)
{
}

template<typename T>
MySharedPtr<T>::MySharedPtr(T* p):ptr(p)
{
    cb = new ControlBlock{1,0};
}

template<typename T>
MySharedPtr<T>::MySharedPtr(const MySharedPtr& other):ptr(other.ptr),cb(other.cp){
    cb->strong_count++;
}


template<typename T>
MySharedPtr<T>& MySharedPtr<T>::operator=(const MySharedPtr& other) {
    if (this != &other) {
        if (cb) {
            cb->strong_count--;
            if (cb->strong_count == 0) delete ptr;
            if (cb->strong_count == 0 && cb->weak_count == 0) delete cb;
        }
        // take new resource
        ptr = other.ptr;
        cb  = other.cb;
        if (cb) cb->strong_count++;
    }
    return *this;
}

template<typename T>
MySharedPtr<T>::MySharedPtr(MySharedPtr&& other): ptr(other.ptr) , cb(other.cb){
    other.ptr = nullptr;
    other.cb = nullptr;
}

template <typename T>
MySharedPtr<T>& MySharedPtr<T>::operator=(MySharedPtr<T>&& other) {
    if (this != &other) {
        if (cb) {
            cb->strong_count--;
            if (cb->strong_count == 0) delete ptr;
            if (cb->strong_count == 0 && cb->weak_count == 0) delete cb;
        }

        ptr = other.ptr;
        cb  = other.cb;

        other.ptr = nullptr;
        other.cb  = nullptr;
    }
    return *this;
}


//Observers 
template <typename T>
T* MySharedPtr<T>::get(){
    return ptr;
}

template <typename T>
int MySharedPtr<T>::use_count(){
    return cb ? cb->strong_count : 0 ;
}

template<typename T>
MySharedPtr<T>::~MySharedPtr()
{
    if(!cb) return;
    cb->strong_count--;
    if(!cb->strong_count) delete ptr;
    if(!cb->strong_count && !cb->weak_count) delete cb;
}
