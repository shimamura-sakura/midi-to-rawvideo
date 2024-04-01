#!/bin/sh

gcc -Wall -Wextra \
  main.c main-{video,audio,receiver}.c file-load.c midi.c queu.c tckk.c bars.c draw.c \
  -I./include-bass -L./lib64-bass \
  -lbass -lbassenc -lbassmidi -lbass_fx -lpthread \
  -o main -Ofast -flto
