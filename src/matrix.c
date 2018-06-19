/* matrix.c */

#include <string.h>
#include <math.h>
#include <stdio.h>

#include "matrix.h"

void mat4_dump(const float *mat)
{
  for (int i = 0; i < 16; i++) {
    printf("  %8.5f", mat[i]);
    if (i % 4 == 3)
      printf("\n");
  }
}

void mat3_dump(const float *mat)
{
  for (int i = 0; i < 9; i++) {
    printf("  %8.5f", mat[i]);
    if (i % 4 == 2)
      printf("\n");
  }
}

void vec4_dump(const float *v)
{
  for (int i = 0; i < 4; i++)
    printf("  %8.5f", v[i]);
  printf("\n");
}

void vec3_dump(const float *v)
{
  for (int i = 0; i < 3; i++)
    printf("  %8.5f", v[i]);
  printf("\n");
}

void mat4_copy(float *restrict dest, const float *restrict src)
{
  memcpy(dest, src, 16*sizeof(float));
}

void mat4_load(float *m,
               float x0, float x1, float x2, float x3,
               float x4, float x5, float x6, float x7,
               float x8, float x9, float x10, float x11,
               float x12, float x13, float x14, float x15)
{
  m[ 0] = x0;
  m[ 1] = x1;
  m[ 2] = x2;
  m[ 3] = x3;

  m[ 4] = x4;
  m[ 5] = x5;
  m[ 6] = x6;
  m[ 7] = x7;

  m[ 8] = x8;
  m[ 9] = x9;
  m[10] = x10;
  m[11] = x11;

  m[12] = x12;
  m[13] = x13;
  m[14] = x14;
  m[15] = x15;
}

void mat4_id(float *m)
{
  m[ 0] = 1.0;
  m[ 1] = 0.0;
  m[ 2] = 0.0;
  m[ 3] = 0.0;
  
  m[ 4] = 0.0;
  m[ 5] = 1.0;
  m[ 6] = 0.0;
  m[ 7] = 0.0;

  m[ 8] = 0.0;
  m[ 9] = 0.0;
  m[10] = 1.0;
  m[11] = 0.0;

  m[12] = 0.0;
  m[13] = 0.0;
  m[14] = 0.0;
  m[15] = 1.0;
}

void mat4_load_scale(float *m, float sx, float sy, float sz)
{
  m[ 0] = sx;
  m[ 1] = 0.0;
  m[ 2] = 0.0;
  m[ 3] = 0.0;
  
  m[ 4] = 0.0;
  m[ 5] = sy;
  m[ 6] = 0.0;
  m[ 7] = 0.0;

  m[ 8] = 0.0;
  m[ 9] = 0.0;
  m[10] = sz;
  m[11] = 0.0;

  m[12] = 0.0;
  m[13] = 0.0;
  m[14] = 0.0;
  m[15] = 1.0;
}

void mat4_load_translation(float *m, float tx, float ty, float tz)
{
  m[ 0] = 1.0;
  m[ 1] = 0.0;
  m[ 2] = 0.0;
  m[ 3] = tx;
  
  m[ 4] = 0.0;
  m[ 5] = 1.0;
  m[ 6] = 0.0;
  m[ 7] = ty;

  m[ 8] = 0.0;
  m[ 9] = 0.0;
  m[10] = 1.0;
  m[11] = tz;

  m[12] = 0.0;
  m[13] = 0.0;
  m[14] = 0.0;
  m[15] = 1.0;
}

void mat4_frustum(float *m, float left, float right, float bottom, float top, float near, float far)
{
  float width = right - left;
  float height = top - bottom;
  float depth = far - near;
  
  mat4_load(m,
            2.0*near/width, 0.0,             (right+left)/width,  0.0,
            0.0,            2.0*near/height, (top+bottom)/height, 0.0,
            0.0,            0.0,             -(far+near)/depth,   -2.0*far*near/depth,
            0.0,            0.0,             -1.0,                0.0);
}

void mat4_inf_perspective(float *m, float aspect, float yfov, float near)
{
  float a = tan(0.5 * yfov);
  
  mat4_load(m,
            1.0/(aspect*a), 0.0,      0.0,   0.0,
            0.0,            1.0/a,    0.0,   0.0,
            0.0,            0.0,     -1.0,  -2.0*near,
            0.0,            0.0,     -1.0,   0.0);
}

