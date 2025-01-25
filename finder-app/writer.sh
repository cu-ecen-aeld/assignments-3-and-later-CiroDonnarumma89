#!/bin/bash

if [ $# -ne 2 ]
then
    echo "Usage:"
    echo "      ./writer.sh writefile writestr"
    exit 1
fi

directory=$( dirname $1 )

mkdir -p $directory

if [ $? -ne 0 ]
then
    echo "Unable to create the directory $directory"
    exit 1
fi

echo $2 > $1

if [ $? -ne 0 ]
then
    "Unable to create the file $1"
    exit 1
fi