## Filo

[![Build Status](https://api.travis-ci.org/ECP-VeloC/filo.png?branch=master)](https://travis-ci.org/ECP-VeloC/filo)

This module uses AXL to transfer files between local storage and a global file system.
The transfer from local storage to global storage is called a "flush" and the reverse transfer is called a "fetch".
During a flush, filo associates each file with its owner MPI rank, and it records this association in a metadata file.
This metadata file is read during a "fetch" to copy each file back into local storage near its owner rank.
Filo also flow controls the number of MPI ranks reading or writing data at a time.
During a flush, filo creates destination directories as necessary.

For usage, see [src/filo.h](src/filo.h), [test/test\_filo.c](test/test_filo.c), and the [API User Docs](https://ecp-veloc.github.io/component-user-docs/group__filo.html).

# Building

To build dependencies:

    ./bootstrap.sh

To build filo:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../install ..
    make
    make install

# Testing
Some simple test programs exist in the test directory.

All the tests can be run with:

    make check

To build a test for the filo API:

    mpicc -g -O0 -o test_filo ../test/test_filo.c -I../install/include -L../install/lib64 -I../src -L../src -lkvtree -laxl -lspath -lfilo

## Release

Copyright (c) 2018, Lawrence Livermore National Security, LLC.
Produced at the Lawrence Livermore National Laboratory.
<br>
Copyright (c) 2018, UChicago Argonne LLC, operator of Argonne National Laboratory.


For release details and restrictions, please read the [LICENSE]() and [NOTICE]() files.

`LLNL-CODE-751725` `OCEC-18-060`
