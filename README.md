This repo contains a C++ port of [`gunzip`](https://github.com/TechHara/gunzip) for decompression of `.gz` files. CRC32 checksum is perfomed using [Fast CRC32](https://create.stephan-brumme.com/crc32/) library.

# Benchmark
Below shows runtime comparison with other C/C++ gunzip implementations.

## Decompression of linux.tar.gz (Linux x64)
|  # Threads | GNU Gzip | Pigz | This |
|:-:|:-:|:-:|:-:|
| 1 | 4.93 | 3.26 | 2.70 |
| 2 | - | 2.35 | 2.13 |

## Decompression of linux.tar.gz (macOS arm64)
|  # Threads | GNU Gzip  | Pigz | This  |
|:-:|:-:|:-:|:-:|
| 1 | 7.19 | 2.64 | 4.44 |
| 2 | - | 2.78 | 3.22 |

# Build
```sh
$ cmake -Bbuild -DCMAKE_CXX_FLAGS="-O3 -g -DNDBUG"
$ make -Cbuild -j
```

# Run
```sh
# single thread
$ build/gunzip < compressed.gz > decompressed

# two threads
$ build/gunzip -t < compressed.gz > decompressed
```