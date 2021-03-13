#!/bin/bash

make

INPUT_DIR=images/original
OUTPUT_DIR=images/processed
mkdir $OUTPUT_DIR 2>/dev/null
export OMP_NUM_THREADS=6

for i in $INPUT_DIR/*gif ; do
    DEST=$OUTPUT_DIR/`basename $i .gif`-sobel.gif
    #echo "Running test on $i -> $DEST"

    # MPI, from 1 node to 10 nodes
    for j in {1..6}
    do
        salloc -N 1 -n $j mpirun ./sobelf $i $DEST 3
    done

    # Cuda
    salloc -n 1 mpirun ./sobelf $i $DEST 2

    # OpenMP, from 1 to 6 threads
    for j in {1..6}
    do
        export OMP_NUM_THREADS=$j
        ./sobelf $i $DEST 1
    done
    
    # Sequentiel
    salloc -n 1 mpirun ./sobelf $i $DEST 0
done
