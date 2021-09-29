#!/bin/bash

./build/cli/axi-bram write -b50000000 -d10000000 -o40 -l1 1
sleep 0.001
./build/cli/axi-bram write -b50000000 -d10000000 -o4004 -l1 1
sleep 0.001
./build/cli/axi-bram write -b50000000 -d10000000 -o4000 -l1 1
sleep 0.001
./build/cli/axi-bram write -b50000000 -d10000000 -o4000 -l1 0