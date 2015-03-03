#!/bin/sh

ACTIONS="distclean all test"

make $ACTIONS
vagrant ssh linux32 -- make -C hfsinspect $ACTIONS
vagrant ssh linux64 -- make -C hfsinspect $ACTIONS
