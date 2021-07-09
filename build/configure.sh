#!/bin/sh

echo \#ifndef CONF_H
echo \#define CONF_H

echo \#ifdef __cplusplus
echo extern "C" {
echo #endif

echo \#define HOST_PLATFORM \"`uname -o -i`\"
./configure
echo \#include  \"conf.h.inc\"

echo \#   define HOST_COMPILER \"`g++ --version|head -1`\"

echo \#ifdef __cplusplus
echo }
echo \#endif
echo \#endif

