## Filo
This module uses AXL to transfer files between local storage and a global file system.
The transfer from local storage to global storage is called a "flush" and the reverse transfer is called a "fetch".
During a flush, filo associates each file with its owner MPI rank, and it records this association in a metadata file.
This metadata file is read during a "fetch" to copy each file back into local storage near its owner rank.
Filo also flow controls the number of MPI ranks reading or writing data at a time.
During a flush, filo creates destination directories as necessary.

For usage, see [src/filo.h](src/filo.h) and [test/test\_filo.c](test/test_filo.c).

# Building

To build dependencies:

    git clone git@github.com:ECP-VeloC/KVTree.git  KVTree.git
    git clone git@github.com:ECP-VeloC/AXL.git     AXL.git
    git clone git@github.com:ECP-VeloC/spath.git   spath.git

    mkdir install

    mkdir build
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=../install -DMPI=ON ../KVTree.git
    make clean
    make
    make install
    make test
    cd ..

    rm -rf build
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../install ../AXL.git
    make clean
    make
    make install
    cd ..

    rm -rf build
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../install ../spath.git
    make clean
    make
    make install
    cd ..

To build filo:

    cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=./install -DWITH_KVTREE_PREFIX=`pwd`/install -DWITH_AXL_PREFIX=`pwd`/install .
    make
    make install

# Testing
Some simple test programs exist in the test directory.

To build a test for the filo API:

    mpicc -g -O0 -o test_filo test_filo.c -I../install/include -L../install/lib64 -I../src -L../src -lkvtree -laxl -lspath -lfilo

## Release

Copyright (c) 2018, Lawrence Livermore National Security, LLC.
Produced at the Lawrence Livermore National Laboratory.
<br>
Copyright (c) 2018, UChicago Argonne LLC, operator of Argonne National Laboratory.


For release details and restrictions, please read the [LICENSE]() and [NOTICE]() files.

`LLNL-CODE-751725` `OCEC-18-060`
