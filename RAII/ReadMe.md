# RAII Library

A minimal C++ library demonstrating the RAII pattern 
(Resource Acquisition Is Initialization) across three 
common resource types: files, mutexes, and timers.

## Core Guarantee
All resources are guaranteed to be released automatically 
when they go out of scope — even if an exception is thrown.

---

## How to Build
```bash
make
```

## How to Run
```bash
# Run demo
./build/demoRAII

# Run with AddressSanitizer (memory check)
make
./build/demoRAII

# Run with Valgrind (leak check)
make valgrind
valgrind --leak-check=full ./build/demoRAII
```

---

## Classes

### `File`
Wraps a file handle using RAII.

| | |
|---|---|
| Constructor | Opens the file, throws if it does not exist |
| Destructor | Closes the file automatically |
| `read()` | Reads content from the file |
| `write()` | Writes content to the file |
| Guarantee | File is always closed — no dangling handles |

**Known Limitations:**
- Not copyable — copying a file handle causes double-close
- Not thread-safe on its own — use with MutexLock

---

### `MutexLock`
Wraps a `std::mutex` using RAII.

| | |
|---|---|
| Constructor | Takes a reference to an existing mutex and locks it |
| Destructor | Unlocks automatically |
| `cancel()` | Suppresses unlock if early exit is needed |
| Guarantee | Mutex is always unlocked — no deadlocks from exceptions |

**Known Limitations:**
- Not copyable — two owners unlocking the same mutex causes undefined behavior
- Does not support recursive locking — locking twice on the same thread deadlocks. Use `std::recursive_mutex` if recursion is needed

---

### `MyTimer`
Measures elapsed time using RAII.

| | |
|---|---|
| Constructor | Records start time, accepts an optional callback |
| Destructor | Calls callback with elapsed duration automatically |
| `elapsed()` | Prints elapsed time at any point mid-run |
| `cancel()` | Suppresses the callback on destruction |
| Guarantee | Callback always called on scope exit unless cancelled |

**Known Limitations:**
- Not copyable — a copied timer carries the original start time, making elapsed time meaningless
- Not safe to use inside signal handlers — uses `std::chrono` and `std::function` which are not async-signal-safe

---

## Example
```cpp
std::mutex fileMutex;

MyTimer t([](duration d) {
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
    std::cout << "Done in: " << ns << "ns\n";
});

MutexLock mtx(fileMutex);  // locked
File f("log.txt");          // opened
f.write("entry");

// scope ends — file closed, mutex unlocked, timer logs ✅
```

---

## Stress Test

Creates and destroys all three classes in 10,000 iterations.
Verified with:
- AddressSanitizer (`-fsanitize=address,undefined`) → zero errors
- Valgrind (`--leak-check=full`) → zero leaks