void mat4_perspective(float *m, float aspect, float yfov, float near, float far)
{
  float a = tan(0.5 * yfov);
  float nmf = near - far;
  
  mat4_load(m,
            1.0/(aspect*a), 0.0,    0.0,            0.0,
            0.0,            1.0/a,  0.0,            0.0,
            0.0,            0.0,    (far+near)/nmf, 2.0*far*near/nmf,
            0.0,            0.0,   -1.0,            0.0);
}

void mat4_look_at(float *m,
                  float ex, float ey, float ez,
                  float cx, float cy, float cz,
                  float ux, float uy, float uz)
{
  float eye[3] = { ex, ey, ez };
  float dir[3] = { cx-ex, cy-ey, cz-ez };
  float up[3] = { ux, uy, uz };

  vec3_normalize(dir);
  vec3_normalize(up);

  float side[3];
  vec3_cross(side, dir, up);
  vec3_normalize(side);
  vec3_cross(up, side, dir);

  float x = -vec3_dot(eye, side);
  float y = -vec3_dot(eye, up);
  float z = vec3_dot(eye, dir);
  
  mat4_load(m,
             side[0],  side[1],  side[2], x,
             up[0],    up[1],    up[2],   y, 
            -dir[0],  -dir[1],  -dir[2],  z,
             0.0,      0.0,      0.0,     1.0);
}

void mat4_scale(float *m, float scale)
{
  m[ 0] *= scale;
  m[ 1] *= scale;
  m[ 2] *= scale;

  m[ 4] *= scale;
  m[ 5] *= scale;
  m[ 6] *= scale;
  
  m[ 8] *= scale;
  m[ 9] *= scale;
  m[10] *= scale;

  m[12] *= scale;
  m[13] *= scale;
  m[14] *= scale;
}

void mat4_load_rot_x(float *m, float angle)
{
  float c = cos(angle);
  float s = sin(angle);

  m[ 0] = 1.0;
  m[ 1] = 0.0;
  m[ 2] = 0.0;
  m[ 3] = 0.0;

  m[ 4] = 0.0;
  m[ 5] = c;
  m[ 6] = -s;
  m[ 7] = 0.0;
  
  m[ 8] = 0.0;
  m[ 9] = s;
  m[10] = c;
  m[11] = 0.0;

  m[12] = 0.0;
  m[13] = 0.0;
  m[14] = 0.0;
  m[15] = 1.0;
}

void mat4_load_rot_y(float *m, float angle)
{
  float c = cos(angle);
  float s = sin(angle);

  m[ 0] = c;
  m[ 1] = 0.0;
  m[ 2] = -s;
  m[ 3] = 0.0;
  
  m[ 4] = 0.0;
  m[ 5] = 1.0;
  m[ 6] = 0.0;
  m[ 7] = 0.0;

  m[ 8] = s;
  m[ 9] = 0.0;
  m[10] = c;
  m[11] = 0.0;

  m[12] = 0.0;
  m[13] = 0.0;
  m[14] = 0.0;
  m[15] = 1.0;
}

void mat4_load_rot_z(float *m, float angle)
{
  float c = cos(angle);
  float s = sin(angle);

  m[ 0] = c;
  m[ 1] = -s;
  m[ 2] = 0.0;
  m[ 3] = 0.0;
  
  m[ 4] = s;
  m[ 5] = c;
  m[ 6] = 0.0;
  m[ 7] = 0.0;

  m[ 8] = 0.0;
  m[ 9] = 0.0;
  m[10] = 1.0;
  m[11] = 0.0;

  m[12] = 0.0;
  m[13] = 0.0;
  m[14] = 0.0;
  m[15] = 1.0;
}

