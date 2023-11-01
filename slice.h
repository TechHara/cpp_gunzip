#pragma once

#include <vector>

template <typename T>
class Slice {
 public:
  explicit Slice(T* begin, T* end) : begin_{begin}, end_{end} {}

  explicit Slice(std::vector<T>& xs) : begin_{&xs[0]}, end_{&xs[xs.size()]} {}

  std::size_t size() const { return end_ - begin_; }

  bool empty() const { return size() == 0; }

  T& operator[](std::size_t idx) { return *(begin_ + idx); }

  auto begin() const { return begin_; }

  auto end() const { return end_; }

 private:
  T* begin_;
  T* end_;
};