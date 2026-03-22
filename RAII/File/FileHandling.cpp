#include "FileHandling.hpp"

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