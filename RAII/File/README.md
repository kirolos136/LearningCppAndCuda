# FileHandling — RAII with `FILE*`

A from-scratch RAII implementation wrapping C's raw `FILE*`. No tutorials — written, broken, and rewritten.

---

## What is RAII?

**Resource Acquisition Is Initialization.**

The idea: tie a resource's lifetime to an object's lifetime.

| Event | What happens |
|---|---|
| Object constructed | Resource acquired (file opened) |
| Object goes out of scope | Resource released (file closed) |
| Exception thrown | Destructor still runs — resource still released |

You never call `fclose` manually. The object does it for you, always, no matter how the scope exits.

---

## The Problem Without RAII

```cpp
void doWork() {
    FILE* fp = fopen("file.txt", "w+");
    
    doSomething();    // throws an exception
    
    fclose(fp);       // never reached — file handle leaked forever
}
```

With RAII the destructor runs even on exception. The file is always closed.

---

## The Rule of Five

If your class manually manages a resource (opens a file, allocates memory, holds a lock), the compiler-generated defaults for copying and moving will all do the wrong thing. You must take control of all five:

### Rule 1 — Destructor
Releases the resource. Called automatically when the object goes out of scope.

```cpp
File::~File() {
    if (fp) fclose(fp);   // guard for nullptr (needed after a move)
}
```

---

### Rules 2 & 3 — Copy Constructor + Copy Assignment (DELETED)

The compiler auto-generates a **shallow copy** — it copies the pointer byte-for-byte:

```
file.fp  ──►  [ FILE struct at 0x1234 ]
file2.fp ──►  [ FILE struct at 0x1234 ]   ← same object!
```

When scope ends both destructors fire:
```
fclose(0x1234)  ← file2 destructor  ✅
fclose(0x1234)  ← file  destructor  💥 double-free → crash
```

**Fix:** ban copying entirely at compile time.

```cpp
File(const File&)            = delete;   // copy constructor
File& operator=(const File&) = delete;   // copy assignment
```

`= delete` means: *"I know you could auto-generate this — don't. Throw a compile error if anyone tries."*

Instead of a silent runtime crash you get:
```
error: use of deleted function 'File::File(const File&)'
```

Compile error beats runtime crash every time.

---

### Rule 4 — Move Constructor

Copying a `File` makes no sense. But **transferring ownership** does.

```cpp
File b = std::move(a);   // b takes ownership, a gives it up
```

`std::move` doesn't move anything itself — it just tells the compiler *"treat `a` as a temporary you're allowed to steal from."*

```cpp
File(File&& other) : fp(other.fp)   // take other's pointer
{
    other.fp = nullptr;             // defuse other's destructor
}
```

Memory diagram:
```
BEFORE move:
a.fp ──► [ FILE 0x1234 ]

AFTER move:
a.fp ──► nullptr              ← destructor does nothing
b.fp ──► [ FILE 0x1234 ]      ← sole owner, will close it
```

One `fclose`, correct, no crash.

---

### Rule 5 — Move Assignment (exercise)

Same as move constructor but `this` already owns a file — close it first:

```cpp
File& operator=(File&& other) {
    if (this != &other) {
        if (fp) fclose(fp);   // release what we currently own
        fp = other.fp;        // take ownership
        other.fp = nullptr;   // defuse source
    }
    return *this;
}
```

---

## Break Demos

### Break 1 — Double fclose
```cpp
File file("file.txt");
fclose(file.fp);    // manual close
// destructor fires → fclose again → double-free → crash
```
Result: `free(): double free detected in tcache 2 — Aborted`

### Break 2 — Shallow copy (copy rules not deleted)
```cpp
File file("file.txt");
File b = file;      // shallow copy — both fp point to 0x1234
// both destructors fire → double-free → crash
```
With `= delete` this becomes a compile error instead.

---

## Key Takeaways

- Constructors acquire. Destructors release. Never the other way around.
- An object should never exist in a broken state — throw in the constructor if acquisition fails.
- `= delete` moves bugs from runtime to compile time.
- After a move, the source must be left in a valid (destructible) state — hence `fp = nullptr`.
- The destructor must always be safe to call — hence the `if (fp)` guard.