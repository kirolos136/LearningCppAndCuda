#include<cstddef>
#include<chrono>
#include<iostream>

using Clock = std::chrono::steady_clock;

template<typename T>
class MyVector
{
private:
    T* data;
    size_t m_size;
    size_t m_capacity;
public:
    MyVector();
    void push_back(const T& element);

    //accessing it as an array
    T& operator[](size_t index){
        return data[index];
    }

    // for read-only access on const objects: const MyVector<int> b; b[0]
    const T& operator[](size_t index) const
    {
        return data[index];
    }

    //Copy Constructor (deep copy)
    MyVector(const MyVector& other):m_size(other.m_size),m_capacity(other.m_capacity),data(new T[other.m_capacity]){
        //acquire other
        for (size_t i = 0; i < m_size; i++)
            data[i] = other.data[i];
    }

    void swap(MyVector& other) noexcept{
        std::swap(data,other.data);
        std::swap(m_size,other.m_size);
        std::swap(m_capacity,other.m_capacity);
    }

    MyVector& operator=(const MyVector& other){
        MyVector temp(other);
        swap(temp);
        return *this;
    }

    MyVector(MyVector&& other)noexcept 
        :data(other.data),m_capacity(other.m_capacity),m_size(other.m_size)
    {        
        other.data = nullptr;
        other.m_size = 0;
        other.m_capacity = 0;
    }

    MyVector& operator=(MyVector&& other) noexcept{
        if(this != &other){
            delete[] data;

            this->data = other.data;
            this->m_capacity = other.m_capacity;
            this->m_size = other.m_size;

            other.data = nullptr;
            other.m_size = 0;
            other.m_capacity = 0;
        }

        return *this;
    }

    ~MyVector();
};

//start with 100 as initial capacity
template<typename T>
MyVector<T>::MyVector():m_size(0),data(new T[100]),m_capacity(100)
{
}

template<typename T>
void MyVector<T>::push_back(const T& element){
    //check size if less than capacity just add the element and inc the size
    if(this->m_size < this->m_capacity){
        data[m_size] = element;
        m_size++;
    }else{
        //reallocating
        T* newData = new T[2*this->m_capacity];

        for (size_t i = 0; i < m_size; i++)
            newData[i] = data[i];
        
        
        delete[] data;
        data = newData;

        m_capacity *=2;
        data[m_size] = element;
        m_size++;
    }
}

template<typename T>
MyVector<T>::~MyVector()
{
    delete[] data;
}

int main(){
    // a small profiling test 10000 elements to see timming copy vs move

    MyVector<int> a;
    for(int i=0 ;i<10000;i++){
        a.push_back(i+2);
    }


    auto start = Clock::now();
    MyVector<int> b(a);
    auto end = Clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Time elapsed for Copying: " << duration.count() << " microseconds\n" << std::endl;

    start = Clock::now();
    MyVector<int> c = std::move(a);
    end = Clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Time elapsed for Moving: " << duration.count() << " microseconds\n" << std::endl;
}