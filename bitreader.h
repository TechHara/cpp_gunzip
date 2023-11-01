#pragma once

#include <algorithm>
#include <cassert>
#include <cstring>
#include <vector>

#include "io.h"

template <typename Read>
class BitReader {
 public:
  explicit BitReader(Read &reader)
      : reader_{reader}, buf_(BUFFER_SIZE), nbits_{0}, begin_{0}, cap_{0} {}

  uint32_t peek_bits() {
    while (cap_ - begin_ < sizeof(uint32_t)) {
      if (fill_buf() == 0) throw Error{ErrorType::UnexpectedEof};
    }
    auto bits = reinterpret_cast<uint32_t *>(&buf_[begin_]);
    return (*bits) >> nbits_;
  }

  void consume(uint32_t n) noexcept {
    assert(n <= bit_len());
    nbits_ += n;
    begin_ += (nbits_ / 8);
    nbits_ %= 8;
  }

  void byte_align() noexcept {
    if (nbits_ > 0) {
      nbits_ = 0;
      ++begin_;
    }
  }

  uint32_t read_bits(uint32_t n) {
    assert(n <= 24);
    auto bits = peek_bits();
    consume(n);
    return bits & ((1 << n) - 1);
  }

  bool has_data_left() { return cap_ > begin_ || fill_buf() != 0; }

  std::size_t read(Slice<uint8_t> buf) {
    byte_align();
    auto len = std::min(buf.size(), cap_ - begin_);
    std::copy(&buf_[begin_], &buf_[begin_ + len], buf.begin());
    begin_ += len;

    len += reader_.read(Slice{buf.begin() + len, buf.end()});
    return len;
  }

  std::size_t read_until(uint8_t byte, std::vector<uint8_t> &buf) {
    byte_align();
    std::size_t n = 0;
    for (;;) {
      auto it = std::find(&buf_[begin_], &buf_[cap_], byte);
      if (it == &buf_[cap_]) {
        buf.insert(buf.end(), &buf_[begin_], &buf_[cap_]);
        n += cap_ - begin_;
        if (fill_buf() == 0) return n;
      } else {
        buf.insert(buf.end(), &buf_[begin_], it + 1);
        auto pos = it - &buf_[begin_];
        n += pos + 1;
        begin_ += pos + 1;
        return n;
      }
    }
  }

 private:
  Read &reader_;
  std::vector<uint8_t> buf_;
  uint32_t nbits_;
  std::size_t begin_, cap_;
  static constexpr std::size_t BUFFER_SIZE = 16 << 10;

  std::size_t bit_len() const noexcept { return (cap_ - begin_) * 8 - nbits_; }

  std::size_t fill_buf() {
    std::memmove(&buf_[0], &buf_[begin_], cap_ - begin_);
    cap_ -= begin_;
    begin_ = 0;
    auto n = reader_.read(Slice{&buf_[cap_], &buf_[buf_.size()]});
    cap_ += n;
    return n;
  }
};
