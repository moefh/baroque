/* file.c */

#if defined(_WIN32)

#include "file_win32.c"

#elif defined(__unix__)

#include "file_mmap.c"

#else

#error "Unknown system, can't memory mapped file"

#endif
