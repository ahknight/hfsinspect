#!/bin/bash
OUT=tmp/clang_static
mkdir -p "${OUT}"
make clean
scan-build \
	-enable-checker security.insecureAPI.strcpy \
	-enable-checker alpha.core.BoolAssignment \
	-enable-checker alpha.core.CastSize \
	-enable-checker alpha.core.FixedAddr \
	-enable-checker alpha.core.IdenticalExpr \
	-enable-checker alpha.core.PointerArithm \
	-enable-checker alpha.core.PointerSub \
	-enable-checker alpha.core.SizeofPtr \
	-enable-checker alpha.core.TestAfterDivZero \
	-enable-checker alpha.deadcode.UnreachableCode \
	-enable-checker alpha.security.ArrayBoundV2 \
	-enable-checker alpha.security.MallocOverflow \
	-enable-checker alpha.security.ReturnPtrRange \
	-enable-checker alpha.security.taint.TaintPropagation \
	-enable-checker alpha.unix.MallocWithAnnotations \
	-enable-checker alpha.unix.SimpleStream \
	-enable-checker alpha.unix.Stream \
	-enable-checker alpha.unix.cstring.BufferOverlap \
	-enable-checker alpha.unix.cstring.NotNullTerminated \
	-enable-checker alpha.unix.cstring.OutOfBounds \
	-enable-checker security.insecureAPI.rand \
	-enable-checker security.insecureAPI.strcpy \
	-o "${OUT}" make
if [[ -x `which open` ]]
    then
    open "${OUT}"/`ls -1t "${OUT}"/ | head -1`/index.html
fi
