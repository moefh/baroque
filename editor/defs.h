/* defs.h */

#ifndef DEFS_H_FILE
#define DEFS_H_FILE

#if defined _MSC_VER
#  define __attribute__(x)
#  define COMPILER_VERSION_FORMAT "MSVC %d", _MSC_FULL_VER
#elif defined __clang__
#  define COMPILER_VERSION_FORMAT "Clang %s", __clang_version__
#elif defined __GNUC__
#  define COMPILER_VERSION_FORMAT "GCC %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__
#else
#  define COMPILER_VERSION_FORMAT "Unknown compiler"
#endif

#endif /* DEFS_H_FILE */
