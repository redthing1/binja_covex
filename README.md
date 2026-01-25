
# binja_covex

Binary Ninja coverage explorer plugin for analyzing code coverage.

## build

```sh
# configure
cmake -G Ninja -B build-release -DCMAKE_BUILD_TYPE=Release -DBINJA_API_VERSION=<commit_hash> -DBINJA_QT_VERSION=<qt_ver>
# build
cmake --build build-release --parallel
```

on macos, auto-configure:
```sh
./scripts/configure_mac.py
```

## usage

load coverage via the CovEx sidebar or `Plugins > CovEx > Load Coverage`.

### composition expressions

traces are assigned aliases A, B, C, ...

operators:
- union: `|`
- intersection: `&`
- subtraction: `-`
- parentheses: `( )`

examples:
- `A | B`
- `A - B`
- `(A | B) & C`

### block filter

filter expression supports simple space-separated tokens:
- `addr:0x401000`
- `hits>=10`
- `size>=16`
- `func:memcpy`
