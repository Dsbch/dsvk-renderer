#version 440 core

uniform vec4 u_color;
uniform sampler2D u_textures[32];

in vec2 v_tex_coords;
in float v_tex_index;
in vec4 pos;

out vec4 color;

void main()
{
    color = texture(u_textures[int(v_tex_index)], v_tex_coords);
    // color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}