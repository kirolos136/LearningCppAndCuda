#include <cstdio>
#include <iostream>
#include <stdexcept>

class File
{
private:
    
    
public:
    FILE *fp;
    File(const char* fileName);

    //For equal operator or File(another File)
    //Prevent Copying
    // I know you could auto-generate this — don't. 
    // And if anyone tries to use it, throw an error in their face at compile time.
    // File(const File&) = delete;
    // File& operator=(const File&) = delete;

    //For std::move
    File(File&& other) : fp(other.fp)  // 1. take other's pointer
    { 
        other.fp = nullptr;             // 2. null out the source
    }
    ~File();
    void write(const char *text);
    std::string read();
};

File::File(const char* fileName)
{   
    fp = fopen(fileName, "w+");
    if (fp == nullptr) {
        throw std::runtime_error(std::string("Could not open file: ") + fileName);
    }
}

File::~File()
{
    fclose(fp);
}

void File::write(const char *text)
{
    if (fputs(text, fp) == EOF) {
        throw std::runtime_error("Write failed");
    }
}

std::string File::read()
{
    rewind(fp);  // go back to start before reading
    std::string result;
    char buf[256];
    while (fgets(buf, sizeof(buf), fp)) {
        result += buf;
    }
    return result;
}

//function To break RAII intentionally
//first making 2 fclose 1-> manually and one called inside destructor
//result:
// free(): double free detected in tcache 2
// Aborted (core dumped)
void writeHelloWorld(){
    File file("file.txt");
    file.write("hello world");
    std::cout << file.read() << std::endl;
    fclose(file.fp);
}

//second copying(shallow copy) point to  same file object (FILE struct) in memory
//result:
// free(): double free detected in tcache 2
// Aborted (core dumped)
void shallowCopy(){
    File file("file.txt");
    file.write("hello world");
    std::cout << file.read() << std::endl;
    File b = std::move(file);
}

int main(){
    // writeHelloWorld();
    shallowCopy();
}