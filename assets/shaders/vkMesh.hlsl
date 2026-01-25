//  dxc -T ms_6_9 -E msmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkMeshMs.spv vkMesh.hlsl
//  dxc -T ps_6_9 -E psmain -spirv -Fo -fvk-use-scalar-layout vkMeshPs.spv vkMesh.hlsl
//  dxc -T as_6_9 -E asmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshAs.spv vkMesh.hlsl
//  add -fspv-reflect flag only for debug.
#ifdef __spirv__
#define DEFINE_AS_PUSH_CONSTANT [[vk::push_constant]]
#else
#define DEFINE_AS_PUSH_CONSTANT
#endif

#define THREADS_COUNT 32

#define PI 3.14159265359f

#define GAMMA 2.2f

// INPUT START.

// DescriptorSet START.

struct vertex
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

struct perInstanceAttr
{
    float3 bsWorldCenter;
    float bsWorldRadius;
    float4x4 modelMatrix;
    float3x3 normalMatrix;

    uint albedoIndex;
    uint normalIndex;
    uint metallicRoughnesIndex;
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
    float3 cameraPos;
    uint useDebugCamera;
    float3 cameraFront;
    float3 cameraUp;
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
    frustum cameraFrustum;
    float deltaTime;
};

ConstantBuffer<perDrawData> drawData : register(b6, space0);

// UBO END.

// TEXTURES START.

Texture2D materials[] : register(t7, space0);
SamplerState materialsSampler[] : register(s7, space0);

// TEXTURES END.

// DescriptorSets END.

// Push constant START.
struct pushConstant
{
    uint commandBufferOffset;
    uint meshletCount;
};

DEFINE_AS_PUSH_CONSTANT
pushConstant push;

// Push constant END.

// INPUT END.

// TS START.

struct MeshShaderPayload
{
    uint meshletIndex[THREADS_COUNT];
    uint meshletOffset[THREADS_COUNT];
    uint perInstanceIndex[THREADS_COUNT];
    uint perInstanceOffset[THREADS_COUNT];
    uint lodLevel[THREADS_COUNT];
};

groupshared MeshShaderPayload payload;

uint getMeshletOffset(uint lodLevel, uint idx)
{
    uint result;
    
    switch (lodLevel)
    {
        case 2:
            result = commandBuffer[idx].meshletOffset2;
            break;
        case 3:
            result = commandBuffer[idx].meshletOffset3;
            break;
        case 4:
            result = commandBuffer[idx].meshletOffset4;
            break;
        default:
            result = commandBuffer[idx].meshletOffset1;
            break;
    }
    
    return result;
}

uint selectLodLevel(float4x4 model, float3 bsWorldCenter, float bsWorldRadius)
{
    // Get viewSpace of the center.
    float4 vsCenter = mul(drawData.view, float4(bsWorldCenter, 1.0f));
    
    // Calculate view space for second point that is at the sphere border on y axis.
    float4 vsBorder = float4(vsCenter.x, vsCenter.y + bsWorldRadius, vsCenter.zw);

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
bool isFrontfaceMeshlet(float4x4 model, float3x3 normalMatrix, float3 coneAxis, float3 coneApex, float coneCutoff)
{
    if (coneAxis.x == 0 && coneAxis.y == 0 && coneAxis.z == 0)
        return true;
    
    if (coneCutoff == 1.0f)
        return true;
    
    float3 worldConeApex = mul(model, float4(coneApex, 1.0f)).xyz;
    float3 worldConeAxis = normalize(mul(normalMatrix, coneAxis));
    float3 viewDir = normalize(worldConeApex - drawData.cameraPos);
    
    return dot(viewDir, worldConeAxis) < coneCutoff;
}

bool isInFrustum(float4x4 model, float3 bsCenter, float bsRadius)
{
    float3 worldCenter = mul(model, float4(bsCenter, 1.0f)).xyz;
    
    float scale = length(model[0]);
    float worldRadius = scale * bsRadius;
    
    bool front = dot(worldRadius * drawData.cameraFrustum.worldFrontN + worldCenter, drawData.cameraFrustum.worldFrontN) - drawData.cameraFrustum.frontDistance > 0;
    bool back = dot(worldRadius * drawData.cameraFrustum.worldBackN + worldCenter, drawData.cameraFrustum.worldBackN) - drawData.cameraFrustum.backDistance > 0;
    bool right = dot(worldRadius * drawData.cameraFrustum.worldRightN + worldCenter, drawData.cameraFrustum.worldRightN) - drawData.cameraFrustum.rightDistance > 0;
    bool left = dot(worldRadius * drawData.cameraFrustum.worldLeftN + worldCenter, drawData.cameraFrustum.worldLeftN) - drawData.cameraFrustum.leftDistance > 0;
    bool top = dot(worldRadius * drawData.cameraFrustum.worldTopN + worldCenter, drawData.cameraFrustum.worldTopN) - drawData.cameraFrustum.topDistance > 0;
    bool bottom = dot(worldRadius * drawData.cameraFrustum.worldBottomN + worldCenter, drawData.cameraFrustum.worldBottomN) - drawData.cameraFrustum.bottomDistance > 0;
    
    return front && back && right && left && top && bottom;
}

[numthreads(THREADS_COUNT, 1, 1)]
void asmain(
    uint gtid : SV_GroupThreadID,
    uint dtid : SV_DispatchThreadID,
    uint gid : SV_GroupID
)
{
    const uint maxUint = 4294967295;
    
    float visible = false;
    
    // Not overdraw.
    if (dtid < push.meshletCount)
    {
        uint perInstanceIndex = commandBuffer[dtid + push.commandBufferOffset].instanceIndex;
        uint perInstanceOffset = commandBuffer[dtid + push.commandBufferOffset].instanceOffset;
    
        perInstanceAttr instanceAttr = perInstanceBuffer[perInstanceIndex][perInstanceOffset];
        uint selectedLod = selectLodLevel(instanceAttr.modelMatrix, instanceAttr.bsWorldCenter, instanceAttr.bsWorldRadius);
        uint meshletIdx = commandBuffer[dtid + push.commandBufferOffset].meshletIndex;
        uint meshletOffset = getMeshletOffset(selectedLod, dtid + push.commandBufferOffset);
    
        // Still have meshlets for that lodLevel.
        if (meshletOffset != maxUint)
        {
            meshlet mesh = meshletBuffer[meshletIdx][meshletOffset];
            
            visible =
                isFrontfaceMeshlet(instanceAttr.modelMatrix, (float3x3) instanceAttr.normalMatrix, mesh.bounds.coneAxis, mesh.bounds.center, mesh.bounds.coneCutoff) &&
                isInFrustum(instanceAttr.modelMatrix, mesh.bounds.center, mesh.bounds.radius);
            
            if (visible)
            {
                uint index = WavePrefixCountBits(visible);
        
                payload.perInstanceIndex[index] = perInstanceIndex;
                payload.perInstanceOffset[index] = perInstanceOffset;
     
                payload.lodLevel[index] = selectedLod;
        
                payload.meshletIndex[index] = meshletIdx;
                payload.meshletOffset[index] = meshletOffset;
            }
        }
    }
    
    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, payload);
}
// TS END.

