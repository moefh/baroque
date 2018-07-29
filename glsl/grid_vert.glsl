#version 330 core
layout (location = 0) in vec3 vtx_pos;

uniform mat4 mat_model_view_projection;

void main()
{
  gl_Position = mat_model_view_projection * vec4(vtx_pos, 1.0);
}
