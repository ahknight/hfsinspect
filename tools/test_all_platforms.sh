#!/bin/sh

vagrant ssh linux32 -- make -C hfsinspect clean all test
vagrant ssh linux64 -- make -C hfsinspect clean all test
make clean all test
