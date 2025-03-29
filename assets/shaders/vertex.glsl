#version 440 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coords;
layout(location = 2) in int tex_index;

uniform mat4 uProjection;
uniform mat4 uView;

out vec4 pos;
out vec2 v_tex_coords;
out float v_tex_index;

void main()
{
    gl_Position = uProjection * uView * vec4(position.xyz, 1.0f);
    pos = gl_Position;
    v_tex_coords = tex_coords;
    v_tex_index = float(tex_index);
}