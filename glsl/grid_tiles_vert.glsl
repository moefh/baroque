#version 330 core
layout (location = 0) in vec3 vtx_pos;
layout (location = 1) in vec2 vtx_uv;

out vec2 frag_uv;

uniform mat4 mat_model_view_projection;

void main()
{
  frag_uv = vtx_uv;
  gl_Position = mat_model_view_projection * vec4(vtx_pos, 1.0);
}
