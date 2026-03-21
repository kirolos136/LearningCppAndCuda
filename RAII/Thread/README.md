# ThreadHandling — RAII with `std::mutex`

A from-scratch RAII lock guard. No tutorials — written, broken, and rewritten.

---

## The Problem

A mutex prevents multiple threads from accessing shared data simultaneously. You lock it before the critical section, unlock it after.

**Without RAII:**

```cpp
void doWork() {
    mutex.lock();
    
    doSomething();    // throws an exception
    
    mutex.unlock();   // never reached — mutex stays locked forever
}
```

The exception skips `unlock()`. Any other thread that tries to lock the same mutex **waits forever**. That's a deadlock.

---

## What is a Deadlock?

A deadlock is when a thread locks a mutex and never unlocks it. Every other thread trying to acquire that mutex blocks indefinitely. The program freezes.

```
Thread 1: lock() → exception → unlock() skipped → mutex stuck locked
Thread 2: lock() → blocks forever waiting → program hangs
```

---

## The RAII Solution

Tie the lock/unlock to an object's lifetime:

```cpp
void doWork() {
    MutexLock lock(mutex);   // constructor: mutex.lock()
    
    doSomething();           // exception thrown here...
    
}   // ← destructor runs automatically: mutex.unlock() ✅
```

The destructor runs whether the scope exits normally or via exception. The mutex is always unlocked.

---

## Reference Members and the Initializer List

`MutexLock` holds a **reference** to an external mutex — not the mutex itself. The mutex lives outside and is shared between threads. `MutexLock` only manages the lock/unlock lifetime.

```cpp
class MutexLock {
    std::mutex& mtx;   // reference — must be bound at construction
};
```

References **cannot** be assigned after construction. They must be initialized in the **initializer list** — the `: member(value)` syntax before the constructor body:

```cpp
// ❌ Wrong — too late, reference was never bound
MutexLock::MutexLock(std::mutex& m) {
    mtx = m;   // error: references can't be reseated
}

// ✅ Correct — bind the reference before the body runs
MutexLock::MutexLock(std::mutex& m) : mtx(m) {
    mtx.lock();
}
```

| Member type | Can assign in body? | Must use initializer list? |
|---|---|---|
| `int`, `float`, etc | ✅ yes | optional |
| `const` member | ❌ no | ✅ yes |
| Reference member | ❌ no | ✅ yes |

---

## The `locked` Flag

The `bool locked` member tracks whether **this object** currently holds the lock. It's required for two reasons:

**1. Safe destructor after a move**
After `MutexLock b = std::move(a)`, `a.locked` is set to `false`. When `a`'s destructor runs it checks the flag and does nothing — no double-unlock.

**2. Preventing double-unlock**
Unlocking a mutex you don't own is **undefined behavior** — on Linux it may silently corrupt state or terminate the program. The flag makes the destructor safe to call in any state.

```cpp
~MutexLock() {
    if (locked) {           // only unlock if WE own it
        mtx.unlock();
    }
}
```

---

## The Rule of Five

### Rule 1 — Destructor
Unlocks the mutex. Guards with `locked` flag.

---

### Rules 2 & 3 — Copy: DELETED

Copying a lock makes no logical sense. What does it mean to "copy" a held lock?

The auto-generated copy does a shallow copy:
```
mutex2.mtx    → same mutex reference
mutex2.locked → same bool value (true)
```

Both objects think they own the lock. When scope ends both destructors fire — two `unlock()` calls on one mutex → undefined behavior.

```cpp
MutexLock(const MutexLock&)            = delete;
MutexLock& operator=(const MutexLock&) = delete;
```

`= delete` turns a silent runtime bug into a loud compile error:
```
error: use of deleted function 'MutexLock::MutexLock(const MutexLock&)'
```

---

### Rule 4 — Move Constructor

Transferring lock ownership is valid — one object gives it up, another takes it.

```cpp
MutexLock b = std::move(a);
```

`std::move` doesn't move anything — it casts `a` to an rvalue reference (`&&`), telling the compiler "this object is being abandoned, you may steal from it."

```cpp
MutexLock(MutexLock&& other) : mtx(other.mtx), locked(other.locked)
{
    other.locked = false;   // defuse source — its destructor will do nothing
}
```

Memory diagram:
```
BEFORE:
a.locked = true   → holds the lock

AFTER std::move:
a.locked = false  → destructor does nothing
b.locked = true   → sole owner, will unlock
```

---

### Rule 5 — Move Assignment

Same as move constructor but `this` already holds a lock — release it first:

```cpp
MutexLock& operator=(MutexLock&& other)
{
    if (this != &other) {           // guard: don't move into yourself (a = std::move(a))
        if (locked) mtx.unlock();   // release what we currently hold
        locked       = other.locked;
        other.locked = false;       // defuse source
    }
    return *this;                   // return *this enables chaining: a = b = std::move(c)
}
```

---

## `&`, `&&`, and `*` — Quick Reference

| Syntax | Means | Used for |
|---|---|---|
| `T*` | Pointer — stores a memory address | Optional ownership, can be null |
| `T&` | Lvalue reference — alias for existing variable | Copy rules, normal function params |
| `T&&` | Rvalue reference — "I can steal this" | Move constructor and move assignment |
| `const T&` | Read-only alias | Copy rules — won't modify source |
| `std::move(x)` | Cast x to rvalue reference | Trigger move instead of copy |

**Lvalue vs Rvalue:**
```cpp
MutexLock a(m);         // a is an lvalue — has a name, persists beyond the line
std::move(a)            // turns a into an rvalue — "treat a as a temporary"
MutexLock(m)            // rvalue — temporary, no name, dies at end of expression
```

---

## Demos

| Demo | What it shows |
|---|---|
| `unlockingSolved` | RAII destructor unlocks even on exception ✅ |
| `noUnlocking` | Manual lock + exception = deadlock risk ❌ |
| `copyProblem` | Uncomment the copy line to see double-unlock |
| `moveDemo` | Move transfers ownership cleanly — one unlock |

---

## Key Takeaways

- RAII guarantees cleanup because destructors always run — even on exceptions.
- Reference members must be initialized in the initializer list, not the body.
- The `locked` flag is essential: it defuses the destructor after a move and prevents double-unlock.
- Copy is banned because two objects unlocking one mutex is undefined behavior.
- `std::move` doesn't move — it just enables the move constructor/assignment to be called.
- The destructor is guaranteed to run. Your only job is to make sure it's safe when it does.