// a = b*c
void mat4_mul(float *restrict a, const float *restrict b, const float *restrict c)
{
  a[ 0] = b[ 0]*c[ 0] + b[ 1]*c[ 4] + b[ 2]*c[ 8] + b[ 3]*c[12];
  a[ 1] = b[ 0]*c[ 1] + b[ 1]*c[ 5] + b[ 2]*c[ 9] + b[ 3]*c[13];
  a[ 2] = b[ 0]*c[ 2] + b[ 1]*c[ 6] + b[ 2]*c[10] + b[ 3]*c[14];
  a[ 3] = b[ 0]*c[ 3] + b[ 1]*c[ 7] + b[ 2]*c[11] + b[ 3]*c[15];
  
  a[ 4] = b[ 4]*c[ 0] + b[ 5]*c[ 4] + b[ 6]*c[ 8] + b[ 7]*c[12];
  a[ 5] = b[ 4]*c[ 1] + b[ 5]*c[ 5] + b[ 6]*c[ 9] + b[ 7]*c[13];
  a[ 6] = b[ 4]*c[ 2] + b[ 5]*c[ 6] + b[ 6]*c[10] + b[ 7]*c[14];
  a[ 7] = b[ 4]*c[ 3] + b[ 5]*c[ 7] + b[ 6]*c[11] + b[ 7]*c[15];

  a[ 8] = b[ 8]*c[ 0] + b[ 9]*c[ 4] + b[10]*c[ 8] + b[11]*c[12];
  a[ 9] = b[ 8]*c[ 1] + b[ 9]*c[ 5] + b[10]*c[ 9] + b[11]*c[13];
  a[10] = b[ 8]*c[ 2] + b[ 9]*c[ 6] + b[10]*c[10] + b[11]*c[14];
  a[11] = b[ 8]*c[ 3] + b[ 9]*c[ 7] + b[10]*c[11] + b[11]*c[15];
  
  a[12] = b[12]*c[ 0] + b[13]*c[ 4] + b[14]*c[ 8] + b[15]*c[12];
  a[13] = b[12]*c[ 1] + b[13]*c[ 5] + b[14]*c[ 9] + b[15]*c[13];
  a[14] = b[12]*c[ 2] + b[13]*c[ 6] + b[14]*c[10] + b[15]*c[14];
  a[15] = b[12]*c[ 3] + b[13]*c[ 7] + b[14]*c[11] + b[15]*c[15];
}

// a = a*b
void mat4_mul_right(float *restrict a, const float *restrict b)
{
  float x0  = a[ 0]*b[ 0] + a[ 1]*b[ 4] + a[ 2]*b[ 8] + a[ 3]*b[12];
  float x1  = a[ 0]*b[ 1] + a[ 1]*b[ 5] + a[ 2]*b[ 9] + a[ 3]*b[13];
  float x2  = a[ 0]*b[ 2] + a[ 1]*b[ 6] + a[ 2]*b[10] + a[ 3]*b[14];
  float x3  = a[ 0]*b[ 3] + a[ 1]*b[ 7] + a[ 2]*b[11] + a[ 3]*b[15];
  
  float x4  = a[ 4]*b[ 0] + a[ 5]*b[ 4] + a[ 6]*b[ 8] + a[ 7]*b[12];
  float x5  = a[ 4]*b[ 1] + a[ 5]*b[ 5] + a[ 6]*b[ 9] + a[ 7]*b[13];
  float x6  = a[ 4]*b[ 2] + a[ 5]*b[ 6] + a[ 6]*b[10] + a[ 7]*b[14];
  float x7  = a[ 4]*b[ 3] + a[ 5]*b[ 7] + a[ 6]*b[11] + a[ 7]*b[15];

  float x8  = a[ 8]*b[ 0] + a[ 9]*b[ 4] + a[10]*b[ 8] + a[11]*b[12];
  float x9  = a[ 8]*b[ 1] + a[ 9]*b[ 5] + a[10]*b[ 9] + a[11]*b[13];
  float x10 = a[ 8]*b[ 2] + a[ 9]*b[ 6] + a[10]*b[10] + a[11]*b[14];
  float x11 = a[ 8]*b[ 3] + a[ 9]*b[ 7] + a[10]*b[11] + a[11]*b[15];
  
  float x12 = a[12]*b[ 0] + a[13]*b[ 4] + a[14]*b[ 8] + a[15]*b[12];
  float x13 = a[12]*b[ 1] + a[13]*b[ 5] + a[14]*b[ 9] + a[15]*b[13];
  float x14 = a[12]*b[ 2] + a[13]*b[ 6] + a[14]*b[10] + a[15]*b[14];
  float x15 = a[12]*b[ 3] + a[13]*b[ 7] + a[14]*b[11] + a[15]*b[15];

  a[ 0] = x0;
  a[ 1] = x1;
  a[ 2] = x2;
  a[ 3] = x3;
  a[ 4] = x4;
  a[ 5] = x5;
  a[ 6] = x6;
  a[ 7] = x7;
  a[ 8] = x8;
  a[ 9] = x9;
  a[10] = x10;
  a[11] = x11;
  a[12] = x12;
  a[13] = x13;
  a[14] = x14;
  a[15] = x15;
}

