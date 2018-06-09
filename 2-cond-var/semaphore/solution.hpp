#pragma once

#include <tpcc/stdlike/condition_variable.hpp>
#include <mutex>
#include <atomic>

namespace tpcc {
namespace solutions {

class Semaphore {
 public:
  explicit Semaphore(const size_t capacity = 0) : capacity_(capacity) {
  }

  void Acquire() {
    std::unique_lock<std::mutex> u_lock{mutex_};
    while (current_tokens_.load() == capacity_) {
      has_tokens_.wait(u_lock);
    }
    ++current_tokens_;
  }

  void Release() {
    std::unique_lock<std::mutex> u_lock{mutex_};
    --current_tokens_;
    has_tokens_.notify_one();
  }

 private:
  std::mutex mutex_;
  const std::size_t capacity_;
  std::atomic<std::size_t> current_tokens_{0};
  tpcc::condition_variable has_tokens_;
};

}  // namespace solutions
}  // namespace tpcc
