Unity scope template

This project contains a simple Unity Scope and all files needed to build
and deploy it. Building it is simple. First you need to install the scopes
development package and CMake. Then you go into the source root and type
the following commands.

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=debug ..
make
(make install)

The build system uses standard CMake conventions. See CMake's documentation
for further details.

