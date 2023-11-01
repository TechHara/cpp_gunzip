#pragma once

#include <cstdio>
#include <iostream>
#include <limits>
#include <vector>

#include "error.h"
#include "slice.h"

/**
 * These are implicit interfaces that will used in the library
 *
 * read n-bytes or until EOF. Throws on error.
 * std::size_t read(Slice<uint8_t> buf);
 *
 * read until byte is read or EOF. Throws on error.
 * std::size_t read_until(uint8_t byte, std::vector<uint8_t> &buf_);
 *
 * write n-bytes. Throws on error.
 * void write(Slice<uint8_t> buf);
 */

// wrapper around stdin
struct Stdin {
  std::size_t read(Slice<uint8_t> buf) {
    auto result = fread(buf.begin(), 1, buf.size(), stdin);
    if (ferror(stdin)) throw Error{ErrorType::StdIoError};
    return result;
  }
};

// wrapper around cin
struct Cin {
  std::size_t read(Slice<uint8_t> buf) {
    if (buf.size() > std::numeric_limits<std::streamsize>::max())
      throw Error{ErrorType::SizeTooLarge};

    auto result =
        std::cin.read(reinterpret_cast<char*>(buf.begin()), buf.size())
            .gcount();
    return result;
  }
};

// wrapper around stdout
struct Stdout {
  void write(Slice<uint8_t> buf) {
    fwrite(buf.begin(), 1, buf.size(), stdout);
    if (ferror(stdout)) throw Error{ErrorType::StdIoError};
  }
};

// wrapper around cout
struct Cout {
  void write(Slice<uint8_t> buf) {
    if (buf.size() > std::numeric_limits<std::streamsize>::max())
      throw Error{ErrorType::SizeTooLarge};

    std::cout.write(reinterpret_cast<char*>(buf.begin()), buf.size());
  }
};
