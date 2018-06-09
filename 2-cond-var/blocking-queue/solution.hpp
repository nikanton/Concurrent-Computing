#pragma once

#include <tpcc/stdlike/condition_variable.hpp>

#include <iostream>
#include <cstddef>
#include <deque>
#include <mutex>
#include <stdexcept>

namespace tpcc {
namespace solutions {

class QueueClosed : public std::runtime_error {
 public:
  QueueClosed() : std::runtime_error("Queue closed for Puts") {
  }
};

template <typename T, class Container = std::deque<T>>
class BlockingQueue {
 public:
  // capacity == 0 means queue is unbounded
  explicit BlockingQueue(const size_t capacity = 0) : capacity_(capacity) {
  }

  // throws QueueClosed exception after Close
  void Put(T item) {
    std::unique_lock<std::mutex> lock{mutex_};

    wait_put_.wait(lock, [&] { return !IsFull() || closed_; });
    if (IsFull() || closed_) {
      throw tpcc::solutions::QueueClosed();
    } else {
      items_.push_back(std::move(item));
      wait_get_.notify_one();
    }
  }

  // returns false iff queue is empty and closed
  bool Get(T& item) {
    std::unique_lock<std::mutex> lock{mutex_};
    wait_get_.wait(lock, [&] { return !IsEmpty() || closed_; });

    if (IsEmpty()) {
      return false;
    } else {
      item = std::move(items_.front());
      items_.pop_front();
      lock.unlock();
      wait_put_.notify_one();
      return true;
    }
  }

  // close queue for Puts
  void Close() {
    std::unique_lock<std::mutex> lock{mutex_};
    closed_ = true;
    lock.unlock();
    wait_put_.notify_all();
    wait_get_.notify_all();
  }

 private:
  // internal predicates for condition variables

  bool IsFull() const {
    return capacity_ == items_.size() && capacity_ != 0;
  }

  bool IsEmpty() const {
    return items_.empty();
  }

 private:
  size_t capacity_;
  Container items_;
  bool closed_{false};
  std::mutex mutex_;
  tpcc::condition_variable wait_put_;
  tpcc::condition_variable wait_get_;
};

}  // namespace solutions
}  // namespace tpcc
