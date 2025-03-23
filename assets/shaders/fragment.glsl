#version 440 core

out vec4 color;

void main()
{
    // color = texture(u_textures[int(v_tex_index)], v_tex_coords);
    color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}