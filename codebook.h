#pragma once

constexpr uint32_t MAX_CODELENGTH = 15;
constexpr uint32_t MAX_LL_SYMBOL = 288;

class Codebook {
 public:
  static Codebook default_ll() {
    uint32_t lengths[] = {
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
        9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8,
    };

    return Codebook{Slice{lengths, std::end(lengths)}};
  }

  static Codebook default_dist() {
    uint32_t lengths[30];
    std::fill(lengths, std::end(lengths), 5);
    return Codebook{Slice{lengths, std::end(lengths)}};
  }

  explicit Codebook(Slice<uint32_t> lengths) {
    if (lengths.empty() || lengths.size() > MAX_LL_SYMBOL + 1) {
      throw Error{ErrorType::InvalidCodeLengths};
    }

    tree_.reserve(lengths.size());
    max_length_ = 0;

    // step 1
    uint32_t bl_count[MAX_CODELENGTH + 1];
    std::fill(bl_count, &bl_count[MAX_CODELENGTH + 1], 0);
    for (unsigned int length : lengths) {
      ++bl_count[length];
      tree_.emplace_back(0, length);
      max_length_ = std::max(max_length_, length);
    }
    if (max_length_ > MAX_CODELENGTH)
      throw Error{ErrorType::InvalidCodeLengths};

    // step 2
    uint32_t next_code[MAX_CODELENGTH + 1];
    uint32_t code = 0;
    bl_count[0] = 0;
    for (uint32_t bits = 1; bits <= max_length_; ++bits) {
      code = (code + bl_count[bits - 1]) << 1;
      next_code[bits] = code;
    }

    // step 3
    for (auto& pair : tree_) {
      std::size_t length = pair.second;
      if (length != 0) {
        pair.first = next_code[length];
        ++next_code[length];
      }
    }
  }

  uint32_t max_length() const { return max_length_; }

  std::size_t size() const { return tree_.size(); }

  std::pair<uint32_t, uint32_t> const& operator[](std::size_t idx) const {
    return tree_[idx];
  }

 private:
  std::vector<std::pair<uint32_t, uint32_t>> tree_;  // bitcode, length
  uint32_t max_length_;
};