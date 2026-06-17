#pragma once

#include <cstdint>
#include <queue>
#include <utility>
#include <vector>

#include "graph.hpp"

namespace sssp {

class BinaryHeapQueue {
 public:
  void push(Weight key, Vertex vertex) {
    heap_.push({key, vertex});
  }

  bool empty() const {
    return heap_.empty();
  }

  std::pair<Weight, Vertex> pop_min() {
    auto item = heap_.top();
    heap_.pop();
    return item;
  }

 private:
  using Item = std::pair<Weight, Vertex>;
  struct Greater {
    bool operator()(const Item& a, const Item& b) const {
      return a.first > b.first;
    }
  };

  std::priority_queue<Item, std::vector<Item>, Greater> heap_;
};

}  // namespace sssp