// a = b*a
void mat4_mul_left(float *restrict a, const float *restrict b)
{
  float x0  = b[ 0]*a[ 0] + b[ 1]*a[ 4] + b[ 2]*a[ 8] + b[ 3]*a[12];
  float x1  = b[ 0]*a[ 1] + b[ 1]*a[ 5] + b[ 2]*a[ 9] + b[ 3]*a[13];
  float x2  = b[ 0]*a[ 2] + b[ 1]*a[ 6] + b[ 2]*a[10] + b[ 3]*a[14];
  float x3  = b[ 0]*a[ 3] + b[ 1]*a[ 7] + b[ 2]*a[11] + b[ 3]*a[15];
  
  float x4  = b[ 4]*a[ 0] + b[ 5]*a[ 4] + b[ 6]*a[ 8] + b[ 7]*a[12];
  float x5  = b[ 4]*a[ 1] + b[ 5]*a[ 5] + b[ 6]*a[ 9] + b[ 7]*a[13];
  float x6  = b[ 4]*a[ 2] + b[ 5]*a[ 6] + b[ 6]*a[10] + b[ 7]*a[14];
  float x7  = b[ 4]*a[ 3] + b[ 5]*a[ 7] + b[ 6]*a[11] + b[ 7]*a[15];

  float x8  = b[ 8]*a[ 0] + b[ 9]*a[ 4] + b[10]*a[ 8] + b[11]*a[12];
  float x9  = b[ 8]*a[ 1] + b[ 9]*a[ 5] + b[10]*a[ 9] + b[11]*a[13];
  float x10 = b[ 8]*a[ 2] + b[ 9]*a[ 6] + b[10]*a[10] + b[11]*a[14];
  float x11 = b[ 8]*a[ 3] + b[ 9]*a[ 7] + b[10]*a[11] + b[11]*a[15];
  
  float x12 = b[12]*a[ 0] + b[13]*a[ 4] + b[14]*a[ 8] + b[15]*a[12];
  float x13 = b[12]*a[ 1] + b[13]*a[ 5] + b[14]*a[ 9] + b[15]*a[13];
  float x14 = b[12]*a[ 2] + b[13]*a[ 6] + b[14]*a[10] + b[15]*a[14];
  float x15 = b[12]*a[ 3] + b[13]*a[ 7] + b[14]*a[11] + b[15]*a[15];

  a[ 0] = x0;
  a[ 1] = x1;
  a[ 2] = x2;
  a[ 3] = x3;
  a[ 4] = x4;
  a[ 5] = x5;
  a[ 6] = x6;
  a[ 7] = x7;
  a[ 8] = x8;
  a[ 9] = x9;
  a[10] = x10;
  a[11] = x11;
  a[12] = x12;
  a[13] = x13;
  a[14] = x14;
  a[15] = x15;
}

