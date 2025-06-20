#version 440 core

uniform sampler2D uAlbedo;

in vsOUT {
    vec2 texCoords;
} fsIN;

out vec4 color;

void main()
{
    color = texture(uAlbedo, fsIN.texCoords);
}