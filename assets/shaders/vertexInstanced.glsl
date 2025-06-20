#version 440 core

// Per vertex attrs.
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 tex_coords;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

// Per instance attr.
layout(location = 4) in mat4 model_matrix;

uniform mat4 uProjection;
uniform mat4 uView;
uniform vec3 uViewPos;

out vsOUT {
    vec2 texCoords;
} vsOut;

void main()
{
    vec4 worldPos = model_matrix * vec4(position.xyz, 1.0f);

    gl_Position = uProjection * uView * worldPos;
    vsOut.texCoords = tex_coords;
}