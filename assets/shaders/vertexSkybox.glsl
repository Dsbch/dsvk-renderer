#version 440 core

// Per vertex attrs.
layout(location = 0) in vec3 position;

uniform mat4 uProjection;
uniform mat4 uView;

out vec3 v_tex_coords;

void main()
{
    v_tex_coords = position;

    vec4 pos = uProjection * uView * vec4(position, 1.0f);
    gl_Position = pos.xyww;
}