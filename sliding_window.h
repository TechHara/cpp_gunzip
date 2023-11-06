#pragma once

#include <cstring>

#include "slice.h"

constexpr uint16_t MAX_DISTANCE = 1 << 15;
constexpr std::size_t WINDOW_SIZE = static_cast<std::size_t>(MAX_DISTANCE) * 3;

struct SlidingWindow {
  std::vector<uint8_t> data;
  std::size_t cur;

  explicit SlidingWindow() : data(WINDOW_SIZE, 0), cur{0} {}

  auto buffer() { return Slice{&data[0], &data[data.size()]}; }

  auto write_buffer() { return Slice{&data[cur], &data[data.size()]}; }

  void slide(std::size_t n) {
    auto end = cur + n;
    if (end > MAX_DISTANCE) {
      auto delta = end - MAX_DISTANCE;
      std::memmove(&data[0], &data[delta], end - delta);
      cur = MAX_DISTANCE;
    } else {
      cur = end;
    }
  }

  std::size_t boundary() const { return cur; }
};
