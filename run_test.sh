#!/bin/bash

make

INPUT_DIR=images/original
OUTPUT_DIR=images/processed
mkdir $OUTPUT_DIR 2>/dev/null
export OMP_NUM_THREADS=6

for i in $INPUT_DIR/*gif ; do
    DEST=$OUTPUT_DIR/`basename $i .gif`-sobel.gif
    #echo "Running test on $i -> $DEST"

    ./sobelf $i $DEST 1
    ./sobelf $i $DEST 0
done
