# MyUniquePtr — Study Notes
> Smart pointers internals · Days 3–4

---

## Table of Contents
1. [Core Idea](#1-core-idea)
2. [Method-by-Method Reference](#2-method-by-method-reference)
3. [Key Rules to Remember](#3-key-rules-to-remember)
4. [Custom Deleters](#4-custom-deleters)
5. [Empty Base Optimisation (EBO)](#5-empty-base-optimisation-ebo)
6. [Zero-Cost Abstraction](#6-zero-cost-abstraction)
7. [Template Class Syntax — Quick Reference](#7-template-class-syntax--quick-reference)

---

## 1. Core Idea

A `MyUniquePtr<T>` is just a class that wraps a single raw pointer and enforces ownership rules around it. Internally it holds exactly one member:

```cpp
T* ptr;
```

Everything else — constructors, destructor, move semantics — is **policy** around that pointer.

> **Mental model:** the object's lifetime = the resource's lifetime. The resource lives exactly as long as its owner, no more, no less.

---

## 2. Method-by-Method Reference

| Method | Core job | Key detail |
|---|---|---|
| Constructor | Store the pointer, take ownership | Caller already did `new` — you just hold it |
| Destructor | `delete` the pointer | `if(ptr)` check before deleting |
| Copy ctor/assign | `= delete` — prevent two owners | Compile-time error, not runtime |
| Move ctor | Steal ptr, null out source | `other.ptr = nullptr` is everything |
| Move assign | Delete own, steal, null source | Check `this != &other` first |
| `get()` | Return ptr, keep ownership | Lending a view — no transfer |
| `release()` | Return ptr, give up ownership | Sets internal ptr to `nullptr` |
| `reset()` | Delete own ptr, store new one | Default param `nullptr` to clear |
| `operator*` | Return `*ptr` — a reference | `T&` not `T` — avoids copying |
| `operator->` | Return `ptr` — the raw pointer | C++ applies `->` automatically |
| `operator bool` | Return `ptr != nullptr` | Mark `explicit` to prevent accidents |

---

## 3. Key Rules to Remember

### Move constructor — the null-out rule

After stealing `other.ptr` you **must** null it out. If you forget, both destructors fire on the same address — double-free.

```cpp
MyUniquePtr<T>::MyUniquePtr(MyUniquePtr&& other) : ptr(other.ptr) {
    other.ptr = nullptr;   // without this — double-free
}
```

### Move assignment — delete before stealing

The left-hand side already owns something. Delete it first or it leaks. Then steal and null.

```cpp
if (this != &other) {
    if (ptr) d(ptr);       // free current resource first
    ptr = other.ptr;       // steal
    other.ptr = nullptr;   // null source
}
```

### Default argument — declaration only

Default parameter values go in the declaration inside the class, **never** repeated in the definition outside.

```cpp
// Inside class — OK
void reset(T* p = nullptr);

// Outside definition — no default again
template <typename T, typename Deleter>
void MyUniquePtr<T, Deleter>::reset(T* p) { ... }
```

### `->` vs `.` — the classic trap

Inside template methods, `other` is always a **reference** — use `.` not `->`. Only raw pointers use `->`.

```cpp
other.ptr = nullptr;   // correct — other is a reference
other->ptr = nullptr;  // wrong   — other is not a pointer
```

### `operator bool` — why `explicit`

Without `explicit`, the compiler can silently convert your object to `bool` in unintended places (like arithmetic). With `explicit` it only works in boolean contexts like `if`.

```cpp
explicit operator bool() const { return ptr != nullptr; }

// usage
if (p)   { ... }   // works  ✅
if (!p)  { ... }   // works  ✅
int x = p;         // compile error  ✅ — prevented by explicit
```

---

## 4. Custom Deleters

### The problem with hardcoded `delete`

```cpp
~MyUniquePtr() {
    delete ptr;   // wrong for arrays, file handles, custom allocators
}
```

| Resource | Needs |
|---|---|
| `new int(42)` | `delete ptr` |
| `new int[100]` | `delete[] ptr` |
| `fopen(...)` | `fclose(ptr)` |
| Custom allocator | `MyPool::free(ptr)` |

### What a deleter is

Any callable — a struct with `operator()`, a function pointer, or a lambda. The destructor calls `d(ptr)` instead of `delete ptr`.

```cpp
struct MyDeleter {
    void operator()(int* p) { delete p; }
};

// d(ptr) is identical to d.operator()(ptr)
// C++ translates one into the other automatically
```

### Why a struct, not a plain function

- **Can carry state** — member variables persist between calls
- **Is a type** — can be a template parameter; functions are not types
- **Enables EBO** — empty structs can be zero-size via inheritance (see section 5)

### Make it a template to handle any type

```cpp
template <typename T>
struct MyDeleter {
    void operator()(T* p) { delete p; }
};

MyUniquePtr<int,  MyDeleter<int>>   p1(new int(42),  MyDeleter<int>{});
MyUniquePtr<Demo, MyDeleter<Demo>>  p2(new Demo(),   MyDeleter<Demo>{});
```

### Add a default so the user doesn't always need to specify it

```cpp
template <typename T, typename Deleter = std::default_delete<T>>
class MyUniquePtr : private Deleter { ... };

MyUniquePtr<int>  p(new int(42));   // uses default_delete<int> automatically
```

> `std::default_delete<T>` is the standard's built-in stateless deleter — just calls `delete p`. Use your own only for special cases like arrays or file handles.

---

## 5. Empty Base Optimisation (EBO)

> **This explains why `sizeof(unique_ptr<T>) == sizeof(T*)` with a stateless deleter.**

### 5.1 The Problem — naive storage bloats size

If you store the deleter as a member, even an empty struct costs space:

```cpp
struct EmptyDeleter {};
sizeof(EmptyDeleter);   // == 1, NOT 0
                        // C++ rule: every object needs at least 1 byte
                        // so that every object has a unique address

// Stored as a member — pays the size cost:
struct NaiveUniquePtr {
    T*            ptr;       // 8 bytes
    EmptyDeleter  deleter;   // 1 byte + 7 padding = 8 bytes
};
sizeof(NaiveUniquePtr)  // == 16 bytes — double a raw pointer!
```

> For millions of objects or arrays of smart pointers, doubling the size is a real performance cost — not just theoretical.

### 5.2 The Rule — empty bases take zero space

C++ has a special rule for **inheritance** that does NOT apply to members:

```cpp
struct Empty {};

// As a member — costs at least 1 byte
struct WithMember { Empty e; int* ptr; };
sizeof(WithMember)  // 16 bytes (with padding)

// As a base class — costs 0 bytes
struct WithBase : private Empty { int* ptr; };
sizeof(WithBase)    // 8 bytes — Empty contributes nothing
```

The compiler sees the base has no data and collapses it to zero. This is **EBO**.

### 5.3 The Solution — inherit from the deleter instead of storing it

```cpp
template <typename T, typename Deleter = std::default_delete<T>>
class MyUniquePtr : private Deleter {   // inherit — don't store
private:
    T* ptr;                             // no Deleter d; member anymore
public:
    ~MyUniquePtr() {
        if (ptr) Deleter::operator()(ptr);  // call via base class scope
    }
};

sizeof(MyUniquePtr<int>)  // == sizeof(int*) == 8 bytes  ✅
```

### 5.4 When EBO does NOT apply

EBO only works when the base class has **no member variables**. The moment the deleter holds data, it must be stored physically:

```cpp
// Stateless — EBO applies ✅
struct SimpleDeleter {
    void operator()(int* p) { delete p; }
    // no members
};

// Stateful — EBO cannot apply ❌
struct LoggingDeleter {
    std::string label;   // this is data — must live somewhere in memory
    void operator()(int* p) {
        std::cout << label;
        delete p;
    }
};
```

### 5.5 Size table

| Deleter type | Has data? | EBO applies | `sizeof(UniquePtr<T>)` |
|---|---|---|---|
| `std::default_delete<T>` | No | Yes ✅ | `== sizeof(T*)` |
| Empty struct (stateless) | No | Yes ✅ | `== sizeof(T*)` |
| Struct with members | Yes | No ❌ | `> sizeof(T*)` |
| Lambda with captures | Yes | No ❌ | `> sizeof(T*)` |
| Lambda without captures (C++20) | No | Yes ✅ | `== sizeof(T*)` |

### 5.6 How to implement EBO in MyUniquePtr

Three changes to your existing code:

**Step 1 — Inherit from `Deleter` privately, remove the member:**

```cpp
template <typename T, typename Deleter = std::default_delete<T>>
class MyUniquePtr : private Deleter {   // ← inherit, not store
private:
    T* ptr;                             // ← remove: Deleter d;
    ...
};
```

**Step 2 — Update constructors to initialise the base:**

```cpp
// Default constructor
template <typename T, typename Deleter>
MyUniquePtr<T, Deleter>::MyUniquePtr()
    : Deleter(), ptr(nullptr) {}

// Constructor with pointer + deleter
template <typename T, typename Deleter>
MyUniquePtr<T, Deleter>::MyUniquePtr(T* p, Deleter del)
    : Deleter(del), ptr(p) {}
```

**Step 3 — Update destructor to call the base:**

```cpp
template <typename T, typename Deleter>
MyUniquePtr<T, Deleter>::~MyUniquePtr() {
    if (ptr) Deleter::operator()(ptr);   // call via base class scope
}
```

> Remove the `Deleter d;` member from the class. Everything else — move ctor, move assign, `get()`, `release()`, `reset()` — stays exactly the same.

---

## 6. Zero-Cost Abstraction

`std::unique_ptr` is called a **zero-cost abstraction** because:

- **Same size as a raw pointer** — `sizeof(unique_ptr<T>) == sizeof(T*)` with a stateless deleter (via EBO)
- **No runtime overhead** — destructor call is inlined by the compiler, no virtual dispatch
- **Compile-time enforcement** — deleted copy constructor catches ownership errors before running
- **No extra allocations** — unlike `shared_ptr`, there is no control block heap allocation

> You pay nothing at runtime for the safety you gain at compile time. That is the definition of zero-cost.

---

## 7. Template Class Syntax — Quick Reference

```cpp
// Declaration inside class — normal, no template prefix needed
template <typename T, typename Deleter = std::default_delete<T>>
class MyUniquePtr : private Deleter {
    T*   get();
    void reset(T* p = nullptr);   // default value here only
};

// Definition outside — always needs template prefix + <T, Deleter>
template <typename T, typename Deleter>
T* MyUniquePtr<T, Deleter>::get() {
    return ptr;
}

template <typename T, typename Deleter>
void MyUniquePtr<T, Deleter>::reset(T* p) {   // no default here
    if (ptr) Deleter::operator()(ptr);
    ptr = p;
}
```

### The pattern is always:
```
template <typename T, typename Deleter>
[return type]  MyUniquePtr<T, Deleter>::[method]([params]) { }
```

### Common return types

| Returns | Means |
|---|---|
| `T` | a copy of the object |
| `T&` | a reference to the object — use for `operator*` |
| `T*` | a raw pointer — use for `get()`, `release()`, `operator->` |
| `void` | nothing — use for `reset()` |

### The one-file rule

Unlike regular classes, template class **declarations and definitions** must both live in the same `.h` file. The compiler needs to see the full definition at the point where it instantiates `T`.
