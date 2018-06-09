#pragma once

#include <tpcc/concurrency/backoff.hpp>

#include <tpcc/stdlike/atomic.hpp>

namespace tpcc {
namespace solutions {

class TicketLock {
 public:
  // don't change this method
  void Lock() {
    const size_t this_thread_ticket = next_free_ticket_.fetch_add(1);

    Backoff backoff{};
    while (this_thread_ticket != owner_ticket_.load()) {
      backoff();
    }
  }

  bool TryLock() {
    size_t last_served_ticket_{owner_ticket_.load()};
    size_t next_ticket_{last_served_ticket_ + 1};
    return next_free_ticket_.compare_exchange_weak(last_served_ticket_,
                                                   next_ticket_);
  }

  // don't change this method
  void Unlock() {
    owner_ticket_.store(owner_ticket_.load() + 1);
  }

 private:
  tpcc::atomic<size_t> next_free_ticket_{0};
  tpcc::atomic<size_t> owner_ticket_{0};
};

}  // namespace solutions
}  // namespace tpcc
