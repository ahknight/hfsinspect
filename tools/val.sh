#!/bin/bash
BIN=build/`uname -s`-`uname -m`/hfsinspect
make ${BIN}
if [[ -z "$@" ]]; then
    ARGS="-d images/test.img -lP /"
else
    ARGS="$@"
fi

valgrind --leak-check=full --error-limit=no --track-origins=yes "$BIN" $ARGS