// MS START.

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

struct meshOutput
{
    float3 tangentWorldPos : TANGENT0;
    float3 tangentCameraPos : TANGENT1;
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    nointerpolation uint albedoIndex : TEXCOORD1;
    nointerpolation uint normalIndex : TEXCOORD2;
    nointerpolation uint metalicRoughnesIndex : TEXCOORD3;
};

struct meshletPrimitiveOut
{
    bool cullPrimitive : SV_CULLPRIMITIVE;
};

bool isBackface(float4x4 model, float3 v1, float3 v2, float3 v3)
{
    v1 = mul(model, float4(v1, 1.0f)).xyz;
    v2 = mul(model, float4(v2, 1.0f)).xyz;
    v3 = mul(model, float4(v3, 1.0f)).xyz;
    
    float3 normal = cross(v2 - v1, v3 - v1);
    
    float3 center = (v1 + v2 + v3) / 3;
    
    return dot(normal, drawData.cameraPos - center) < 0;
}

float3x3 calculateTBN(float3x3 normalMatrix, vertex v)
{
    float3 T = normalize(mul(normalMatrix, float3(v.tangent.xyz)));
    float3 N = normalize(mul(normalMatrix, v.normal));
    
    T = normalize(T - dot(T, N) * N);
    
    float3 B = v.tangent.w * cross(N, T);
    
    return float3x3(T, B, N);
}

