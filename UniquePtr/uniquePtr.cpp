#include <iostream>
#include <memory>
// ==========================================================
// My Unique Ptr Class
// ==========================================================
// Adding custom deleter to solve hardcoded destructor problem
template <typename T,typename Deleter = std::default_delete<T>> 
class MyUniquePtr
{
private:
    T* ptr;
    Deleter d;
public:
    MyUniquePtr();
    MyUniquePtr(T* p, Deleter del);
    MyUniquePtr(const MyUniquePtr&) = delete;
    MyUniquePtr& operator=(const MyUniquePtr&) = delete;
    MyUniquePtr(MyUniquePtr&& other);
    MyUniquePtr& operator=(MyUniquePtr&& other);
    explicit operator bool() const; //as if you write the name of object in if condition what to do ex: if(ptr) 
    T* get();
    T* release();
    void reset(T* p = nullptr);
    T& operator*();
    T* operator->();
    ~MyUniquePtr();
};

template <typename T,typename Deleter> 
MyUniquePtr<T,Deleter>::MyUniquePtr():ptr(nullptr){}

template<typename T,typename Deleter>
MyUniquePtr<T,Deleter>::MyUniquePtr(T* p,Deleter del) : ptr(p),d(del){}

//there is a problem with the following destructor that it is hardcoded
// so it only works if data is like the following : int* p = new int(42)
// but if it changed to array int* p = new int[100] it will lead to undefined behaviour
// as it needs to be delete[] ptr; and if it was a file pointer it will need fclose
// solution : CUSTOM DELETER
template <typename T,typename Deleter>
MyUniquePtr<T,Deleter>::~MyUniquePtr()
{
    if(ptr){
        d(ptr);
        std::cout<<"memory is freed\n";
    }
}

template <typename T,typename Deleter>
MyUniquePtr<T,Deleter>::MyUniquePtr(MyUniquePtr&& other): ptr(other.ptr) 
{
    other.ptr = nullptr;
}

template <typename T,typename Deleter>
MyUniquePtr<T,Deleter>& MyUniquePtr<T,Deleter>::operator=(MyUniquePtr<T,Deleter>&& other){
    if(this != &other){
        if(ptr) d(ptr);
        ptr = other.ptr;
        other.ptr = nullptr;
    }
    return *this;
}

template <typename T,typename Deleter>
T* MyUniquePtr<T,Deleter>::get(){
    return this->ptr;
}

template <typename T,typename Deleter>
T* MyUniquePtr<T,Deleter>::release(){
    T* savedptr = this->ptr;
    this->ptr = nullptr;
    return savedptr;
}

template <typename T,typename Deleter>
void MyUniquePtr<T,Deleter>::reset(T* p){
    if(ptr) d(this->ptr);
    this->ptr = p;
}

template <typename T,typename Deleter>
T& MyUniquePtr<T,Deleter>::operator*(){
    return *ptr;
}

template <typename T,typename Deleter>
T* MyUniquePtr<T,Deleter>::operator->(){
    return ptr;
}

template <typename T,typename Deleter>
MyUniquePtr<T,Deleter>::operator bool() const {
    return ptr != nullptr;
}
// ===================================================

// custom deleter make it based on what you want
template <typename T>
struct MyDeleter{
    void operator()(T *p){
        delete p;
    }
};


void checkDestructor(){
    MyUniquePtr<int,MyDeleter<int>> p(new int(32),MyDeleter<int>{});

    //at the end of the scope
    // the destructor should be called automatically 
    // freeing the memory
}

void checkCopyAndMoveSemantics(){
    MyUniquePtr<int,MyDeleter<int>> a(new int(2),MyDeleter<int>{}) ;
    // MyUniquePtr<int> b = a; copy is deleted
    MyUniquePtr<int,MyDeleter<int>> b = std::move(a);
    if(a.get()){
        std::cout<<"a has value\n";
    }else{
        std::cout<<"a is null\n";
    }

    //should print a is null due to move semantics
    //2 calls to destructor => 1 freed memory print as the another is already nullptr
}

//comparing To unique Ptr
class Demo {
public:
    Demo() { std::cout << "Demo Constructed\n"; }
    ~Demo() { std::cout << "Demo Destroyed\n"; }
    void hello() { std::cout << "Hello from Demo\n"; }
};

void compare(){
    //unique ptr
    std::unique_ptr<Demo> ptr1 = std::make_unique<Demo>();
    ptr1->hello();

    std::unique_ptr<Demo> ptr2 =std::move(ptr1);
    if(ptr1 == nullptr){
        std::cout<<"ptr1 is now null\n";
    }
    ptr2->hello();

    //My unique ptr
    MyUniquePtr<Demo,MyDeleter<Demo>> ptr3(new Demo(),MyDeleter<Demo>{});
    ptr3->hello();

    MyUniquePtr<Demo,MyDeleter<Demo>> ptr4 = std::move(ptr3);
    if(!ptr3){
        std::cout<<"ptr3 is now null\n";
    }

    ptr4->hello();
}

int main(){
    // checkDestructor();
    // checkCopyAndMoveSemantics();
    compare();
}