#!/bin/bash
cd kernobj
rm -f linux-headers
ln -s /usr/src/linux-headers-`uname -r` linux-headers
make
cd ..
cd lib
make
cd ..

