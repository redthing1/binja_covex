
# binja_covex

binaryninja coverage explorer plugin for analyzing code coverage!

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
