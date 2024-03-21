# MOSER Linux build instructions

Moser itself is build using CMake.

note: x64 only!

## debian/ubuntu dependencies

```bash
sudo apt install build-essential cmake pkg-config libcurl4 nlohmann-json3-dev libffi-dev
```

for fedora deps see end of page.

## building with cmake
```bash
git clone moser
cd moser
cmake --preset gcc-release
chmake --build --preset gcc-release
``` 

## run test suite

```bash
cd test
python3 ../bin/testrunner.py
```

## how to run examples

examples for linux are in the linux subdirectory.

to run the curl example, for example, do this:

```bash
cd linux/curl
../../build/moser curl.msr
```

<details>
  <summary>fedora dependencies</summary>

```bash

sudo yum install git cmake joe libffi-devel c++ json-devel

sudo yum install libcurl-devel gtk3-devel gtksourceview3-devel gobject-introspection-devel

sudo yum install webkit2gtk4.0-devel
```

</details>

