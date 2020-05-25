#!/bin/bash
#
# This is an easy-bake script to download and build all FILO's external
# dependencies.  The dependencies will be built in filo/deps/ and
# installed to filo/install/
#
# Usage:
#
#   ./bootstrap.sh

ROOT="$(pwd)"

mkdir -p deps
mkdir -p install
INSTALL_DIR=$ROOT/install

cd deps

lwgrp=lwgrp-1.0.2
dtcmp=dtcmp-1.0.3

if [ ! -f ${lwgrp}.tar.gz ] ; then
  wget https://github.com/hpc/lwgrp/releases/download/v1.0.2/${lwgrp}.tar.gz
fi
if [ ! -f ${dtcmp}.tar.gz ] ; then
  wget https://github.com/hpc/dtcmp/releases/download/v1.0.3/${dtcmp}.tar.gz
fi

repos=(https://github.com/ECP-Veloc/KVTree.git
    https://github.com/ECP-Veloc/AXL.git
    https://github.com/ECP-Veloc/spath.git
)

for i in "${repos[@]}" ; do
	# Get just the name of the project (like "mercury")
	name=$(basename $i | sed 's/\.git//g')
	if [ -d $name ] ; then
		echo "$name already exists, skipping it"
	else
		git clone $i
	fi
done

rm -rf ${lwgrp}
tar -zxf ${lwgrp}.tar.gz
pushd ${lwgrp}
  ./configure CFLAGS="-g -O0" \
    --prefix=${INSTALL_DIR} && \
  make && \
  make install
  if [ $? -ne 0 ]; then
    echo "failed to configure, build, or install liblwgrp"
    exit 1
  fi
popd

rm -rf ${dtcmp}
tar -zxf ${dtcmp}.tar.gz
pushd ${dtcmp}
  ./configure CFLAGS="-g -O0" \
    --prefix=${INSTALL_DIR} \
    --with-lwgrp=${INSTALL_DIR} && \
  make && \
  make install
  if [ $? -ne 0 ]; then
    echo "failed to configure, build, or install libdtcmp"
    exit 1
  fi
popd

cd KVTree
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DMPI=ON ..
make -j `nproc`
make install
cd ../..

cd AXL
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DAXL_ASYNC_DAEMON=OFF -DMPI=ON ..
make -j `nproc`
make install
cd ../..

# spath
cd spath
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DMPI=ON ..
make -j `nproc`
make install
#make test

cd "$ROOT"

echo "*************************************************************************"
echo "Dependencies are all built.  You can now build FILO with:"
echo ""
echo "    mkdir -p build && cd build"
echo "    cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR ../"
echo "    make"
echo ""
echo "*************************************************************************"
