#!/bin/bash
OUT=tmp/clang_static
mkdir -p "${OUT}"
scan-build -enable-checker security.insecureAPI.strcpy -o "${OUT}" make clean all
if [[ -x `which open` ]]
    then
    open "${OUT}"/`ls -1t "${OUT}"/ | head -1`/index.html
fi
