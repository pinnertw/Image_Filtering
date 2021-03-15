#!/bin/bash

make >> /dev/stderr
cd ../images/test_single_image
echo "Creating testing gif..." >> /dev/stderr
python ./create_gif.py $1 $2
if [ $? -eq 1 ]; then
    exit 1
fi
cd ../..
echo "Done!" >> /dev/stderr

INPUT_DIR=images/test_single_image
OUTPUT_DIR=images/test_processed
mkdir $OUTPUT_DIR 2>/dev/null

for i in $INPUT_DIR/*gif ; do
    DEST=$OUTPUT_DIR/`basename $i .gif`-sobel.gif
    #echo "Running test on $i -> $DEST"

    # Sequentiel
    salloc -n 1 mpirun ./sobelf $i $DEST 0

    # MPI, from 1 node to 6 nodes
    for j in {1..3}
    do
        salloc -N $j -n $j mpirun ./sobelf $i $DEST 3
    done

    # MPI+OMP, from 1 node to 6 nodes
    for j in {1..3}
    do
        OMP_NUM_THREADS=6 salloc -N $j -n $j mpirun ./sobelf $i $DEST 4
    done

    # MPI+Cuda, from 1 node to 6 nodes
    for j in {1..3}
    do
        salloc -N $j -n $j mpirun ./sobelf $i $DEST 5
    done
done
rm -f $INPUT_DIR/*.gif
