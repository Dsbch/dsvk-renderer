#version 440 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coords;
layout(location = 2) in int tex_index;

void main()
{
    gl_Position = vec4(position.xyz, 1.0f);
}