int mat4_inverse(float *restrict out, const float *restrict m)
{
  float inv[16];

  inv[ 0] = ( m[ 5] * m[10] * m[15] -
              m[ 5] * m[14] * m[11] -
              m[ 6] * m[ 9] * m[15] +
              m[ 6] * m[13] * m[11] +
              m[ 7] * m[ 9] * m[14] -
              m[ 7] * m[13] * m[10]);
  
  inv[ 1] = (-m[ 1] * m[10] * m[15] +
              m[ 1] * m[14] * m[11] +
              m[ 2] * m[ 9] * m[15] -
              m[ 2] * m[13] * m[11] -
              m[ 3] * m[ 9] * m[14] +
              m[ 3] * m[13] * m[10]);

  inv[ 2] = (m[ 1] * m[ 6] * m[15] -
             m[ 1] * m[14] * m[ 7] -
             m[ 2] * m[ 5] * m[15] +
             m[ 2] * m[13] * m[ 7] +
             m[ 3] * m[ 5] * m[14] -
             m[ 3] * m[13] * m[ 6]);

  inv[ 3] = (-m[ 1] * m[ 6] * m[11] +
              m[ 1] * m[10] * m[ 7] +
              m[ 2] * m[ 5] * m[11] -
              m[ 2] * m[ 9] * m[ 7] -
              m[ 3] * m[ 5] * m[10] +
              m[ 3] * m[ 9] * m[ 6]);

  inv[ 4] = (-m[ 4] * m[10] * m[15] +
              m[ 4] * m[14] * m[11] +
              m[ 6] * m[ 8] * m[15] -
              m[ 6] * m[12] * m[11] -
              m[ 7] * m[ 8] * m[14] +
              m[ 7] * m[12] * m[10]);

  inv[ 5] = ( m[ 0] * m[10] * m[15] -
              m[ 0] * m[14] * m[11] -
              m[ 2] * m[ 8] * m[15] +
              m[ 2] * m[12] * m[11] +
              m[ 3] * m[ 8] * m[14] -
              m[ 3] * m[12] * m[10]);

  inv[ 6] = (-m[ 0] * m[ 6] * m[15] +
              m[ 0] * m[14] * m[ 7] +
              m[ 2] * m[ 4] * m[15] -
              m[ 2] * m[12] * m[ 7] -
              m[ 3] * m[ 4] * m[14] +
              m[ 3] * m[12] * m[ 6]);

  inv[ 7] = (m[ 0] * m[ 6] * m[11] -
             m[ 0] * m[10] * m[ 7] -
             m[ 2] * m[ 4] * m[11] +
             m[ 2] * m[ 8] * m[ 7] +
             m[ 3] * m[ 4] * m[10] -
             m[ 3] * m[ 8] * m[ 6]);

  inv[ 8] = (m[ 4] * m[ 9] * m[15] -
             m[ 4] * m[13] * m[11] -
             m[ 5] * m[ 8] * m[15] +
             m[ 5] * m[12] * m[11] +
             m[ 7] * m[ 8] * m[13] -
             m[ 7] * m[12] * m[ 9]);

  inv[ 9] = (-m[ 0] * m[ 9] * m[15] +
              m[ 0] * m[13] * m[11] +
              m[ 1] * m[ 8] * m[15] -
              m[ 1] * m[12] * m[11] -
              m[ 3] * m[ 8] * m[13] +
              m[ 3] * m[12] * m[ 9]);

  inv[10] = ( m[ 0] * m[ 5] * m[15] -
              m[ 0] * m[13] * m[ 7] -
              m[ 1] * m[ 4] * m[15] +
              m[ 1] * m[12] * m[ 7] +
              m[ 3] * m[ 4] * m[13] -
              m[ 3] * m[12] * m[ 5]);

  inv[11] = (-m[ 0] * m[ 5] * m[11] +
              m[ 0] * m[ 9] * m[ 7] +
              m[ 1] * m[ 4] * m[11] -
              m[ 1] * m[ 8] * m[ 7] -
              m[ 3] * m[ 4] * m[ 9] +
              m[ 3] * m[ 8] * m[ 5]);

  inv[12] = (-m[ 4] * m[ 9] * m[14] +
              m[ 4] * m[13] * m[10] +
              m[ 5] * m[ 8] * m[14] -
              m[ 5] * m[12] * m[10] -
              m[ 6] * m[ 8] * m[13] +
              m[ 6] * m[12] * m[ 9]);

  inv[13] = ( m[ 0] * m[ 9] * m[14] -
              m[ 0] * m[13] * m[10] -
              m[ 1] * m[ 8] * m[14] +
              m[ 1] * m[12] * m[10] +
              m[ 2] * m[ 8] * m[13] -
              m[ 2] * m[12] * m[ 9]);

  inv[14] = (-m[ 0] * m[ 5] * m[14] +
              m[ 0] * m[13] * m[ 6] +
              m[ 1] * m[ 4] * m[14] -
              m[ 1] * m[12] * m[ 6] -
              m[ 2] * m[ 4] * m[13] +
              m[ 2] * m[12] * m[ 5]);

  inv[15] = ( m[ 0] * m[ 5] * m[10] -
              m[ 0] * m[ 9] * m[ 6] -
              m[ 1] * m[ 4] * m[10] +
              m[ 1] * m[ 8] * m[ 6] +
              m[ 2] * m[ 4] * m[ 9] -
              m[ 2] * m[ 8] * m[ 5]);

  double det = m[0] * inv[0] + m[4] * inv[1] + m[8] * inv[2] + m[12] * inv[3];
  if (det == 0.0)
    return 1;

  det = 1.0 / det;
  for (int i = 0; i < 16; i++)
    out[i] = inv[i] * det;
  return 0;
}

