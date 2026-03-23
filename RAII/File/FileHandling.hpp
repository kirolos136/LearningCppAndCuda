#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

class File
{
private:
    FILE* fp = nullptr;
    
public:
    File(const char* fileName);

    //For equal operator or File(another File)
    //Prevent Copying
    // I know you could auto-generate this — don't. 
    // And if anyone tries to use it, throw an error in their face at compile time.
    File(const File&) = delete;
    File& operator=(const File&) = delete;

    //For std::move
    File(File&& other) noexcept : fp(other.fp)  // 1. take other's pointer
    { 
        other.fp = nullptr;           // 2. null out the source
    }
    File& operator=(File&& other) noexcept;
    ~File();
    void write(const char *text);
    std::string read();

    FILE* get() const { return fp; }
};
