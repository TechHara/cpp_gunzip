#include <cstring>
#include <iostream>

#include "decompressor.h"
#include "io.h"

int usage(std::string const& program) {
  std::cerr << "usage: " << program << " [-t]\n";
  std::cerr
      << "\tDecompresses .gz file read from stdin and outputs to stdout\n";
  std::cerr << "\t-t: employ two threads\n";
  std::cerr << "\tExample: " << program << " < input.gz > output\n";
  return -1;
}

constexpr std::streamsize BUFFER_SIZE = 4 << 10;

int main(int argc, const char** argv) {
  std::ios_base::sync_with_stdio(false);

  bool multithread = false;
  if (argc == 2 && std::strcmp("-t", argv[1]) == 0) {
    multithread = true;
  } else if (argc != 1) {
    return usage(argv[0]);
  }

  Stdin in;
  Stdout out;
  Decompressor decompressor{in, multithread};
  std::vector<uint8_t> buffer(BUFFER_SIZE, 0);
  Slice buf{buffer};
  while (true) {
    auto n = decompressor.read(buf);
    out.write(Slice{buf.begin(), buf.begin() + n});
    if (n < BUFFER_SIZE) break;
  }
  return 0;
}
