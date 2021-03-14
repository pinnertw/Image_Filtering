#!/bin/bash

make >> /dev/stderr
cd images/test
echo "Creating testing gif..." >> /dev/stderr
python ./create_gif.py $1 $2
if [ $? -eq 1 ]; then
    exit 1
fi
cd ../..
echo "Done!" >> /dev/stderr
