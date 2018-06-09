#pragma once

#include <tpcc/stdlike/condition_variable.hpp>

#include <cstddef>
#include <mutex>
#include <atomic>

namespace tpcc {
namespace solutions {

class CyclicBarrier {
 public:
  explicit CyclicBarrier(const size_t num_threads)
      : threads_(num_threads), thread_necessary_to_pas_(num_threads) {
  }

  void PassThrough() {
    std::unique_lock<std::mutex> lock{mutex_};
    --thread_necessary_to_pas_;
    if (thread_necessary_to_pas_ == 0) {
      ++cycles_;
      all_threads_arrived_.notify_all();
      thread_necessary_to_pas_ = threads_;
    } else {
      size_t cycle_of_this_thread_ = cycles_;
      all_threads_arrived_.wait(
          lock, [&]() { return cycle_of_this_thread_ < cycles_; });
    }
  }

 private:
  std::mutex mutex_;
  std::condition_variable all_threads_arrived_;
  size_t thread_necessary_to_pas_;
  size_t cycles_{0};
  const size_t threads_;
};

}  // namespace solutions
}  // namespace tpcc
