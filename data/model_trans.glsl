#version 330 core

out vec4 frag;

in vec3 frag_pos;
in vec3 frag_normal;
in vec2 frag_uv;

uniform vec3 light_pos;
uniform vec3 camera_pos;
uniform float alpha;
uniform sampler2D tex1;

void main()
{
  vec3 normal = normalize(frag_normal);

  float ambient = 0.4;

  vec3 light_dir = normalize(light_pos - frag_pos);
  //vec3 light_dir = vec3(0,1,0);
  float diffuse = max(dot(normal, light_dir), 0.0);
  diffuse *= 0.6;

  vec3 camera_dir = normalize(camera_pos - frag_pos);
  vec3 reflect_dir = reflect(-light_dir, normal);
  float specular = 0.8 * pow(max(dot(camera_dir, reflect_dir), 0.0), 32);

  vec4 tex_rgba = texture(tex1, frag_uv);
  
  frag = vec4(clamp(ambient + diffuse + specular, 0.0, 1.0) * tex_rgba.rgb, tex_rgba.a * alpha);
}
