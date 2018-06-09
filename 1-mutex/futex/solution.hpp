#pragma once

#include "futex_like.hpp"

#include <tpcc/stdlike/atomic.hpp>

#include <mutex>

namespace tpcc {
namespace solutions {

class AdaptiveLock {
 public:
  AdaptiveLock() {
  }

  void Lock() {
    threads_in_queue_.fetch_add(1);
    while (lock_.exchange(true)) {
      futex_.Wait(true);
    }
  }

  void Unlock() {
    lock_.store(false);
    if (threads_in_queue_.fetch_sub(1) > 1) {
      futex_.WakeOne();
    }
  }

 private:
  atomic<bool> lock_{false}, for_futex_{false};
  atomic<size_t> threads_in_queue_{0};
  FutexLike<bool> futex_{for_futex_};
};

}  // namespace solutions
}  // namespace tpcc
