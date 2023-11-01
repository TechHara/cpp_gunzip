#pragma once

#include "error.h"

struct Footer {
  uint32_t crc32, size;
};

template <typename Reader>
Footer read_footer(Reader &reader) {
  uint8_t buf[8];
  auto gcount = reader.read(Slice{buf, std::end(buf)});
  if (gcount != 8) throw Error{ErrorType::UnexpectedEof};
  auto crc32 = reinterpret_cast<uint32_t *>(&buf[0]);
  auto size = reinterpret_cast<uint32_t *>(&buf[4]);
  return Footer{*crc32, *size};
}