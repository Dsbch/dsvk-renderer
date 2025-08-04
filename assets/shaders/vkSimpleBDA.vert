#version 450
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 outColor;

struct vertex {
    vec3 position;       float _pad0;
    vec2 textureCoords;  vec2  _pad1;
    vec3 normal;         float _pad2;
    vec3 tangent;        float _pad3;
};

layout(buffer_reference, std430) readonly buffer vertexBuffer{ 
	vertex vertices[];
};

layout(buffer_reference, std430) readonly buffer indexBuffer{ 
	uint indices[];
};

layout( push_constant ) uniform constants
{
	mat4 viewProjection;
	vertexBuffer vb;
	indexBuffer ib;
} pushConstants;

vec3 hash(vec3 p) {
    p = fract(p * 0.3183099 + vec3(0.1, 0.2, 0.3));
    p *= 17.0;
    return fract(p);
}

void main()
{
	// load index/vertex data from device adress
	uint i = pushConstants.ib.indices[gl_VertexIndex];
	vertex v = pushConstants.vb.vertices[i];

	gl_Position = pushConstants.viewProjection*vec4(14.0f*v.position, 1.0f);

	outColor = hash(v.position);
}