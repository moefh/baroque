#version 330 core
layout (location = 0) in vec3 vtx_pos;
layout (location = 1) in vec2 vtx_uv;

out vec2 frag_uv;

uniform vec2 text_scale;
uniform vec2 text_pos;
uniform vec2 char_uv[64];

void main()
{
  vec2 pos = text_scale*vtx_pos.xy + text_pos;
  vec2 char_uv = char_uv[uint(vtx_pos.z)];
  
  frag_uv = vtx_uv + char_uv;
  gl_Position = vec4(pos.x, -pos.y, 0.0, 1.0);
}
