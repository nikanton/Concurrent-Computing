#pragma once

#include <tpcc/concurrency/backoff.hpp>
#include <tpcc/memory/bump_pointer_allocator.hpp>
#include <tpcc/stdlike/atomic.hpp>
#include <tpcc/support/compiler.hpp>

#include <limits>
#include <mutex>
#include <thread>
#include <utility>

namespace tpcc {
namespace solutions {

////////////////////////////////////////////////////////////////////////////////

class SpinLock {
 public:
  void Lock() {
    while (locked_.exchange(true)) {
      while (locked_.load() == true)
        ;
    }
  }

  void Unlock() {
    locked_.store(false);
  }

  // adapters for BasicLockable concept

  void lock() {
    Lock();
  }

  void unlock() {
    Unlock();
  }

 private:
  tpcc::atomic<bool> locked_{false};
};

////////////////////////////////////////////////////////////////////////////////

// don't touch this
template <typename T>
struct KeyTraits {
  static T LowerBound() {
    return std::numeric_limits<T>::min();
  }

  static T UpperBound() {
    return std::numeric_limits<T>::max();
  }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T, class TTraits = KeyTraits<T>>
class OptimisticLinkedSet {
 private:
  struct Node {
    T key_;
    tpcc::atomic<Node*> next_;
    SpinLock spinlock_;
    tpcc::atomic<bool> marked_{false};

    Node(const T& key, Node* next = nullptr) : key_(key), next_(next) {
    }

    // use: auto node_lock = node->Lock();
    std::unique_lock<SpinLock> Lock() {
      return std::unique_lock<SpinLock>{spinlock_};
    }
  };

  struct EdgeCandidate {
    Node* pred_;
    Node* curr_;

    EdgeCandidate(Node* pred, Node* curr) : pred_(pred), curr_(curr) {
    }
  };

 public:
  explicit OptimisticLinkedSet(BumpPointerAllocator& allocator)
      : allocator_(allocator) {
    CreateEmptyList();
  }

  bool Insert(T key) {
    while (true) {
      auto edge_ = Locate(key);
      auto pred_lock_ = edge_.pred_->Lock();
      auto curr_lock_ = edge_.curr_->Lock();
      if (!Validate(edge_)) {
        continue;
      }
      if (edge_.curr_->key_ == key) {
        return false;
      } else {
        auto to_be_inserted_ = allocator_.New<Node>(key, edge_.curr_);
        edge_.pred_->next_ = to_be_inserted_;
        ++size_;
        return true;
      }
    }
  }

  bool Remove(const T& key) {
    while (true) {
      auto edge_ = Locate(key);
      auto pred_lock_ = edge_.pred_->Lock();
      auto curr_lock_ = edge_.curr_->Lock();
      if (!Validate(edge_)) {
        continue;
      }
      if (edge_.curr_->key_ != key) {
        return false;
      } else {
        edge_.pred_->next_.store(edge_.curr_->next_);
        edge_.curr_->marked_ = true;
        --size_;
        return true;
      }
    }
  }

  bool Contains(const T& key) const {
    auto edge_ = Locate(key);
    return edge_.curr_->key_ == key && !edge_.curr_->marked_;
  }

  size_t GetSize() const {
    return size_.load();  // to be implemented
  }

 private:
  void CreateEmptyList() {
    // create sentinel nodes
    head_ = allocator_.New<Node>(TTraits::LowerBound());
    head_->next_ = allocator_.New<Node>(TTraits::UpperBound());
  }

  EdgeCandidate Locate(const T& key) const {
    Node* less_ = head_;
    Node* more_ = less_->next_;
    while (more_->key_ < key) {
      less_ = more_;
      more_ = more_->next_;
    }
    return {less_, more_};
  }

  bool Validate(const EdgeCandidate& edge) const {
    return !edge.curr_->marked_ && !edge.pred_->marked_ &&
           edge.pred_->next_ == edge.curr_;
  }

 private:
  BumpPointerAllocator& allocator_;
  Node* head_{nullptr};
  tpcc::atomic<size_t> size_{0};
};

}  // namespace solutions
}  // namespace tpcc
