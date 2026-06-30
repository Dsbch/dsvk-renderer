#ifdef __spirv__
#define DEFINE_AS_PUSH_CONSTANT [[vk::push_constant]]
#else
#define DEFINE_AS_PUSH_CONSTANT
#endif

#define THREADS_COUNT 32

#define PI 3.14159265359f

#define GAMMA 2.2f

#define EPSILON 0.00001f

#define MAX_UINT 4294967295;
    
#define VISIBLE_BIT          (1 << 0)
#define NOT_VISIBLE_NOW_BIT  (1 << 1)
#define NOT_VISIBLE_PREV_BIT (1 << 2)

#define BLEND_ALPHA_MODE 1

// INPUT START.

// DescriptorSet START.

struct skinnedVertex
{
    float3 position;
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
    uint alphaType;
    uint localMaterialOffset;

    uint indexBufferIndex;
    uint indexBufferOffset;
    
    uint weightBufferOffset;
    uint weightBufferIndex;
    
    uint vertexBufferIndex;
    uint vertexBufferOffset;
    uint vertexCount;
    
    uint triangleBufferIndex;
    uint triangleBufferOffset;
    uint triangleCount;
    
    uint perMeshBufferIndex;
    uint perMeshBufferOffset;
    
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
    transform modelTransform;
    uint globalMaterialOffset;
    uint jointIndex;
    uint jointOffset;
};

static const uint VISIBLE_FLAG_BIT = 1 << 0;
static const uint NOT_VISIBLE_FLAG_BIT = 1 << 1;
static const uint VISIBLE_CURRENT_FRAME_FLAG_BIT = 1 << 2;
static const uint NOT_VISIBLE_CURRENT_FRAME_FLAG_BIT = 1 << 3;

struct command
{
    uint instanceIndex;
    uint instanceOffset;
    
    uint meshletIndex;
    uint meshletOffset1;
    uint meshletOffset2;
    uint meshletOffset3;
    uint meshletOffset4;
    
    uint visabilityBit;
};

struct perMeshAttributes
{   
    float bsRadius;
    float3 bsCenter;
    uint isSkinned;
    float4x4 meshLocalTransform;
    float4x4 meshGlobalTransform;
    float3x3 meshLocalNormal;
    float3x3 meshGlobalNormal;
};

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
    uint width;
    uint height;
};

struct meshOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    
    nointerpolation uint albedoIndex : TEXCOORD1;
    nointerpolation uint normalIndex : TEXCOORD2;
    nointerpolation uint metallicRoughnessIndex : TEXCOORD3;

    float3 normal : TANGENT0;
    float4 tangent : TANGENT1;
    nointerpolation float4 rotation : TANGENT2;
    float3 cameraPos : POSITIONT1;
    float3 cameraFront : POSITIONT2;
    float3 worldPos : POSITIONT3;
};

// SSBO START.

// Vertex attributes.
// 0 - 10.
// Common attributes always present.
StructuredBuffer<float4> positionBuffer[] : register(t0, space0);
StructuredBuffer<float4> normalBuffer[] : register(t1, space0);
StructuredBuffer<float4> tangenBuffer[] : register(t2, space0);
// Optional only for animated meshes.
StructuredBuffer<uint4> jointIndexBuffer[] : register(t3, space0);
StructuredBuffer<float4> weightBuffer[] : register(t4, space0);

// Buffers.
// 11 - 20.
StructuredBuffer<perInstanceAttr> perInstanceBuffer[] : register(t11, space0);
StructuredBuffer<command> commandOpaqueBuffer[] : register(t12, space0);
StructuredBuffer<command> commandAccumilationBuffer : register(t13, space0);
StructuredBuffer<uint> vertexIndexBuffer[] : register(t14, space0);
StructuredBuffer<uint> primitiveBuffer[] : register(t15, space0);
StructuredBuffer<meshlet> meshletBuffer[] : register(t16, space0);
StructuredBuffer<float4x4> jointBuffer[] : register(t17, space0);
StructuredBuffer<perMeshAttributes> perMeshBuffer[] : register(t18, space0);

// SSBO END.

// UBO START.

ConstantBuffer<perDrawData> drawData : register(b19, space0);

// UBO END.

