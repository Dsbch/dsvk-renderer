#version 440 core

// Per vertex attrs.
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoords;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

// Per instance attr.
layout(location = 4) in mat4 model;

uniform mat4 uProjection;
uniform mat4 uView;
uniform vec3 uCameraPos;

out vsOUT {
    vec2 texCoords;
    vec3 tangentCameraPos;
    vec3 tangentFragmentPos;
} vsOut;

void main()
{
    vec4 worldPos = model * vec4(position.xyz, 1.0f);
    gl_Position = uProjection * uView * worldPos;

    vsOut.texCoords = texCoords;

    // cast to mat3 removes translation.
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * tangent);
    vec3 N = normalize(normalMatrix * normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    mat3 TBN = transpose(mat3(T, B, N));

    vsOut.tangentCameraPos = TBN*uCameraPos;
    vsOut.tangentFragmentPos = TBN*worldPos.xyz;
}

if (tangentAccessor)
{
    float tan[4]{};
    cgltf_accessor_read_float(tangentAccessor, i, tan, 4);

    glm::vec3 t(tan[0], tan[1], tan[2]);

    // Tangent is a direction → w = 0
    glm::vec3 worldTangent =
        glm::normalize(glm::transpose(glm::inverse(glm::mat3(transform))) * t);

    v.tangent = glm::vec4(worldTangent, tan[3]); // keep handedness in .w
}
