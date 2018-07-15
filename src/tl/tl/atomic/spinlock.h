//-----------------------------------------------------------------------------
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or distribute
// this software, either in source code form or as a compiled binary, for any
// purpose, commercial or non-commercial, and by any means.
//
// In jurisdictions that recognize copyright laws, the author or authors of
// this software dedicate any and all copyright interest in the software to the
// public domain. We make this dedication for the benefit of the public at
// large and to the detriment of our heirs and successors. We intend this
// dedication to be an overt act of relinquishment in perpetuity of all present
// and future rights to this software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <http://unlicense.org/>
//-----------------------------------------------------------------------------

#ifndef ATOMIC_SPINLOCK_H_
#define ATOMIC_SPINLOCK_H_

#include "atomic.h"

namespace atomic {
class spinlock {
public:
  spinlock() : value_(0) {}

  /// @brief Acquire the lock (blocking).
  /// @note Trying to acquire a lock that is already held by the calling thread
  /// will dead-lock (block indefinitely).
  void lock() {
    while (!value_.compare_exchange(UNLOCKED, LOCKED))
      ;
  }

  /// @brief Release the lock.
  /// @note It is an error to release a lock that has not been previously
  /// acquired.
  void unlock() { value_.store(UNLOCKED); }

private:
  static const int UNLOCKED = 0;
  static const int LOCKED = 1;

  atomic<int> value_;

  ATOMIC_DISALLOW_COPY(spinlock)
};

class lock_guard {
public:
  /// @brief The constructor acquires the lock.
  /// @param lock The spinlock that will be locked.
  explicit lock_guard(spinlock& lock) : lock_(lock) {
    lock_.lock();
  }

  /// @brief The destructor releases the lock.
  ~lock_guard() {
    lock_.unlock();
  }

private:
  spinlock& lock_;

  ATOMIC_DISALLOW_COPY(lock_guard)
};

}  // namespace atomic

#endif  // ATOMIC_SPINLOCK_H_
