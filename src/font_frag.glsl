#version 330 core

out vec4 frag;

in vec2 frag_uv;

uniform sampler2D tex1;
uniform vec4 text_color;

void main()
{
  frag = texture(tex1, frag_uv) * text_color;
}
