#pragma once 

namespace parametric {

//#ifdef _OPENMP

# include <omp.h>

struct mutex
 {
   mutex() { omp_init_lock(&m_lock); }
   ~mutex() { omp_destroy_lock(&m_lock); }
   void lock() { omp_set_lock(&m_lock); }
   void unlock() { omp_unset_lock(&m_lock); }
   
   mutex(const mutex& ) { omp_init_lock(&m_lock); }
   mutex& operator= (const mutex& ) { return *this; }
 public:
   omp_lock_t m_lock;
 };

//#else
// /* A dummy mutex that doesn't actually exclude anything,
//  * but as there is no parallelism either, no worries. */
// struct MutexType
// {
//   void Lock() {}
//   void Unlock() {}
// };
// #endif
 
/* An exception-safe scoped lock-keeper. */
struct ScopedLock
{
    explicit ScopedLock(mutex& m) : mut(m), locked(true) { mut.lock(); }
    ~ScopedLock() { unlock(); }
    void unlock() { if(!locked) return; locked=false; mut.unlock(); }
    void lock_again() { if(locked) return; mut.lock(); locked=true; }
private:
    mutex& mut;
    bool locked;
private: // prevent copying the scoped lock.
    void operator=(const ScopedLock&);
    ScopedLock(const ScopedLock&);
};

}