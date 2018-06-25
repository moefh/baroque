#version 330 core

out vec4 frag;

in vec2 frag_uv;

uniform sampler2D tex1;
uniform vec3 text_color;

void main()
{
  frag = texture(tex1, frag_uv) * vec4(text_color, 1.0);
}
