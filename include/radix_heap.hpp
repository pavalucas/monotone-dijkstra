#pragma once

#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "graph.hpp"

namespace sssp {

class RadixHeapQueue {
 public:
  void push(Weight key, Vertex vertex) {
    if (key < last_deleted_key_) {
      throw std::invalid_argument("radix heap requires monotone keys");
    }
    buckets_[bucket_index(key)].push_back({key, vertex});
    ++size_;
  }

  bool empty() const {
    return size_ == 0;
  }

  std::pair<Weight, Vertex> pop_min() {
    if (empty()) {
      throw std::out_of_range("pop_min on empty radix heap");
    }

    if (buckets_[0].empty()) {
      std::size_t i = 1;
      while (i < buckets_.size() && buckets_[i].empty()) {
        ++i;
      }

      Weight new_last = std::numeric_limits<Weight>::max();
      for (const auto& item : buckets_[i]) {
        if (item.first < new_last) {
          new_last = item.first;
        }
      }
      last_deleted_key_ = new_last;

      auto items = std::move(buckets_[i]);
      buckets_[i].clear();
      for (const auto& item : items) {
        buckets_[bucket_index(item.first)].push_back(item);
      }
    }

    auto item = buckets_[0].back();
    buckets_[0].pop_back();
    --size_;
    last_deleted_key_ = item.first;
    return item;
  }

 private:
  static int most_significant_bit(Weight value) {
    return 63 - __builtin_clzll(value);
  }

  std::size_t bucket_index(Weight key) const {
    Weight diff = key ^ last_deleted_key_;
    if (diff == 0) {
      return 0;
    }
    return static_cast<std::size_t>(most_significant_bit(diff) + 1);
  }

  Weight last_deleted_key_ = 0;
  std::array<std::vector<std::pair<Weight, Vertex>>, 65> buckets_;
  std::size_t size_ = 0;
};

// Two-level radix heap: a base-K radix heap with K = 2^kRadixBits. Each monotone
// key is read as a sequence of base-K digits relative to the current minimum
// (base_). The first level (kDigits top buckets) selects the most significant
// digit position at which the key differs from base_; the second level (kRadix
// sub-buckets) indexes that digit's value. Compared with the one-level heap,
// using a radix larger than 2 lowers the number of digit positions an element
// can descend through to O(log C / log log C) when the radix matches the key
// range.
class TwoLevelRadixHeapQueue {
 public:
  void push(Weight key, Vertex vertex) {
    if (key < base_) {
      throw std::invalid_argument("two-level radix heap requires monotone keys");
    }
    const auto [d, s] = bucket_of(key);
    buckets_[d][s].push_back({key, vertex});
    ++size_;
  }

  bool empty() const {
    return size_ == 0;
  }

  std::pair<Weight, Vertex> pop_min() {
    if (empty()) {
      throw std::out_of_range("pop_min on empty two-level radix heap");
    }

    while (true) {
      const auto [d, s] = first_nonempty_bucket();
      if (d == 0) {
        // A top bucket d == 0 holds a single exact key value, so its minimum is
        // the global minimum and can be returned directly.
        auto& bucket = buckets_[0][s];
        auto item = bucket.back();
        bucket.pop_back();
        --size_;
        base_ = item.first;
        return item;
      }
      redistribute(d, s);
    }
  }

 private:
  static constexpr int kRadixBits = 4;
  static constexpr int kRadix = 1 << kRadixBits;
  static constexpr Weight kDigitMask = kRadix - 1;
  static constexpr int kDigits = (64 + kRadixBits - 1) / kRadixBits;

  using Item = std::pair<Weight, Vertex>;

  static int most_significant_bit(Weight value) {
    return 63 - __builtin_clzll(value);
  }

  std::pair<int, int> bucket_of(Weight key) const {
    const Weight diff = key ^ base_;
    const int d = (diff == 0) ? 0 : (most_significant_bit(diff) / kRadixBits);
    const int s = static_cast<int>((key >> (d * kRadixBits)) & kDigitMask);
    return {d, s};
  }

  std::pair<int, int> first_nonempty_bucket() const {
    for (int d = 0; d < kDigits; ++d) {
      for (int s = 0; s < kRadix; ++s) {
        if (!buckets_[d][s].empty()) {
          return {d, s};
        }
      }
    }
    throw std::out_of_range("pop_min on empty two-level radix heap");
  }

  void redistribute(int d, int s) {
    const auto items = std::move(buckets_[d][s]);
    buckets_[d][s].clear();

    Weight new_base = std::numeric_limits<Weight>::max();
    for (const auto& item : items) {
      if (item.first < new_base) {
        new_base = item.first;
      }
    }
    base_ = new_base;

    for (const auto& item : items) {
      const auto [nd, ns] = bucket_of(item.first);
      buckets_[nd][ns].push_back(item);
    }
  }

  Weight base_ = 0;
  std::array<std::array<std::vector<Item>, kRadix>, kDigits> buckets_;
  std::size_t size_ = 0;
};

}  // namespace sssp
