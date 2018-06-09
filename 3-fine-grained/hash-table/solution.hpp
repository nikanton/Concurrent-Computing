#pragma once

#include <tpcc/stdlike/atomic.hpp>
#include <tpcc/stdlike/condition_variable.hpp>
#include <tpcc/stdlike/mutex.hpp>

#include <tpcc/support/compiler.hpp>

#include <algorithm>
#include <iostream>
#include <forward_list>
#include <functional>
#include <shared_mutex>
#include <vector>
#include <utility>

namespace tpcc {
namespace solutions {

class ReaderWriterLock {
 public:
  ReaderWriterLock() = default;
  void lock() {
    std::unique_lock<std::mutex> u_lock_{mutex_};
    ++writers_;
    thread_wait_.wait(
        u_lock_, [&] { return !locked_by_writer_ && locks_by_readers_ == 0; });
    locked_by_writer_ = true;
  }

  void unlock() {
    std::unique_lock<std::mutex> u_lock_{mutex_};
    locked_by_writer_ = false;
    --writers_;
    thread_wait_.notify_all();
  }

  void lock_shared() {
    std::unique_lock<std::mutex> u_lock_{mutex_};
    thread_wait_.wait(u_lock_,
                      [&] { return !locked_by_writer_ && writers_ == 0; });
    ++locks_by_readers_;
  }

  void unlock_shared() {
    std::unique_lock<std::mutex> u_lock_{mutex_};
    --locks_by_readers_;
    if (locks_by_readers_ == 0) {
      thread_wait_.notify_all();
    }
  }

 private:
  size_t writers_{0};
  size_t locks_by_readers_{0};
  bool locked_by_writer_{false};
  tpcc::condition_variable thread_wait_;
  std::mutex mutex_;
};

template <typename T, class HashFunction = std::hash<T>>
class StripedHashSet {
 private:
  using RWLock = ReaderWriterLock;

  using ReaderLocker = std::shared_lock<RWLock>;
  using WriterLocker = std::unique_lock<RWLock>;

  using Bucket = std::forward_list<T>;
  using Buckets = std::vector<Bucket>;

 public:
  explicit StripedHashSet(const size_t concurrency_level = 4,
                          const size_t growth_factor = 2,
                          const double max_load_factor = 0.8)
      : concurrency_level_(concurrency_level),
        growth_factor_(growth_factor),
        max_load_factor_(max_load_factor),
        // stripe_locks_(concurrency_level),
        elements_(concurrency_level) {
    for (size_t i = 0; i < concurrency_level_; ++i) {
      stripe_locks_.push_back(new RWLock);
    }
  }

  ~StripedHashSet() {
    for (size_t i = 0; i < concurrency_level_; ++i) {
      delete stripe_locks_[i];
    }
  }

  bool Insert(T element) {
    size_t hash_value_ = HashFunction{}(element);
    auto stripe_lock_ = LockStripe<WriterLocker>(hash_value_);
    size_t bucket_index_ = GetBucketIndex(hash_value_);
    Bucket& bucket_ = GetBucket(bucket_index_);
    if (std::find(bucket_.cbegin(), bucket_.cend(), element) != bucket_.end()) {
      return false;
    } else {
      bucket_.push_front(element);
      ++elements_in_set_;
      if (MaxLoadFactorExceeded()) {
        size_t arr_size = elements_.size();
        stripe_lock_.unlock();
        TryExpandTable(arr_size);
      }
      return true;
    }
  }

  bool Remove(const T& element) {
    size_t hash_value_ = HashFunction{}(element);
    auto stripe_lock_ = LockStripe<WriterLocker>(hash_value_);
    size_t bucket_index_ = GetBucketIndex(hash_value_);
    Bucket& bucket_ = GetBucket(bucket_index_);
    auto remove_candidate_ =
        std::find(bucket_.cbegin(), bucket_.cend(), element);
    if (remove_candidate_ == bucket_.end()) {
      return false;
    } else {
      bucket_.remove(element);
      --elements_in_set_;
      return true;
    }
  }

  bool Contains(const T& element) const {
    size_t hash_value_ = HashFunction{}(element);
    auto stripe_lock_ = LockStripe<ReaderLocker>(hash_value_);
    size_t bucket_index_ = GetBucketIndex(hash_value_);
    const Bucket& bucket_ = GetBucket(bucket_index_);
    return std::find(bucket_.cbegin(), bucket_.cend(), element) !=
           bucket_.cend();
  }

  size_t GetSize() const {
    return elements_in_set_.load();
  }

  size_t GetBucketCount() const {
    auto stripe_lock_ = LockStripe<ReaderLocker>(0);
    return elements_.size();
  }

 private:
  size_t GetStripeIndex(const size_t hash_value) const {
    return hash_value % concurrency_level_;
  }

  template <class Locker>
  Locker LockStripe(const size_t hash_value) const {
    size_t lock_index_ = GetStripeIndex(hash_value);
    RWLock& stripes_lock_ = *stripe_locks_[lock_index_];
    Locker lock_(stripes_lock_);
    return std::move(lock_);
  }

  size_t GetBucketIndex(const size_t hash_value) const {
    return hash_value % elements_.size();
  }

  Bucket& GetBucket(const size_t hash_value) {
    return elements_[GetBucketIndex(hash_value)];
  }

  const Bucket& GetBucket(const size_t hash_value) const {
    return elements_[GetBucketIndex(hash_value)];
  }

  bool MaxLoadFactorExceeded() const {
    return elements_in_set_.load() > max_load_factor_ * elements_.size();
  }

  void TryExpandTable(const size_t expected_bucket_count) {
    std::vector<WriterLocker> locks_{concurrency_level_};
    locks_[0] = LockStripe<WriterLocker>(0);
    if (elements_.size() != expected_bucket_count) {
      return void();
    }
    for (size_t i = 1; i < concurrency_level_; ++i) {
      locks_[i] = LockStripe<WriterLocker>(i);
    }
    size_t new_size_ = expected_bucket_count * growth_factor_;
    std::vector<std::forward_list<T>> new_elements_{new_size_};
    for (auto& i : elements_) {
      for (auto& j : i) {
        size_t hash_value_ = HashFunction{}(j);
        new_elements_[hash_value_ % new_size_].push_front(j);
      }
    }

    elements_ = std::move(new_elements_);
  }

 private:
  std::vector<std::forward_list<T>> elements_;
  std::vector<RWLock*> stripe_locks_;
  size_t concurrency_level_;
  size_t growth_factor_;
  std::atomic<size_t> elements_in_set_{0};
  double max_load_factor_;
};

}  // namespace solutions
}  // namespace tpcc