// MATERIALS START.
// 21 - 30.                               
Texture2D materials[] : register(t21, space0);
SamplerState materialsSampler[] : register(s21, space0);
Texture2D accum : register(t22, space0);
SamplerState accumSampler : register(s22, space0);
Texture2D reveal : register(t23, space0);
SamplerState revealSampler : register(s23, space0);
Texture2D<float> hzbChain[] : register(t24, space0);

// MATERIALS END.

float3 getPostition(uint index, uint offset)
{
    float4 pos = positionBuffer[index][offset];
    
    return pos.xyz;
}

float2 getTexCoords(uint index, uint offset)
{
    float4 pos = positionBuffer[index][offset];
    float4 normal = normalBuffer[index][offset];
    
    return float2(pos.w, normal.w);
}

float3 getNormal(uint index, uint offset)
{
    float4 normal = normalBuffer[index][offset];
    
    return normal.xyz;
}

float4 getTangent(uint index, uint offset)
{
    return tangenBuffer[index][offset];
}

uint4 getJointIndices(uint index, uint offset)
{
    return jointIndexBuffer[index][offset];
}

float4 getWieghts(uint index, uint offset)
{
    return weightBuffer[index][offset];
}

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

uint getMeshletOffset(command cmd, uint lodLevel)
{
    uint result;
    
    switch (lodLevel)
    {
        case 2:
            result = cmd.meshletOffset2;
            break;
        case 3:
            result = cmd.meshletOffset3;
            break;
        case 4:
            result = cmd.meshletOffset4;
            break;
        default:
            result = cmd.meshletOffset1;
            break;
    }
    
    return result;
}

uint selectLodLevel(
    perMeshAttributes meshAttrs,
    perDrawData drawData,
    transform modelTransform
)
{
    float uniformScale = max(length(meshAttrs.meshGlobalTransform[0].xyz), max(length(meshAttrs.meshGlobalTransform[1].xyz), length(meshAttrs.meshGlobalTransform[2].xyz)));

    meshAttrs.bsCenter = mul(meshAttrs.meshGlobalTransform, float4(meshAttrs.bsCenter, 1.0f)).xyz;
    meshAttrs.bsRadius *= uniformScale;
    
    uniformScale = max(modelTransform.scale.x, max(modelTransform.scale.y, modelTransform.scale.z));
    
    meshAttrs.bsCenter = transformPoint(modelTransform, meshAttrs.bsCenter);
    meshAttrs.bsRadius *= uniformScale;
    
    // Get viewSpace of the center.
    float4 vsCenter = mul(drawData.view, float4(meshAttrs.bsCenter, 1.0f));
    
    // Calculate view space for second point that is at the sphere border on y axis.
    float4 vsBorder = float4(vsCenter.x, vsCenter.y + meshAttrs.bsRadius, vsCenter.zw);

    // To NDC for both.
    float4 clipCenter = mul(drawData.projection, vsCenter);
    float4 clipBorder = mul(drawData.projection, vsBorder);

    float2 ndcCenter = clipCenter.xy / clipCenter.w;
    float2 ndcBorder = clipBorder.xy / clipBorder.w;
    
    float ndcRadius = length(ndcCenter - ndcBorder);
    
    if (ndcRadius * 2 >= 0.2f)   // ~10% of screen.
        return 1;
    
    if (ndcRadius * 2 >= 0.1f)   // ~5% of screen.
        return 2;
    
    if (ndcRadius * 2 >= 0.05f)  // ~2.5% of screen.
        return 3;
    
    return 4; // < 2.5% of screen.
}

// Back face cone culling.
bool isFrontfaceMeshlet(perDrawData drawData, transform modelTransform, float3 coneAxis, float3 coneApex, float coneCutoff)
{
    if (coneAxis.x == 0 && coneAxis.y == 0 && coneAxis.z == 0)
        return true;
    
    if (coneCutoff == 1.0f)
        return true;
    
    float3 worldConeApex = transformPoint(modelTransform, coneApex);
    float3 worldConeAxis = normalize(rotate(modelTransform.rotation, coneAxis));
    float3 viewDir = normalize(worldConeApex - drawData.cameraPos);
    
    return dot(viewDir, worldConeAxis) < coneCutoff;
}

