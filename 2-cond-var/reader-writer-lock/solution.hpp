#pragma once

#include <rwlock_traits.hpp>

#include <tpcc/stdlike/condition_variable.hpp>

#include <mutex>

namespace tpcc {
namespace solutions {

class ReaderWriterLock {
 public:
  // reader section / shared ownership

  void WriterLock() {
    std::unique_lock<std::mutex> u_lock{mutex_};
    if (bunch_ == 0) {
      ++bunch_;
      bunch_type_ = true;
    }
    if (bunch_type_ == false) {
      ++bunch_;
    }
    bunch_type_ = true;
    size_t this_bunch_ = bunch_++;
    size_t prev_bunch_size_ = bunch_size_;
    bunch_size_ = 0;
    threads_wait_token_.wait(u_lock, [&] {
      return (this_bunch_ == current_bunch_) &&
             (served_writers_ == prev_bunch_size_);
    });
    served_writers_ = 0;
  }

  void WriterUnlock() {
    std::unique_lock<std::mutex> u_lock{mutex_};
    ++current_bunch_;
    threads_wait_token_.notify_all();
  }

  // writer section / exclusive ownership

  void ReaderLock() {
    std::unique_lock<std::mutex> u_lock{mutex_};
    if (bunch_ == 0) {
      ++bunch_;
      bunch_type_ = false;
    }
    if (bunch_type_ == true) {
      bunch_type_ = false;
      bunch_size_ = 0;
    }
    size_t this_bunch_ = bunch_;
    ++bunch_size_;
    threads_wait_token_.wait(u_lock,
                             [&] { return this_bunch_ <= current_bunch_; });
    current_bunch_ = this_bunch_ + 1;
    ++writers_in_critical_section_;
  }

  void ReaderUnlock() {
    std::unique_lock<std::mutex> u_lock{mutex_};
    --writers_in_critical_section_;
    ++served_writers_;
    if (writers_in_critical_section_ == 0) {
      threads_wait_token_.notify_all();
    }
  }

 private:
  size_t current_bunch_{1};
  size_t bunch_{0};
  size_t bunch_size_{0};
  size_t served_writers_{0};
  size_t writers_in_critical_section_{0};
  bool bunch_type_ = true;  // true = reader, false = writer
  tpcc::condition_variable threads_wait_token_;
  std::mutex mutex_;
};

template <>
struct RWLockTraits<ReaderWriterLock> {
  static const RWLockPriority kPriority = RWLockPriority::Fair;
};

}  // namespace solutions
}