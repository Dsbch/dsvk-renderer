#version 440 core

// Per vertex attrs.
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coords;
layout(location = 2) in int tex_index;

// Per instance attr.
layout(location = 3) in mat3 model_matrix;

uniform mat4 uProjection;
uniform mat4 uView;

out vec4 pos;
out vec2 v_tex_coords;
out float v_tex_index;

void main()
{
    vec3 worldPos = model_matrix * position;

    gl_Position = uProjection * uView * vec4(worldPos, 1.0f);
    pos = gl_Position;
    v_tex_coords = tex_coords;
    v_tex_index = float(tex_index);
}