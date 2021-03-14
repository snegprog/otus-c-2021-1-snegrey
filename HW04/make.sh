#!/bin/bash
gcc -Wall -Wextra -Wpedantic -std=c11 `pkg-config --cflags glib-2.0`  -c *.c
gcc *.o `pkg-config --libs glib-2.0` -Llib/cJSON -lcJSON -lcurl -o ./bin/main
rm *.o