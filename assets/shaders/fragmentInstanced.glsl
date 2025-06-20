#version 440 core

uniform sampler2D uAlbedo;

in vsOUT {
    vec2 texCoords;
    vec3 tangentViewPos;
    vec3 tangentFragmentPos;
} fsIN;

out vec4 color;

// all calculations in fragment shader should be in tangent space (because of normals).
void main()
{
    // vec3 normal = texture(uNormal, fs_in.TexCoords).rgb;
    // this normal is in tangent space, so we use tangentSpace things in fsIN.
    // normal = normalize(normal * 2.0 - 1.0);

    color = texture(uAlbedo, fsIN.texCoords);
}