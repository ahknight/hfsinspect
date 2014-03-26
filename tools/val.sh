#!/bin/bash
sudo make install
valgrind --leak-check=yes --error-limit=no --track-origins=yes hfsinspect -d images/test.img -lP /
