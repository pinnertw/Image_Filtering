#!/bin/bash

make

INPUT_DIR=images/demo
OUTPUT_DIR=images/processed
mkdir $OUTPUT_DIR 2>/dev/null

export OMP_NUM_THREADS=6

echo "1.gif : 10x500x500"
echo "Campusplan : 1x10000x7000"
echo "Mandelbrot : 20x1200x1200"

echo "3 machine 18 process, MPI"
time {
    for i in $INPUT_DIR/*gif ; do
        DEST=$OUTPUT_DIR/`basename $i .gif`.gif
        echo -n "$i "
        salloc -N 3 -n 18 mpirun ./sobelf $i $DEST 3 2> /dev/null
    done
}

echo "3 machine 18 process, Merge"
time {
    for i in $INPUT_DIR/*gif ; do
        DEST=$OUTPUT_DIR/`basename $i .gif`.gif
        echo -n "$i "
        salloc -N 3 -n 18 mpirun ./sobelf $i $DEST 6 2> /dev/null
    done
}

echo "1 machine 6 threads, OpenMP"
time {
    for i in $INPUT_DIR/*gif ; do
        DEST=$OUTPUT_DIR/`basename $i .gif`.gif
        echo -n "$i "
        ./sobelf $i $DEST 1 2> /dev/null
    done
}
