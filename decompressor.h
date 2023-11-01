#pragma once

#include <memory>
#include <thread>

#include "Crc32.h"
#include "channel.h"
#include "iterator.h"
#include "producer.h"

template <typename Read>
class Decompressor {
 public:
  explicit Decompressor(Read &reader, bool multithread)
      : begin_{0}, crc32_{0}, size_{0} {
    if (multithread) {
      Producer producer{reader};
      auto [tx, rx] = make_channel<Produce>();
      std::thread t{[](Channel<Produce> tx, Producer<Read> producer) {
                      for (;;) {
                        auto produce = producer.next();
                        if (!produce) break;
                        tx.send(std::move(*produce));
                      }
                    },
                    std::move(tx), std::move(producer)};
      thread_ = std::move(t);
      iterator_ = std::make_unique<Channel<Produce>>(std::move(rx));
    } else {
      iterator_ = std::make_unique<Producer<Read>>(reader);
    }
  }

  ~Decompressor() {
    if (thread_) {
      thread_->join();
    }
  }

  std::size_t read(Slice<uint8_t> buf) {
    std::size_t nbytes = 0;
    for (;;) {
      auto n = std::min(buf.size(), buf_.size() - begin_);
      std::copy(&buf_[begin_], &buf_[begin_ + n], buf.begin());
      buf = Slice{buf.begin() + n, buf.end()};
      nbytes += n;
      begin_ += n;
      if (buf.empty() || fill_buf() == 0) break;
    }
    return nbytes;
  }

 private:
  std::unique_ptr<Iterator<Produce>> iterator_;
  std::vector<uint8_t> buf_;
  std::size_t begin_;
  uint32_t crc32_;
  uint32_t size_;
  std::optional<std::thread> thread_;

  std::size_t fill_buf() {
    for (;;) {
      auto iter_result = iterator_->next();
      if (!iter_result) return 0;
      switch (iter_result->index()) {
        case 0:    // Header
          break;   // nothing to do
        case 1: {  // Footer
          auto &footer = std::get<1>(*iter_result);
          if (crc32_ != footer.crc32) throw Error{ErrorType::ChecksumMismatch};
          if (size_ != footer.size) throw Error{ErrorType::SizeMismatch};
          crc32_ = 0;
          size_ = 0;
          break;
        }
        case 2: {  // Data
          auto &xs = std::get<2>(*iter_result);
          if (xs.empty()) continue;
          crc32_ = crc32_fast(&xs[0], xs.size(), crc32_);
          size_ += xs.size();
          buf_ = std::move(xs);
          begin_ = 0;
          return buf_.size();
        }
      }
    }
  }
};