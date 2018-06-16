/* matrix.h */

#ifndef MATRIX_H_FILE
#define MATRIX_H_FILE

void mat3_dump(const float *mat);
void mat4_dump(const float *mat);
void vec3_dump(const float *v);
void vec4_dump(const float *v);

// mat4:
void mat4_id(float *m);
void mat4_copy(float *restrict dest, const float *restrict src);
void mat4_load(float *m,
	       float x0, float x1, float x2, float x3,
	       float x4, float x5, float x6, float x7,
	       float x8, float x9, float x10, float x11,
	       float x12, float x13, float x14, float x15);

void mat4_frustum(float *m,
                  float left, float right,
                  float bottom, float top,
                  float near, float far);
void mat4_look_at(float *m,
		  float ex, float ey, float ez,
		  float cx, float cy, float cz,
		  float ux, float uy, float uz);

void mat4_scale(float *m, float scale);
void mat4_rot_x(float *m, float angle);
void mat4_rot_y(float *m, float angle);
void mat4_rot_z(float *m, float angle);

void mat4_mul(float *restrict a, const float *restrict b, const float *restrict c);
void mat4_mul_right(float *restrict a, const float *restrict b);
void mat4_mul_left(float *restrict a, const float *restrict b);
void mat4_mul_vec4(float *restrict ret, const float *restrict m, const float *restrict v);
void mat4_mul_vec3(float *restrict ret, const float *restrict m, const float *restrict v);

// mat3:
void mat3_copy(float *restrict dest, const float *restrict src);
void mat3_id(float *m);

void mat3_from_mat4(float *restrict mat3, const float *restrict mat4);
void mat3_mul_vec3(float *restrict ret, const float *m, const float *restrict v);

// vec3:
void vec3_load(float *v, float x, float y, float z);
void vec3_load_spherical(float *v, float r, float theta, float phi);
void vec3_copy(float *restrict dest, const float *restrict src);
float vec3_dot(const float *a, const float *b);
void vec3_cross(float *restrict ret, const float *restrict a, const float *restrict b);
void vec3_normalize(float *v);
  
#endif /* MATRIX_H_FILE */
