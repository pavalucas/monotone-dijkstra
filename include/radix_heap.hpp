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

}  // namespace sssp
