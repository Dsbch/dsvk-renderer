#version 440 core

uniform samplerCube uSkybox;

in vec3 v_tex_coords;

out vec4 color;

void main()
{
    color = texture(uSkybox, v_tex_coords);
}