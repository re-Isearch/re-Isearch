#!/bin/sh

gcc-10 -o configure.bin configure.c
./configure.bin



echo \#ifndef CONF_H
echo \#define CONF_H

echo \#ifdef __cplusplus
echo extern "C" {
echo #endif

echo \#define HOST_PLATFORM \"`uname -o -i`\"
echo \#include  \"conf.h.inc\"

echo \#   define HOST_COMPILER \"`gcc-10 --version|head -1`\"

echo \#ifdef __cplusplus
echo }
echo \#endif
echo \#endif