bool isInFrustum(perDrawData drawData, transform modelTransform, float3 bsCenter, float bsRadius)
{
    float3 worldCenter = transformPoint(modelTransform, bsCenter);
    
    float scale = max(0.001f, modelTransform.scale.x);
    scale = max(scale, modelTransform.scale.y);
    scale = max(scale, modelTransform.scale.z);

    float worldRadius = scale * bsRadius;
    
    bool front = dot(worldRadius * drawData.cameraFrustum.worldFrontN + worldCenter, drawData.cameraFrustum.worldFrontN) - drawData.cameraFrustum.frontDistance > 0;
    bool back = dot(worldRadius * drawData.cameraFrustum.worldBackN + worldCenter, drawData.cameraFrustum.worldBackN) - drawData.cameraFrustum.backDistance > 0;
    bool right = dot(worldRadius * drawData.cameraFrustum.worldRightN + worldCenter, drawData.cameraFrustum.worldRightN) - drawData.cameraFrustum.rightDistance > 0;
    bool left = dot(worldRadius * drawData.cameraFrustum.worldLeftN + worldCenter, drawData.cameraFrustum.worldLeftN) - drawData.cameraFrustum.leftDistance > 0;
    bool top = dot(worldRadius * drawData.cameraFrustum.worldTopN + worldCenter, drawData.cameraFrustum.worldTopN) - drawData.cameraFrustum.topDistance > 0;
    bool bottom = dot(worldRadius * drawData.cameraFrustum.worldBottomN + worldCenter, drawData.cameraFrustum.worldBottomN) - drawData.cameraFrustum.bottomDistance > 0;
    
    return front && back && right && left && top && bottom;
}


// meshopt stores the triangle offset in bytes since it stores the
// triangle indices as 3 consecutive bytes. 
//
// Since we repacked those 3 bytes to a 32-bit uint, our offset is now
// aligned to 4 and we can easily grab it as a uint without any 
// additional offset math.
uint3 unpackUint(uint packed)
{
    uint3 result;
    
    result.x = (packed >> 0) & 0xFF;
    result.y = (packed >> 8) & 0xFF;
    result.z = (packed >> 16) & 0xFF;
    
    return result;
}

bool isBackface(perDrawData drawData, transform modelTransform, float3 v1, float3 v2, float3 v3)
{
    v1 = transformPoint(modelTransform, v1);
    v2 = transformPoint(modelTransform, v2);
    v3 = transformPoint(modelTransform, v3);
    
    float3 normal = cross(v2 - v1, v3 - v1);
    
    float3 center = (v1 + v2 + v3) / 3;
    
    return dot(normal, drawData.cameraPos - center) < 0;
}

float3x3 calculateTBN(float4 quat, float4 tangent, float3 normal)
{
    float3 T = normalize(rotate(quat, float3(tangent.xyz)));
    float3 N = normalize(rotate(quat, normal));
    
    T = normalize(T - dot(T, N) * N);
    
    float3 B = tangent.w * cross(N, T);
    
    return transpose(float3x3(T, B, N));
}

// it's just approxiamtion the formula itself quite complex and using radiant flux that we do not have.
float3 lightRadiance(float3 lightColor, float distance)
{
    float attenuation = 1.0 / max(distance * distance, 1.0f);
    float3 radiance = mul(lightColor, attenuation);

    return radiance;
}

// baseReflectivity F0 really hard to calculate, so we use trick like that.
// for dielectrics we return approximation 0.04, for mettalic we return mix.
float3 baseReflectivity(float3 albedo, float metalic)
{
    float3 f0 = float3(0.04f, 0.04f, 0.04f); // base reflectivity of dielectrics.
    f0 = lerp(f0, albedo, metalic);

    return f0;
}

