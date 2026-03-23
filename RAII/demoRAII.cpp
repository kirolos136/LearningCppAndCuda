#include "File/FileHandling.hpp"
#include "Thread/threadHandling.hpp"
#include "Timer/Timer.hpp"

using Clock = std::chrono::steady_clock;
using duration = Clock::duration;

// Simulates processing a student grade entry
//   - Timer measures how long the whole operation takes
//   - Mutex protects the shared file from concurrent writes
//   - File handles reading and writing the grade log
void processGradeEntry(std::mutex& fileMutex, const std::string& entry) {

    // Timer starts here — measures entire operation duration
    MyTimer t([](duration d) {
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
        std::cout << "Operation took: " << ns << "ns\n";
    });

    // Lock acquired — no other thread can write to file simultaneously
    MutexLock mtx(fileMutex);

    // File opened for writing the grade entry
    File gradeLog("grades.txt");
    gradeLog.write(entry.c_str());

    std::cout << "Entry written: " << entry << "\n";

    // --- scope ends here ---
    // File destructor  → closes grades.txt automatically
    // MutexLock destructor → releases the lock automatically
    // Timer destructor → logs elapsed time automatically
    // Even if an exception was thrown above — all cleanup guaranteed
}

int main() {
    std::mutex fileMutex;

    // Normal operation
    processGradeEntry(fileMutex, "Alice - A+");
    processGradeEntry(fileMutex, "Bob   - B ");

    // Prove RAII works even when exception is thrown mid-operation
    try {
        MyTimer t([](duration d) {
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
            std::cout << "Exception operation took: " << ns << "ns\n";
        });
        MutexLock mtx(fileMutex);
        File gradeLog("grades.txt");

        throw std::runtime_error("something went wrong!");

        // even though we throw:
        // File    → still closes  
        // Mutex   → still unlocks  
        // Timer   → still logs     
    }
    catch (const std::exception& e) {
        std::cerr << "Caught exception: " << e.what() << "\n";
        std::cerr << "All resources were still cleaned up safely\n";
    }

    return 0;
}