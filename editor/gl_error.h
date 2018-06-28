/* gl_error.h */

#ifndef GL_ERROR_H_FILE
#define GL_ERROR_H_FILE

#define GL_CHECK(x)       do { x; check_gl_error(__FILE__, __LINE__); } while (0)
#define GL_CHECK_ERRORS() GL_CHECK((void)0)

void check_gl_error(const char *filename, int line_num);

#endif /* GL_ERROR_H_FILE */
