#!/bin/sh

gcc-12 -o configure.bin configure.c
./configure.bin

echo \#define HOST_PLATFORM \"`uname -o -i -r`\" >> ../src/conf.h.inc

echo \#   define HOST_COMPILER \"`gcc-12 --version|head -1`\" >> ../src/conf.h.inc

echo \#ifdef __cplusplus >> ../src/conf.h.inc

echo } >> ../src/conf.h.inc

echo \#endif >> ../src/conf.h.inc

echo \#endif >> ../src/conf.h.inc



