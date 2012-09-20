#!/bin/bash

rm ./snake; gcc -D_DEBUG -Wall ./snake.c -lncurses -lpthread -o ./snake && ./snake


