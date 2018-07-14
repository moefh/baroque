/* thread.c */

#if defined(_WIN32)

#include "thread_win32.c"

#elif defined(__unix__)

#include "thread_pthreads.c"

#else

#error "Unknown system, can't use threads"

#endif