// The Fresnel equation describes the ratio of surface reflection at different surface angles.
float3 fresnelSchlick(float cosTheta, float3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// distributionGGX - approximates the amount the surface's microfacets are aligned to the halfway vector, influenced by the roughness of the surface; this is the primary function approximating the microfacets.
float distributionGGX(float3 n, float3 h, float roughness)
{
    // not sure why we use roughness^4.
    float a = roughness * roughness;
    float a2 = a * a;
    float nDotH = max(dot(n, h), 0.0);
    float nDotH2 = nDotH * nDotH;
    
    float num = a2;
    float denom = (nDotH2 * (a2 - 1.0) + 1.0);
    float p = PI;
    denom = p * denom * denom;
    
    return num / max(denom, 0.001);
}

// geometrySchlickGGX - describes the self-shadowing property of the microfacets. When a surface is relatively rough, the surface's microfacets can overshadow other microfacets reducing the light the surface reflects.
float geometrySchlickGGX(float nDotV, float roughness)
{
    // remap roughness.
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = nDotV;
    float denom = nDotV * (1.0 - k) + k;
    
    return num / max(denom, 0.001);
}

// geometrySmith - is used for approximation of geometrySchlickGGX.
float geometrySmith(float3 n, float3 v, float3 l, float roughness)
{
    float nDotV = max(dot(n, v), 0.0);
    float nDotL = max(dot(n, l), 0.0);
    float ggx2 = geometrySchlickGGX(nDotV, roughness);
    float ggx1 = geometrySchlickGGX(nDotL, roughness);
    
    return ggx1 * ggx2;
}

float3 toRGB(float3 color)
{
    return pow(color, GAMMA);
}

float3 toSRGB(float3 color)
{
    return pow(color, 1.0f / GAMMA);
}

// Skins all vertex attributes if needed.
skinnedVertex skinVertex(perInstanceAttr perInst, perMeshAttributes perMesh, uint index, uint offset, uint weightOffset, uint weightIndex)
{
    skinnedVertex result;
    
    result.position = getPostition(index, offset);
    result.textureCoords = getTexCoords(index, offset);
    result.normal = getNormal(index, offset);
    result.tangent = getTangent(index, offset);
    
    if (!perMesh.isSkinned)
    {
        result.position = mul(perMesh.meshGlobalTransform, float4(result.position, 1.0f)).xyz;
        result.normal = normalize(mul(perMesh.meshGlobalNormal, result.normal));
        result.tangent = float4(normalize(mul((float3x3) perMesh.meshGlobalTransform, result.tangent.xyz).xyz), result.tangent.w);

        return result;
    }
    
    float4 weights = getWieghts(weightIndex, weightOffset);
    uint4 jointIndices = getJointIndices(weightIndex, weightOffset);
    
    float4 bindPos = float4(result.position, 1.0f);
    float4 skinnedPos = float4(0, 0, 0, 0);
        
    skinnedPos += weights[0] * mul(jointBuffer[perInst.jointIndex][perInst.jointOffset + jointIndices[0]], bindPos);
    skinnedPos += weights[1] * mul(jointBuffer[perInst.jointIndex][perInst.jointOffset + jointIndices[1]], bindPos);
    skinnedPos += weights[2] * mul(jointBuffer[perInst.jointIndex][perInst.jointOffset + jointIndices[2]], bindPos);
    skinnedPos += weights[3] * mul(jointBuffer[perInst.jointIndex][perInst.jointOffset + jointIndices[3]], bindPos);
    
    result.position = mul(perMesh.meshGlobalTransform, float4(skinnedPos.xyz, 1.0f)).xyz;
    
    result.normal = normalize(
            weights[0] * mul((float3x3) jointBuffer[perInst.jointIndex][perInst.jointOffset + jointIndices[0]], result.normal) +
            weights[1] * mul((float3x3) jointBuffer[perInst.jointIndex][perInst.jointOffset + jointIndices[1]], result.normal) +
            weights[2] * mul((float3x3) jointBuffer[perInst.jointIndex][perInst.jointOffset + jointIndices[2]], result.normal) +
            weights[3] * mul((float3x3) jointBuffer[perInst.jointIndex][perInst.jointOffset + jointIndices[3]], result.normal)
        );
    
    result.normal = normalize(mul(perMesh.meshGlobalNormal, result.normal));

    float3 skinnedTangent = normalize(
            weights[0] * mul((float3x3) jointBuffer[perInst.jointIndex][perInst.jointOffset + jointIndices[0]], result.tangent.xyz) +
            weights[1] * mul((float3x3) jointBuffer[perInst.jointIndex][perInst.jointOffset + jointIndices[1]], result.tangent.xyz) +
            weights[2] * mul((float3x3) jointBuffer[perInst.jointIndex][perInst.jointOffset + jointIndices[2]], result.tangent.xyz) +
            weights[3] * mul((float3x3) jointBuffer[perInst.jointIndex][perInst.jointOffset + jointIndices[3]], result.tangent.xyz)
        );
    
    result.tangent = float4(
        normalize(mul((float3x3) perMesh.meshGlobalTransform, skinnedTangent)).xyz,
        result.tangent.w
    );
    
    return result;
}