#pragma once

#include <optional>

template <typename T>
struct Iterator {
  using Item = T;

  virtual std::optional<T> next() = 0;

  virtual ~Iterator() = default;
};
