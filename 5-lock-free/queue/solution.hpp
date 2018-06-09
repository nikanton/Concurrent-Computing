#pragma once

#include <tpcc/stdlike/atomic.hpp>
#include <tpcc/support/compiler.hpp>
#include <tpcc/concurrency/backoff.hpp>

#include <utility>
#include <iostream>

namespace tpcc {
namespace solutions {

template <typename T>
class LockFreeQueue {
  struct Node {
    T item_;
    tpcc::atomic<Node*> next_{nullptr};

    Node() {
    }

    explicit Node(T item, Node* next = nullptr)
        : item_(std::move(item)), next_(next) {
    }
  };

 public:
  LockFreeQueue() {
    Node* dummy = new Node{};
    head_ = dummy;
    tail_ = dummy;
    waste_head_ = dummy;
  }

  ~LockFreeQueue() {
    while (waste_head_ != tail_.load()) {
      Node* item_to_delete_ = waste_head_;
      waste_head_ = waste_head_->next_;
      delete item_to_delete_;
    }
    delete tail_.load();
  }

  void Enqueue(T item) {
    ++deals_with_queue_;
    Node* new_element_ = new Node(item);
    Node* current_tail_ = tail_.load();
    Node* null_ptr_ = null_ptr_;
    while (true) {
      if (current_tail_->next_ == nullptr) {
        if (current_tail_->next_.compare_exchange_strong(null_ptr_,
                                                         new_element_)) {
          break;
        }
      } else {
        tail_.compare_exchange_weak(current_tail_, current_tail_->next_);
      }
    }
    tail_.compare_exchange_strong(current_tail_, new_element_);
    --deals_with_queue_;
  }

  bool Dequeue(T& item) {
    ++deals_with_queue_;
    while (true) {
      Node* current_tail_ = tail_.load();
      Node* current_head_ = head_.load();
      if (current_head_ == current_tail_) {
        if (current_head_->next_ == nullptr) {
          --deals_with_queue_;
          return false;
        } else {
          tail_.compare_exchange_weak(current_head_, current_tail_->next_);
        }
      } else {
        if (head_.compare_exchange_strong(current_head_,
                                          current_head_->next_)) {
          item = std::move(current_head_->next_.load()->item_);
          if (deals_with_queue_.load() == 1) {
            while (waste_head_ != current_head_) {
              Node* item_to_delete_ = waste_head_;
              waste_head_ = waste_head_->next_;
              delete item_to_delete_;
            }
          }
          --deals_with_queue_;
          return true;
        }
      }
    }
  }

 private:
  tpcc::atomic<size_t> deals_with_queue_{0};
  Node* waste_head_{nullptr};
  tpcc::atomic<Node*> head_{nullptr};
  tpcc::atomic<Node*> tail_{nullptr};
};

}  // namespace solutions
}  // namespace tpcc
