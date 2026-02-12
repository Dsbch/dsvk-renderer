#define PI 3.14159265359f

#define GAMMA 2.2f

// INPUT START.

// DescriptorSet START.

struct vertex
{
    float3 position;
    uint localMaterialOffset;
    float2 textureCoords;
    float3 normal;
    float4 tangent;
};

struct meshletBounds
{
	/* bounding sphere, useful for frustum and occlusion culling */
    float3 center;
    float radius;

	/* normal cone, useful for backface culling */
    float3 coneAxis;
    float coneCutoff; /* = cos(angle/2) */
};

struct meshlet
{
    uint indexBufferIndex;
    uint indexBufferOffset;
    
    uint vertexBufferIndex;
    uint vertexBufferOffset;
    uint vertexCount;
    
    uint triangleBufferIndex;
    uint triangleBufferOffset;
    uint triangleCount;
    
    meshletBounds bounds;
};

struct transform
{
    float3 translation;
    float3 scale;
    float4 rotation;
};

struct perInstanceAttr
{
    float3 bsWorldCenter;
    float bsWorldRadius;
    
    transform modelTransform;

    uint globalMaterialOffset;
};

struct command
{
    uint instanceIndex;
    uint instanceOffset;
    
    uint meshletIndex;
    uint meshletOffset1;
    uint meshletOffset2;
    uint meshletOffset3;
    uint meshletOffset4;
};

// SSBO START.

StructuredBuffer<vertex> vertexBuffer[] : register(t0, space0);
StructuredBuffer<perInstanceAttr> perInstanceBuffer[] : register(t1, space0);
StructuredBuffer<command> commandBuffer : register(t2, space0);
StructuredBuffer<uint> vertexIndexBuffer[] : register(t3, space0);
StructuredBuffer<uint> primitiveBuffer[] : register(t4, space0);
StructuredBuffer<meshlet> meshletBuffer[] : register(t5, space0);

// SSBO END.

// UBO START.

struct frustum
{
    float3 worldFrontN;
    float frontDistance;
    float3 worldBackN;
    float backDistance;
    float3 worldRightN;
    float rightDistance;
    float3 worldLeftN;
    float leftDistance;
    float3 worldTopN;
    float topDistance;
    float3 worldBottomN;
    float bottomDistance;
};

struct perDrawData
{
    float4x4 debugViewProjection;
    uint useDebugCamera;
    float3 cameraFront;
    float3 cameraPos;
    float3 cameraUp;
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
    frustum cameraFrustum;
    float deltaTime;
};

float3 rotate(float4 quat, float3 v)
{
    float3 uv = cross(quat.xyz, v);
    float3 uuv = cross(quat.xyz, uv);
    
    return v + ((uv * quat.w) + uuv) * 2.0f;
}

float3 translate(float3 translation, float3 v)
{
    return translation + v;
}

float3 scale(float3 scale, float3 v)
{
    return scale * v;
}

float3 transformPoint(transform pointTransform, float3 p)
{
    return translate(pointTransform.translation, rotate(pointTransform.rotation, scale(pointTransform.scale, p)));
}
