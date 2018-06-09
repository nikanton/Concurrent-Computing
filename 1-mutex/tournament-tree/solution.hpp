#pragma once

#include <tpcc/concurrency/backoff.hpp>
#include <tpcc/support/compiler.hpp>
#include <tpcc/stdlike/atomic.hpp>

#include <vector>

namespace tpcc {
namespace solutions {

class PetersonLock {
 public:
  PetersonLock() {
    want_[0].store(false);
    want_[1].store(false);
    victim_.store(0);
  }

  void Lock(size_t thread_index) {
    want_[thread_index].store(true);
    victim_.store(thread_index);
    Backoff backoff{};
    while (want_[1 - thread_index].load() && victim_.load() == thread_index) {
      backoff();
    }
  }

  void Unlock(size_t thread_index) {
    want_[thread_index].store(false);
  }

 private:
  std::array<std::atomic<bool>, 2> want_;
  atomic<size_t> victim_;
};

class TournamentTreeLock {
 public:
  explicit TournamentTreeLock(size_t num_threads) {
    tree_base_ = 2;
    while (num_threads > tree_base_) {
      tree_base_ <<= 1;
    }
    locks_ = std::vector<PetersonLock>(tree_base_);
  }

  void Lock(size_t thread_index) {
    size_t current_lock_ = GetThreadLeaf(thread_index);
    size_t child_type_;
    while (current_lock_ != 0) {
      child_type_ = current_lock_ % 2;
      current_lock_ = GetParent(current_lock_);
      locks_[current_lock_].Lock(child_type_);
    }
  }

  void RecurrentUnlock(size_t leaf_index, size_t child_type) {
    if (leaf_index != 0) {
      RecurrentUnlock(GetParent(leaf_index), leaf_index % 2);
    }
    locks_[leaf_index].Unlock(child_type);
  }

  void Unlock(size_t thread_index) {
    RecurrentUnlock(GetParent(GetThreadLeaf(thread_index)),
                    GetThreadLeaf(thread_index) % 2);
  }

 private:
  size_t GetParent(size_t node_index) const {
    return (node_index + 1) / 2 - 1;
  }

  size_t GetLeftChild(size_t node_index) const {
    return (node_index + 1) * 2 - 1;
  }

  size_t GetRightChild(size_t node_index) const {
    return (node_index + 1) * 2;
  }

  size_t GetThreadLeaf(size_t thread_index) const {
    return tree_base_ + thread_index - 1;
  }

 private:
  size_t tree_base_;
  std::vector<PetersonLock> locks_;
};

}  // namespace solutions
}  // namespace tpcc
