#pragma once

#include <tpcc/stdlike/atomic.hpp>
#include <tpcc/concurrency/backoff.hpp>

namespace tpcc {
namespace solutions {

class QueueSpinLock {
 public:
  class LockGuard {
   public:
    explicit LockGuard(QueueSpinLock& spinlock) : spinlock_(spinlock) {
      AcquireLock();
    }

    ~LockGuard() {
      ReleaseLock();
    }

   private:
    void AcquireLock() {
      // add self to spinlock queue and wait for ownership
      LockGuard* prev_tail = spinlock_.wait_queue_tail_.exchange(this);
      if (prev_tail != nullptr) {
        prev_tail->next_ = this;
        Backoff backoff{};
        while (!is_owner_) {
          backoff();
        }
      }
    }

    void ReleaseLock() {
      /* transfer ownership to the next guard node in spinlock wait queue
       * or reset tail pointer if there are no other contenders
       */
      LockGuard* this_ptr_ = this;
      if (!spinlock_.wait_queue_tail_.compare_exchange_strong(this_ptr_,
                                                              nullptr)) {
        Backoff backoff{};
        while (next_.load() == nullptr) {
          backoff();
        }
        next_.load()->is_owner_.store(true);
      }
    }

   private:
    QueueSpinLock& spinlock_;

    tpcc::atomic<bool> is_owner_{false};
    tpcc::atomic<LockGuard*> next_{nullptr};
  };

 private:
  // tail of intrusive list of LockGuards
  tpcc::atomic<LockGuard*> wait_queue_tail_{nullptr};
};

}  // namespace solutions
}  // namespace tpcc
