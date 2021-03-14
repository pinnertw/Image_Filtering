#!/bin/bash


./dealing_input.sh $1 $2
if [ $? -eq 1 ]; then
    exit 1
fi
INPUT_DIR=images/test
OUTPUT_DIR=images/test_processed
mkdir $OUTPUT_DIR 2>/dev/null

for i in $INPUT_DIR/*gif ; do
    DEST=$OUTPUT_DIR/`basename $i .gif`-sobel.gif
    echo "Running test on $i -> $DEST" >> /dev/stderr

    # Sequentiel
    salloc -n 1 mpirun ./sobelf $i $DEST 0

    # OpenMP, from 1 to 6 threads
    for j in {1..6}
    do
        OMP_NUM_THREADS=$j salloc -n 1 mpirun ./sobelf $i $DEST 1
    done
    export OMP_NUM_THREADS=1

    # Cuda
    salloc -n 1 mpirun ./sobelf $i $DEST 2

    # MPI, from 1 node to 6 nodes
    for j in {1..6}
    do
        salloc -N 1 -n $j mpirun ./sobelf $i $DEST 3
    done

    # MPI+OMP, from 1 node to 6 nodes
    for j in {1..6}
    do
        OMP_NUM_THREADS=6 salloc -N 1 -n $j mpirun ./sobelf $i $DEST 4
    done

    # MPI+Cuda, from 1 node to 6 nodes
    for j in {1..6}
    do
        salloc -N 1 -n $j mpirun ./sobelf $i $DEST 5
    done
done
rm -f $INPUT_DIR/*.gif
