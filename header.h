#pragma once

#include <optional>
#include <vector>

#include "error.h"
#include "slice.h"

constexpr uint8_t ID1 = 0x1f;
constexpr uint8_t ID2 = 0x8b;
constexpr uint8_t DEFLATE = 8;
constexpr uint8_t FTEXT = 1;
constexpr uint8_t FHCRC = 2;
constexpr uint8_t FEXTRA = 4;
constexpr uint8_t FNAME = 8;
constexpr uint8_t FCOMMENT = 16;

struct Header {
  uint8_t head[10];
  std::optional<std::vector<uint8_t>> extra_field;
  std::optional<std::vector<uint8_t>> name;
  std::optional<std::vector<uint8_t>> comment;
  std::optional<uint16_t> crc16;
  std::size_t size;  // head size
};

template <typename Reader>
Header read_header(Reader &reader) {
  Header header;

  // Read the head
  auto gcount = reader.read(Slice{header.head, std::end(header.head)});
  if (gcount != 10) throw Error{ErrorType::UnexpectedEof};
  header.size = 10;

  // Check the head ID
  if (header.head[0] != ID1 || header.head[1] != ID2 ||
      header.head[2] != DEFLATE) {
    throw Error{ErrorType::InvalidGzHeader};
  }

  // Get the head flags
  uint8_t flags = header.head[3];

  // Read the extra field, if present
  if ((flags & FEXTRA) != 0) {
    uint16_t extra_field_length;
    auto begin = reinterpret_cast<uint8_t *>(&extra_field_length);
    gcount = reader.read(Slice{begin, begin + sizeof(uint16_t)});
    if (gcount != 2) throw Error{ErrorType::UnexpectedEof};
    header.size += gcount;

    std::vector<uint8_t> buf(extra_field_length, 0);
    gcount = reader.read(Slice{buf});
    if (gcount != extra_field_length) throw Error{ErrorType::UnexpectedEof};
    header.size += gcount;
    header.extra_field = std::move(buf);
  }

  // Read the file name, if present
  if ((flags & FNAME) != 0) {
    std::vector<uint8_t> buf;
    header.size += reader.read_until('\0', buf);
    header.name = std::move(buf);
  }

  // Read the comment, if present
  if ((flags & FCOMMENT) != 0) {
    std::vector<uint8_t> buf;
    header.size += reader.read_until('\0', buf);
    header.comment = std::move(buf);
  }

  // Read the CRC16, if present
  if ((flags & FHCRC) != 0) {
    uint16_t crc16;
    auto begin = reinterpret_cast<uint8_t *>(&crc16);
    gcount = reader.read(Slice{begin, begin + 2});
    if (gcount != 2) throw Error{ErrorType::UnexpectedEof};
    header.crc16 = crc16;
  }

  return header;
}
