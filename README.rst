====================
CMake build for xorp
====================


Building
========

.. :code-block:
    mkdir build
    cd build
    cmake ..
    make -j$(nproc)


Basic build setup on Ubuntu 20.04.3 with setup:

* CMake 3.16.3

* GNU Make 4.2.1

* GNU GCC 9.3.0

* GNU G++ 9.3.0

* GNU Bison 3.5.1

* Glibc 2.31

* Linux kernel 6.2.x

TODO List
=========

* readdir in libtecla (readdir_r deprecated since glibc 2.19)

* Tests

