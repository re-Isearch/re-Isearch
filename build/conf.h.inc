#ifndef CONF_H_INC
#define CONF_H_INC
#ifdef __cplusplus

extern "C" {

#endif

#define SIZEOF_CHAR 1
#define SIZEOF_UCHAR 1
#define SIZEOF_SHORT_INT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG_INT 8
#define SIZEOF_LONG_LONG_INT 8
#define SIZEOF_FLOAT 4
#define SIZEOF_DOUBLE 8
#define SIZEOF_LONG_DOUBLE 8

#define IS_LITTLE_ENDIAN 1
#define HOST_PLATFORM "Darwin 22.1.0 arm64"
# define HOST_COMPILER "gcc-12 (Homebrew GCC 12.2.0) 12.2.0"
#ifdef __cplusplus
}
#endif
#endif
