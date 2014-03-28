#!/bin/bash
sudo make install
valgrind --leak-check=full --error-limit=no --track-origins=yes hfsinspect -d images/test.img -lP /
