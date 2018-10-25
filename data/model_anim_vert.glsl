#version 330 core
layout (location = 0) in vec3  vtx_pos;
layout (location = 1) in vec3  vtx_normal;
layout (location = 2) in vec2  vtx_uv;
layout (location = 3) in uvec4 vtx_bone;
layout (location = 4) in vec4  vtx_weight;

out vec3 frag_pos;
out vec3 frag_normal;
out vec2 frag_uv;

uniform mat4 mat_model_view_projection;
uniform mat4 mat_model;
uniform mat4 mat_normal;
uniform mat4 mat_bones[32];

void main()
{
  vec4 pos4 = vec4(vtx_pos, 1.0);
  frag_pos = (vtx_weight[0] * vec3(mat_model * mat_bones[vtx_bone[0]] * pos4) +
              vtx_weight[1] * vec3(mat_model * mat_bones[vtx_bone[1]] * pos4) +
              vtx_weight[2] * vec3(mat_model * mat_bones[vtx_bone[2]] * pos4) +
              vtx_weight[3] * vec3(mat_model * mat_bones[vtx_bone[3]] * pos4));

  mat3 mat_normal3 = mat3(mat_normal);
  frag_normal = (vtx_weight[0] * vec3(mat_normal3 * mat3(mat_bones[vtx_bone[0]]) * vtx_normal) +
                 vtx_weight[1] * vec3(mat_normal3 * mat3(mat_bones[vtx_bone[1]]) * vtx_normal) +
                 vtx_weight[2] * vec3(mat_normal3 * mat3(mat_bones[vtx_bone[2]]) * vtx_normal) +
                 vtx_weight[3] * vec3(mat_normal3 * mat3(mat_bones[vtx_bone[3]]) * vtx_normal));
  
  frag_uv = vtx_uv;

  gl_Position = (vtx_weight[0] * mat_model_view_projection * mat_bones[vtx_bone[0]] * pos4 +
                 vtx_weight[1] * mat_model_view_projection * mat_bones[vtx_bone[1]] * pos4 +
                 vtx_weight[2] * mat_model_view_projection * mat_bones[vtx_bone[2]] * pos4 +
                 vtx_weight[3] * mat_model_view_projection * mat_bones[vtx_bone[3]] * pos4);
}
