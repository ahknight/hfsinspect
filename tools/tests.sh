#!/bin/bash

HFSINSPECT="$1"
IMAGE="$2"

test_cmd() {
    echo "---"
    echo "Running: $1"
    echo ""
    ( $1 ) || { echo "*** FAIL: $1"; exit 1; }
    echo "*** SUCCESS: $1"
}

test_cmd "${HFSINSPECT}"
test_cmd "${HFSINSPECT} -h"
test_cmd "${HFSINSPECT} -v"
test_cmd "${HFSINSPECT} -d ${IMAGE}"
test_cmd "${HFSINSPECT} -d ${IMAGE} -r"
test_cmd "${HFSINSPECT} -d ${IMAGE} -j"
test_cmd "${HFSINSPECT} -d ${IMAGE} -D"
test_cmd "${HFSINSPECT} -d ${IMAGE} -0"
test_cmd "${HFSINSPECT} -d ${IMAGE} -P / -l"
test_cmd "${HFSINSPECT} -d ${IMAGE} -b catalog"
test_cmd "${HFSINSPECT} -d ${IMAGE} -b catalog -n 1"
test_cmd "${HFSINSPECT} -d ${IMAGE} -b extents"
test_cmd "${HFSINSPECT} -d ${IMAGE} -b extents -n 1"
test_cmd "${HFSINSPECT} -d ${IMAGE} -b attributes"
test_cmd "${HFSINSPECT} -d ${IMAGE} -b attributes -n 1"
