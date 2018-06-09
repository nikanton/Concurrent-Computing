#pragma once

#include <tpcc/stdlike/atomic.hpp>
#include <tpcc/support/compiler.hpp>

#include <utility>

namespace tpcc {
namespace solutions {

// Treiber lock-free stack

template <typename T>
class LockFreeStack {
  struct Node {
    T item_;
    std::atomic<Node*> next;

    Node(T item) : item_(std::move(item)) {
    }
  };

  void ContinuePush(Node* new_top, tpcc::atomic<Node*>& top) {
    auto current_top = top.load();
    do {
      new_top->next.store(current_top);
    } while (!top.compare_exchange_strong(current_top, new_top));
  }

 public:
  LockFreeStack() = default;

  ~LockFreeStack() {
    /*T item_;
    while (Pop(item_))
      ;*/
    Node* trash_top_ = trash_list_top_.load();
    trash_list_top_.store(nullptr);
    while (trash_top_ != nullptr) {
      Node* old_node_ = trash_top_;
      trash_top_ = trash_top_->next;
      delete old_node_;
    }
    while (top_.load() != nullptr) {
      Node* old_node_ = top_.load();
      top_.store(old_node_->next);
      delete old_node_;
    }
  }

  void Push(T item) {
    Node* new_top_ = new Node(item);
    ContinuePush(new_top_, top_);
  }

  bool Pop(T& item) {
    Node* old_top_ = top_.load();
    do {
      if (old_top_ == nullptr)
        return false;
    } while (!top_.compare_exchange_strong(old_top_, old_top_->next));
    item = old_top_->item_;
    ContinuePush(old_top_, trash_list_top_);
    return true;
  }

 private:
  tpcc::atomic<Node*> top_{nullptr};
  tpcc::atomic<Node*> trash_list_top_{nullptr};
};

}  // namespace solutions
}  // namespace tpcc
