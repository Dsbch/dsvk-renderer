#version 440 core

uniform sampler2D u_albedo;

in vec2 v_tex_coords;
in vec4 pos;

out vec4 color;

void main()
{
    color = texture(u_albedo, v_tex_coords);
}