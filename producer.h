#pragma once

#include <variant>

#include "bitreader.h"
#include "codebook.h"
#include "footer.h"
#include "header.h"
#include "huffman_decoder.h"
#include "lz77.h"
#include "sliding_window.h"

enum struct State {
  Header,
  Block,
  Inflate,
  InflateFinalBlock,
  Footer,
};

using Produce = std::variant<Header, Footer, std::vector<uint8_t>>;

template <typename Read>
class Producer : public Iterator<Produce> {
 public:
  explicit Producer(Read &reader)
      : reader_{reader}, state_{State::Header}, member_idx_{0} {}

  std::optional<Produce> next() override {
    switch (state_) {
      case State::Header:
        if (!reader_.has_data_left()) {
          if (member_idx_ == 0) {
            throw Error{ErrorType::EmptyInput};
          }
          return std::nullopt;
        }
        state_ = State::Block;
        ++member_idx_;
        return read_header(reader_);
      case State::Block: {
        auto header = reader_.read_bits(3);
        auto is_final = (header & 1) == 1;

        switch (header & 0b110) {
          case 0b000:
            if (is_final) state_ = State::Footer;
            return inflate_block0();
          case 0b010:
            ll_decoder_ = HuffmanDecoder{Codebook::default_ll()};
            dist_decoder_ = HuffmanDecoder{Codebook::default_dist()};
            state_ = is_final ? State::InflateFinalBlock : State::Inflate;
            return inflate(is_final);
          case 0b100:
            std::tie(ll_decoder_, dist_decoder_) = read_dynamic_codebook();
            state_ = is_final ? State::InflateFinalBlock : State::Inflate;
            return inflate(is_final);
          default:
            throw Error{ErrorType::InvalidBlockType};
        }
      }
      case State::Inflate:
        return inflate(false);
      case State::InflateFinalBlock:
        return inflate(true);
      case State::Footer:
        state_ = State::Header;
        return read_footer(reader_);
      default:
        return std::nullopt;  // unreachable
    }
  }

 private:
  BitReader<Read> reader_;
  State state_;
  std::size_t member_idx_;
  SlidingWindow window_;
  HuffmanDecoder ll_decoder_;
  HuffmanDecoder dist_decoder_;

  Produce inflate_block0() {
    reader_.byte_align();
    auto len = reader_.read_bits(16);
    auto nlen = reader_.read_bits(16);
    if ((len ^ nlen) != 0xFFFF) {
      throw Error{ErrorType::BlockType0LenMismatch};
    }
    std::vector<uint8_t> buf(len, 0);
    auto gcount = reader_.read(Slice{buf});
    if (gcount != len) throw Error{ErrorType::UnexpectedEof};
    auto write_buffer = window_.write_buffer();
    std::copy(&buf[0], &buf[len], write_buffer.begin());
    window_.slide(len);
    return buf;
  }

  Produce inflate(bool is_final) {
    auto boundary = window_.boundary();
    auto result =
        decode(window_.buffer(), boundary, reader_, ll_decoder_, dist_decoder_);
    auto n = result.n;
    if (result.done) {
      state_ = is_final ? State::Footer : State::Block;
    }
    auto begin = window_.data.begin() + window_.cur;
    std::vector<uint8_t> buf(begin, begin + n);
    window_.slide(n);

    return buf;
  }

  std::pair<HuffmanDecoder, HuffmanDecoder> read_dynamic_codebook() {
    std::size_t hlit = reader_.read_bits(5) + 257;
    std::size_t hdist = reader_.read_bits(5) + 1;
    std::size_t hclen = reader_.read_bits(4) + 4;
    uint32_t cl_lengths[19];
    std::fill(&cl_lengths[0], &cl_lengths[18], 0);
    std::size_t indices[] = {
        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
    };
    for (std::size_t i = 0; i < hclen; ++i) {
      cl_lengths[indices[i]] = reader_.read_bits(3);
    }
    Codebook cl_codes{Slice{cl_lengths, std::end(cl_lengths)}};
    HuffmanDecoder cl_decoder{cl_codes};

    auto num_codes = hlit + hdist;
    std::vector<uint32_t> lengths;
    lengths.reserve(num_codes);
    while (lengths.size() < num_codes) {
      uint32_t cl_code, len;
      try {
        std::tie(cl_code, len) = cl_decoder.decode(reader_.peek_bits());
      } catch (std::exception &e) {
        throw Error{ErrorType::ReadDynamicCodebook};
      }
      reader_.consume(len);
      std::size_t length;
      switch (cl_code) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
          lengths.push_back(cl_code);
          break;
        case 16: {
          assert(!lengths.empty());
          length = 3 + reader_.read_bits(2);
          auto x = lengths.back();
          lengths.resize(lengths.size() + length, x);
          break;
        }
        case 17:
          length = 3 + reader_.read_bits(3);
          lengths.resize(lengths.size() + length, 0);
          break;
        case 18:
          length = 11 + reader_.read_bits(7);
          lengths.resize(lengths.size() + length, 0);
          break;
        default:
          throw Error{ErrorType::ReadDynamicCodebook};
      }
    }

    if (lengths.size() != num_codes)
      throw Error{ErrorType::ReadDynamicCodebook};
    Codebook ll_codes{Slice{&lengths[0], &lengths[hlit]}};
    Codebook dist_codes{Slice{&lengths[hlit], &lengths[lengths.size()]}};
    return std::make_pair(HuffmanDecoder{ll_codes}, HuffmanDecoder{dist_codes});
  }
};