#version 330 core

out vec4 frag;

in vec2 frag_uv;

uniform sampler2D tex1;

void main()
{
  //vec4 color = texture(tex1, frag_uv);
  //frag = vec4(color.rgb, color.a*0.5);
  //if (color.a == 0.0) discard;
  //frag = color;
  frag = texture(tex1, frag_uv);
}
