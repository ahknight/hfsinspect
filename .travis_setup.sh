#!/bin/bash

UNAME=`uname -s`

# Output Directories
mkdir -p "bin/$UNAME"

# Ubuntu Prerequisites
if [[ $UNAME = 'Linux' ]]
    then
    sudo apt-get install uuid-dev
fi