[outputtopology("triangle")]
[numthreads(THREADS_COUNT, 1, 1)]
void msmain(
                 uint gtid : SV_GroupThreadID,
                 uint gid : SV_GroupID,
    in payload MeshShaderPayload payload,
    out indices uint3 triangles[THREADS_COUNT],
    out vertices meshOutput vertices[THREADS_COUNT],
    out primitives meshletPrimitiveOut primitives[THREADS_COUNT])
{
    meshlet mesh = meshletBuffer[payload.meshletIndex[gid]][payload.meshletOffset[gid]];
    perInstanceAttr instanceAttr = perInstanceBuffer[payload.perInstanceIndex[gid]][payload.perInstanceOffset[gid]];
    
    SetMeshOutputCounts(mesh.vertexCount, mesh.triangleCount);
        
    if (gtid < mesh.triangleCount)
    {
        uint packed = primitiveBuffer[mesh.triangleBufferIndex][mesh.triangleBufferOffset + gtid];
         
        uint3 unpacked = unpackUint(packed);
        
        triangles[gtid] = unpacked;
        
        uint idx1 = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + unpacked.x] + mesh.vertexBufferOffset;
        uint idx2 = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + unpacked.y] + mesh.vertexBufferOffset;
        uint idx3 = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + unpacked.z] + mesh.vertexBufferOffset;
        
        primitives[gtid].cullPrimitive = isBackface(
                instanceAttr.modelMatrix,
                vertexBuffer[mesh.vertexBufferIndex][idx1].position,
                vertexBuffer[mesh.vertexBufferIndex][idx2].position,
                vertexBuffer[mesh.vertexBufferIndex][idx3].position
            );
    }

    if (gtid < mesh.vertexCount)
    {
        uint vertexIndex = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + gtid] + mesh.vertexBufferOffset;
        
        vertex v = vertexBuffer[mesh.vertexBufferIndex][vertexIndex];
        float4 worldPos = mul(instanceAttr.modelMatrix, float4(v.position, 1.0));
        
        vertices[gtid].position = mul(drawData.useDebugCamera ? drawData.debugViewProjection : drawData.viewProjection, worldPos);
        
        float3x3 TBN = calculateTBN(instanceAttr.normalMatrix, v);
        
        vertices[gtid].uv = vertexBuffer[mesh.vertexBufferIndex][vertexIndex].textureCoords;
        vertices[gtid].albedoIndex = instanceAttr.albedoIndex;
        vertices[gtid].normalIndex = instanceAttr.normalIndex;
        vertices[gtid].metalicRoughnesIndex = instanceAttr.metallicRoughnesIndex;
        vertices[gtid].tangentCameraPos = mul(TBN, drawData.cameraPos);
        vertices[gtid].tangentWorldPos = mul(TBN, worldPos.xyz);
    }
}

// MESH SHADER END.

// PIXEL SHADER START.

// it's just approxiamtion the formula itself quite complex and using radiant flux that we do not have.
float3 lightRadiance(float3 lightColor, float distance)
{
    float attenuation = 1.0 / (distance * distance);
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
    denom = PI * denom * denom;
    
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

// All calculations are made in tangent space.
float4 psmain(meshOutput input) : SV_TARGET
{
    float4 metalicRoughnes = materials[input.metalicRoughnesIndex].Sample(materialsSampler[input.metalicRoughnesIndex], input.uv);

    float4 albedo = materials[input.albedoIndex].Sample(materialsSampler[input.albedoIndex], input.uv);
    float3 normal = normalize(materials[input.normalIndex].Sample(materialsSampler[input.normalIndex], input.uv).rgb * 2.0f - 1.0f);
    float metalic = metalicRoughnes.b;
    float roughnes = metalicRoughnes.g;
    
    albedo = float4(toRGB(albedo.rgb), albedo.a);
    
    float3 fromFragmentToCamera = normalize(input.tangentCameraPos - input.tangentWorldPos);
    
    float3 lightPositions[4] =
    {
        float3(0.0f, 0.0f, 2.0f),
        float3(0.0f, 0.0f, -2.0f),
        float3(2.0f, 0.0f, 0.0f),
        float3(-2.0f, 0.0f, 0.0f),
    };
    
    float3 lightColors[4] =
    {
        float3(32.0f, 32.0f, 32.0f),
        float3(42.0f, 12.0f, 12.0f),
        float3(12.0f, 12.0f, 42.0f),
        float3(12.0f, 45.0f, 12.0f),
    };
    
    // render equation.
    float3 l0 = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i)
    {
        float3 lightPos = lightPositions[i];
        
        float3 lightColor = lightColors[i];

        float3 fromFragmentToLight = normalize(lightPos - input.tangentWorldPos);
        float3 halfway = normalize(fromFragmentToLight + fromFragmentToCamera);

        // radiance per per light source.
        float3 radiance = lightRadiance(lightColor, length(lightPos - input.tangentWorldPos));

        // Cook-Torrance BRDF
        float d = distributionGGX(normal, halfway, roughnes);
        float g = geometrySmith(normal, fromFragmentToCamera, fromFragmentToLight, roughnes);
        float3 f = fresnelSchlick(max(dot(halfway, fromFragmentToCamera), 0.0), baseReflectivity(albedo.rgb, metalic));

        float3 numerator = d * f * g;
        float denominator = 4.0 * max(dot(normal, fromFragmentToCamera), 0.0) * max(dot(normal, fromFragmentToLight), 0.0) + 0.0001;
        // + 0.0001 to prevent divide by zero
        float3 specular = numerator / denominator;

        // kS is equal to Fresnel
        float3 kS = f;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metalic;

        // scale light by nDotL
        float nDotL = max(dot(normal, fromFragmentToLight), 0.0);

        // add to outgoing radiance Lo
        l0 += (kD * albedo.rgb / PI + specular) * radiance * nDotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }


    // ambient lighting part, removed for now.
    // note that in future you need to replace that ambient light with some Voxel Cone Tracing for reflections.
    // for now we just use AO texture.
    float3 ambient = mul(float3(0.03f, 0.03f, 0.03f), albedo.rgb);
    
    float3 color = l0;

    // HDR tonemapping
    color = color / (color + float3(1.0f, 1.0f, 1.0f));
    
    color = toSRGB(color);
    
    return float4(color.rgb, albedo.a);
}

// PIXEL SHADER END.
