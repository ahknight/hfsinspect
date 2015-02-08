#!/bin/sh

ACTIONS="clean all test"

vagrant ssh linux32 -- make -C hfsinspect $ACTIONS
vagrant ssh linux64 -- make -C hfsinspect $ACTIONS
make $ACTIONS
