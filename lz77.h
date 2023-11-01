#pragma once

#include <optional>
#include <variant>

#include "iterator.h"

constexpr uint16_t MAX_LENGTH = 258;

constexpr uint32_t END_OF_BLOCK = 256;

constexpr std::pair<uint32_t, uint32_t> SYMBOL2BITS_LENGTH[] = {
    {0, 0},   {0, 3},   {0, 4},   {0, 5},   {0, 6},   {0, 7},
    {0, 8},   {0, 9},   {0, 10},  {1, 11},  {1, 13},  {1, 15},
    {1, 17},  {2, 19},  {2, 23},  {2, 27},  {2, 31},  {3, 35},
    {3, 43},  {3, 51},  {3, 59},  {4, 67},  {4, 83},  {4, 99},
    {4, 115}, {5, 131}, {5, 163}, {5, 195}, {5, 227}, {0, 258},
};

constexpr std::pair<uint32_t, uint32_t> SYMBOL2BITS_DISTANCE[] = {
    {0, 1},     {0, 2},     {0, 3},     {0, 4},      {1, 5},      {1, 7},
    {2, 9},     {2, 13},    {3, 17},    {3, 25},     {4, 33},     {4, 49},
    {5, 65},    {5, 97},    {6, 129},   {6, 193},    {7, 257},    {7, 385},
    {8, 513},   {8, 769},   {9, 1025},  {9, 1537},   {10, 2049},  {10, 3073},
    {11, 4097}, {11, 6145}, {12, 8193}, {12, 12289}, {13, 16385}, {13, 24577},
};
struct Literal {
  uint8_t x;

  explicit Literal(uint32_t symbol) : x{static_cast<uint8_t>(symbol)} {}
};

struct EndOfBlock {};

struct Dictionary {
  uint16_t distance, length;

  explicit Dictionary(uint32_t distance, uint32_t length)
      : distance{static_cast<uint16_t>(distance)},
        length{static_cast<uint16_t>(length)} {}
};

using Code = std::variant<Literal, EndOfBlock, Dictionary>;

struct DecodeResult {
  std::size_t n;
  bool done;

  static DecodeResult Done(std::size_t n) { return {n, true}; }

  static DecodeResult WindowIsFull(std::size_t n) { return {n, false}; }
};

template <typename Reader>
class CodeIterator {
 public:
  explicit CodeIterator(BitReader<Reader> &reader, HuffmanDecoder ll_decoder,
                        HuffmanDecoder dist_decoder)
      : reader_{reader},
        ll_decoder_{std::move(ll_decoder)},
        dist_decoder_{std::move(dist_decoder)} {}

  std::optional<Code> next() {
    auto bitcode = reader_.peek_bits();
    uint32_t symbol, len;
    std::tie(symbol, len) = ll_decoder_.decode(bitcode);
    reader_.consume(len);
    if (symbol < END_OF_BLOCK) {
      return Literal{symbol};
    } else if (symbol == END_OF_BLOCK) {
      return EndOfBlock{};
    } else {
      uint32_t bits, length;
      std::tie(bits, length) = SYMBOL2BITS_LENGTH[symbol & 0xFF];
      length += reader_.read_bits(bits);
      bitcode = reader_.peek_bits();
      std::tie(symbol, len) = dist_decoder_.decode(bitcode);
      reader_.consume(len);
      uint32_t distance;
      std::tie(bits, distance) = SYMBOL2BITS_DISTANCE[symbol];
      distance += reader_.read_bits(bits);
      return Dictionary{distance, length};
    }
  }

 private:
  BitReader<Reader> &reader_;
  HuffmanDecoder ll_decoder_, dist_decoder_;
};

template <typename Iter>
DecodeResult decode(Slice<uint8_t> window, std::size_t boundary, Iter &iter) {
  auto idx = boundary;
  if (idx + MAX_LENGTH >= window.size()) {
    return DecodeResult::WindowIsFull(idx - boundary);
  }

  for (;;) {
    std::optional<Code> iter_result = iter.next();
    if (!iter_result) break;
    switch (iter_result->index()) {
      case 0:  // Literal
        window[idx] = std::get<0>(*iter_result).x;
        ++idx;
        break;
      case 1:  // EndOfBlock
        return DecodeResult::Done(idx - boundary);
      case 2:  // Dictionary
        Dictionary dictionary = std::get<2>(*iter_result);
        if (dictionary.distance > idx) throw Error{ErrorType::DistanceTooMuch};
        auto begin = idx - dictionary.distance;
        while (dictionary.length > 0) {
          auto n =
              std::min<std::size_t>(dictionary.distance, dictionary.length);
          std::memmove(&window[idx], &window[begin], n);
          idx += n;
          dictionary.length -= n;
          dictionary.distance += n;
        }
    }
    if (idx + MAX_LENGTH >= window.size()) {
      return DecodeResult::WindowIsFull(idx - boundary);
    }
  }
  throw Error{ErrorType::EndOfBlockNotFound};
}
