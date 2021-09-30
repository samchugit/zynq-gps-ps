#!/bin/bash

./build/cli/axi-bram write -b50000000 -d10000000 -o10 -l1 0
./build/cli/axi-bram write -b50000000 -d10000000 -o1001 -l1 0
./build/cli/axi-bram write -b50000000 -d10000000 -o1000 -l1 0