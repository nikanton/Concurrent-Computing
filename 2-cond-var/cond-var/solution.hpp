#pragma once

#include <tpcc/concurrency/futex.hpp>
 
#include <atomic>

namespace tpcc {
namespace solutions {

class ConditionVariable {
 public:
  ConditionVariable() : futex_(signal_count_) {
  }

  template <class Mutex>
  void Wait(Mutex& mutex) {
    uint32_t this_thread_index_ = signal_count_.load();
    mutex.unlock();
    futex_.Wait(this_thread_index_);
    mutex.lock();
  }

  void NotifyOne() {
    ++signal_count_;
    futex_.WakeOne();
  }

  void NotifyAll() {
    ++signal_count_;
    futex_.WakeAll();
  }

 private:
  std::atomic<uint32_t> signal_count_{0};
  tpcc::Futex futex_;
};

}  // namespace solutions
}  // namespace tpcc
