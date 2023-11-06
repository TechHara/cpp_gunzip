#pragma once

#include <vector>

#include "codebook.h"

constexpr uint32_t NUM_BITS_FIRST_LOOKUP = 9;

class HuffmanDecoder {
 public:
  // uninitialized
  HuffmanDecoder() : primary_mask_{0}, secondary_mask_{0} {}

  explicit HuffmanDecoder(Codebook const& codebook) {
    auto max_nbits = codebook.max_length();
    uint32_t nbits;
    if (max_nbits > NUM_BITS_FIRST_LOOKUP) {
      nbits = NUM_BITS_FIRST_LOOKUP;
      secondary_mask_ = (1 << (max_nbits - NUM_BITS_FIRST_LOOKUP)) - 1;
    } else {
      nbits = max_nbits;
      secondary_mask_ = 0;
    }
    primary_mask_ = (1 << nbits) - 1;

    lookup_.resize(1 << nbits, std::make_pair(0, 0));
    for (uint32_t symbol = 0; symbol < codebook.size(); ++symbol) {
      uint32_t bitcode, length;
      std::tie(bitcode, length) = codebook[symbol];
      if (length == 0) continue;

      bitcode = reverse_bits(bitcode);
      bitcode >>= 16 - length;  // right-align
      if (length <= nbits) {
        auto delta = nbits - length;
        for (uint32_t idx = 0; idx < (static_cast<uint32_t>(1) << delta);
             ++idx) {
          lookup_[(bitcode | (idx << length))] = {symbol, length};
        }
      } else {
        std::size_t base = bitcode & primary_mask_;
        uint32_t offset;
        if (lookup_[base].first == 0) {
          offset = lookup_.size();
          lookup_[base] = {offset, length};
          auto new_len = lookup_.size() + (1 << (max_nbits - nbits));
          lookup_.resize(new_len, {0, 0});
        } else {
          offset = lookup_[base].first;
        }
        auto secondary_len = length - nbits;
        base = offset + ((bitcode >> nbits) & secondary_mask_);
        for (std::size_t idx = 0;
             idx < (static_cast<uint32_t>(1) << (max_nbits - length)); ++idx) {
          lookup_[base + (idx << secondary_len)] = {symbol, length};
        }
      }
    }
  }

  std::pair<uint32_t, uint32_t> decode(uint32_t bits) const {
    uint32_t symbol, length;
    std::tie(symbol, length) = lookup_[bits & primary_mask_];
    if (length == 0) throw Error{ErrorType::HuffmanDecoderCodeNotFound};
    if (length <= NUM_BITS_FIRST_LOOKUP) {
      return {symbol, length};
    }
    std::size_t base = symbol;
    auto idx = (bits >> NUM_BITS_FIRST_LOOKUP) & secondary_mask_;
    return lookup_[base + idx];
  }

 private:
  std::vector<std::pair<uint32_t, uint32_t>> lookup_;
  uint32_t primary_mask_, secondary_mask_;

  // declared as static so that we can define it in the header file
  static uint32_t reverse_bits(uint32_t bits) {
    bits = (bits & 0xFF00) >> 8 | (bits & 0x00FF) << 8;
    bits = (bits & 0xF0F0) >> 4 | (bits & 0x0F0F) << 4;
    bits = (bits & 0xCCCC) >> 2 | (bits & 0x3333) << 2;
    bits = (bits & 0xAAAA) >> 1 | (bits & 0x5555) << 1;
    return bits;
  }
};
