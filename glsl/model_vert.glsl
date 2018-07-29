#version 330 core
layout (location = 0) in vec3 vtx_pos;
layout (location = 1) in vec3 vtx_normal;
layout (location = 2) in vec2 vtx_uv;

out vec3 frag_pos;
out vec3 frag_normal;
out vec2 frag_uv;

uniform mat4 mat_model_view_projection;
uniform mat4 mat_model;
uniform mat4 mat_normal;

void main()
{
  frag_pos = vec3(mat_model * vec4(vtx_pos, 1.0));
  frag_normal = mat3(mat_normal) * vtx_normal;
  frag_uv = vtx_uv;

  gl_Position = mat_model_view_projection * vec4(vtx_pos, 1.0);
}