void mat4_transpose(float *restrict out, const float *restrict m)
{
  out[ 0] = m[ 0];
  out[ 1] = m[ 4];
  out[ 2] = m[ 8];
  out[ 3] = m[12];

  out[ 4] = m[ 1];
  out[ 5] = m[ 5];
  out[ 6] = m[ 9];
  out[ 7] = m[13];

  out[ 8] = m[ 2];
  out[ 9] = m[ 6];
  out[10] = m[10];
  out[11] = m[14];

  out[12] = m[ 3];
  out[13] = m[ 7];
  out[14] = m[11];
  out[15] = m[15];
}

void mat4_mul_vec4(float *restrict ret, const float *restrict m, const float *restrict v)
{
  ret[0] = m[ 0]*v[0] + m[ 1]*v[0] + m[ 2]*v[2] + m[ 3]*v[3];
  ret[1] = m[ 4]*v[1] + m[ 5]*v[1] + m[ 6]*v[2] + m[ 7]*v[3];
  ret[2] = m[ 8]*v[2] + m[ 9]*v[2] + m[10]*v[2] + m[11]*v[3];
  ret[3] = m[12]*v[3] + m[13]*v[3] + m[14]*v[2] + m[15]*v[3];
}

void mat4_mul_vec3(float *restrict ret, const float *m, const float *restrict v)
{
  ret[0] = m[ 0]*v[0] + m[ 1]*v[0] + m[ 2]*v[2];
  ret[1] = m[ 4]*v[1] + m[ 5]*v[1] + m[ 6]*v[2];
  ret[2] = m[ 8]*v[2] + m[ 9]*v[2] + m[10]*v[2];
}

void mat3_copy(float *restrict dest, const float *restrict src)
{
  memcpy(dest, src, 9*sizeof(float));
}

void mat3_id(float *m)
{
  m[0] = 1.0;
  m[1] = 0.0;
  m[2] = 0.0;

  m[3] = 0.0;
  m[4] = 1.0;
  m[5] = 0.0;

  m[6] = 0.0;
  m[7] = 0.0;
  m[8] = 1.0;
}

int mat3_inverse(float *restrict out, float *restrict m)
{
  float inv[9];

  inv[0] =  (m[4]*m[8] - m[7]*m[5]);
  inv[1] = -(m[1]*m[8] - m[7]*m[2]);
  inv[2] =  (m[1]*m[5] - m[4]*m[2]);

  inv[3] = -(m[3]*m[8] - m[6]*m[5]);
  inv[4] =  (m[0]*m[8] - m[6]*m[2]);
  inv[5] = -(m[0]*m[5] - m[3]*m[2]);

  inv[6] =  (m[3]*m[7] - m[6]*m[4]);
  inv[7] = -(m[0]*m[7] - m[6]*m[1]);
  inv[8] =  (m[0]*m[4] - m[3]*m[1]);

  float det = m[0]*inv[0] + m[1]*inv[3] + m[2]*inv[6];
  if (det == 0.0)
    return 1;

  det = 1.0/det;
  for (int i = 0; i < 9; i++)
    out[i] = inv[i] * det;
  return 0;
}

void mat3_from_mat4(float *restrict mat3, const float *restrict mat4)
{
  mat3[0] = mat4[0];
  mat3[1] = mat4[1];
  mat3[2] = mat4[2];

  mat3[3] = mat4[4];
  mat3[4] = mat4[5];
  mat3[5] = mat4[6];

  mat3[6] = mat4[8];
  mat3[7] = mat4[9];
  mat3[8] = mat4[10];
}

void mat3_mul_vec3(float *restrict ret, const float *m, const float *restrict v)
{
  ret[0] = m[0]*v[0] + m[1]*v[0] + m[2]*v[2];
  ret[1] = m[3]*v[1] + m[4]*v[1] + m[5]*v[2];
  ret[2] = m[6]*v[2] + m[7]*v[2] + m[8]*v[2];
}

void vec3_load_spherical(float *v, float r, float theta, float phi)
{
  float st = sin(theta);
  float ct = cos(theta);
  float sp = sin(phi);
  float cp = cos(phi);
  //v[0] = r * st * cp;
  //v[1] = r * st * sp;
  //v[2] = r * ct;
  v[0] = r * cp * ct;
  v[1] = r * sp;
  v[2] = r * cp * st;
}

void vec3_cross(float *restrict ret, const float *restrict a, const float *restrict b)
{
  ret[0] = a[1]*b[2] - a[2]*b[1];
  ret[1] = a[2]*b[0] - a[0]*b[2];
  ret[2] = a[0]*b[1] - a[1]*b[0];
}
