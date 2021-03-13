#!/bin/bash

make
cd images/test
echo "Creating testing gif..."
python ./create_gif.py
cd ../..
echo "Done!"

INPUT_DIR=images/test
OUTPUT_DIR=images/test_processed
mkdir $OUTPUT_DIR 2>/dev/null

for i in $INPUT_DIR/*gif ; do
    DEST=$OUTPUT_DIR/`basename $i .gif`-sobel.gif
    #echo "Running test on $i -> $DEST"

    # Sequentiel
    salloc -n 1 mpirun ./sobelf $i $DEST 0

    # OpenMP, from 1 to 6 threads
    for j in {1..6}
    do
        OMP_NUM_THREADS=$j ./sobelf $i $DEST 1
    done
    export OMP_NUM_THREADS=1

    # Cuda
    salloc -n 1 ./sobelf $i $DEST 2

    # MPI, from 1 node to 6 nodes
    for j in {1..6}
    do
        salloc -N $j -n $j mpirun ./sobelf $i $DEST 3
    done

    # MPI+OMP, from 1 node to 6 nodes
    for j in {1..6}
    do
        salloc -N $j -n $j mpirun ./sobelf $i $DEST 4
    done

    # MPI+Cuda, from 1 node to 6 nodes
    for j in {1..6}
    do
        salloc -N $j -n $j mpirun ./sobelf $i $DEST 5
    done
done
