This repo contains a C++ port of [gunzip](https://github.com/TechHara/gunzip) for decompression of `.gz` files. By default, it relies on [Fast CRC32](https://create.stephan-brumme.com/crc32/) for CRC32 computation but can be switched to use [zlib](https://www.zlib.net) instead.

# Benchmark
Below shows runtime comparison with other C/C++ gunzip implementations.

## Decompression of linux.tar.gz (Linux x64)
|  # Threads | GNU Gzip | Pigz | This |
|:-:|:-:|:-:|:-:|
| 1 | 4.93 | 3.26 | 2.81 |
| 2 | - | 2.35 | 1.87 |

## Decompression of linux.tar.gz (macOS arm64)
|  # Threads | GNU Gzip  | Pigz | This  |
|:-:|:-:|:-:|:-:|
| 1 | 7.19 | 2.64 | 3.44 |
| 2 | - | 2.78 | 2.89 |

# Build
```sh
# Linux
$ cmake -Bbuild -DCMAKE_CXX_FLAGS="-O3 -g -DNDEBUG"
# macOS
$ cmake -Bbuild -DCMAKE_CXX_FLAGS="-O3 -g -DNDEBUG -Wl,-ld_classic -D__BYTE_ORDER=1234" -DCMAKE_CXX_COMPILER=g++-13
$ make -Cbuild -j
```

# Run
```sh
# single thread
$ build/gunzip < compressed.gz > decompressed

# two threads
$ build/gunzip -t < compressed.gz > decompressed
```
