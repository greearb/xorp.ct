# CMake build for xorp
===


# Building
===

CMake need to be called twice, because `file(GLOB ...` would ommit files that doesn't exist yet for some targets.

```
mkdir build
cd build
cmake ..
make targets interfaces
cmake ..
make -j$(nproc)
```
Main binary is build in `./rtrmgr/rtrmgr` location.
Built on Ubuntu 20.04.3 with setup:
* CMake 3.16.3
* GNU Make 4.2.1
* GNU GCC 9.3.0
* GNU G++ 9.3.0
* GNU Bison 3.5.1
* Glibc 2.31
* Linux kernel 5.14.x

# TODO
===

* Fix generating targets based on interfaces and targets
* CMake for all components
* readdir in libtecla (readdir_r deprecated since glibc 2.19)
* Tests
* Tools
* Copy template directory
* Install targets
