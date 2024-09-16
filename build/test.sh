#!/bin/bash

# Run CMake
cmake ..

make
# Build the project and run tests
make test

# Run additional checks if available
make check
