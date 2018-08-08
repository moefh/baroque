/* file.c */

#if defined(_WIN32)

#include "file_win32.c"

#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))

#include "file_mmap.c"

#else

#error "Unknown system, can't use memory mapped file"

#endif
