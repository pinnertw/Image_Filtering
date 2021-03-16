#!/bin/bash


make >> /dev/stderr
cd images/test_heatmap
echo "Creating testing gif..." >> /dev/stderr
#python ./create_heatmap_gif.py
cd ../..
echo "Done!" >> /dev/stderr

INPUT_DIR=images/test_heatmap
OUTPUT_DIR=images/test_processed
mkdir $OUTPUT_DIR 2>/dev/null

for i in $INPUT_DIR/*gif ; do
    DEST=$OUTPUT_DIR/`basename $i .gif`-sobel.gif
    echo "Running test on $i -> $DEST" >> /dev/stderr

    # OpenMP, from 1 to 6 threads
    OMP_NUM_THREADS=6 ./sobelf $i $DEST 1
    export OMP_NUM_THREADS=1
    # Cuda
    salloc -n 1 mpirun ./sobelf $i $DEST 2

done
rm -f $INPUT_DIR/*.gif
