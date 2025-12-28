//  dxc -T ms_6_9 -E msmain -spirv -fspv-target-env=vulkan1.3 -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkMeshMs.spv vkMesh.hlsl
//  dxc -T ps_6_9 -E psmain -spirv -Fo vkMeshPs.spv vkMesh.hlsl
//  dxc -T as_6_9 -E asmain -spirv -fspv-target-env=vulkan1.3 -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshAs.spv vkMesh.hlsl
//  add -fspv-reflect flag only for debug.
#ifdef __spirv__
#define DEFINE_AS_PUSH_CONSTANT [[vk::push_constant]]
#else
#define DEFINE_AS_PUSH_CONSTANT
#endif

#define THREADS_COUNT 32

// INPUT START.

// DescriptorSet START.

struct vertex
{
    float3 position;
    float _pad0;

    float2 textureCoords;
    float2 _pad1;

    float3 normal;
    float _pad2;

    float3 tangent;
    float _pad3;
};

struct meshletBounds
{
	/* bounding sphere, useful for frustum and occlusion culling */
    float3 center;
    float radius;

	/* normal cone, useful for backface culling */
    float3 coneApex;
    float3 coneAxis;
    float coneCutoff; /* = cos(angle/2) */

    float coneAxisS8;
    float coneCutoffS8;
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
    
    uint albedoIndeex;
    uint roughnessIndex;
    uint normalIndex;
    uint metalicIndex;
    uint aoIndex;
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

struct perDrawData
{
    float3 cameraPos;
    float3 cameraFront;
    float3 cameraUp;
    float4x4 view;
    float4x4 projection;
    float4x4 viewProjection;
};

ConstantBuffer<perDrawData> drawData : register(b6, space0);

// UBO END.

// TEXTURES START.

Texture2D albedo[] : register(t7, space0);
SamplerState albedoSamplers[] : register(s7, space0);

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

bool isInFrustum(float4x4 model, float3 bsCenter, float bsRadius)
{
    bool result = false;
    
    return result;
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
            // TODO: add culling.
            visible = true;
            
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
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
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

        vertices[gtid].position = mul(drawData.viewProjection, mul(instanceAttr.modelMatrix, float4(vertexBuffer[mesh.vertexBufferIndex][vertexIndex].position, 1.0)));
        
        float4 color = float4(
            float(payload.meshletOffset[gid] & 1),
            float(payload.meshletOffset[gid] & 3) / 4,
            float(payload.meshletOffset[gid] & 7) / 8,
            payload.perInstanceOffset[gid] % 2 == 0 ? 0.5f : 1.0f
        );
        
        vertices[gtid].color = color;
        vertices[gtid].uv = vertexBuffer[mesh.vertexBufferIndex][vertexIndex].textureCoords;
    }
}

// MESH SHADER END.

// PIXEL SHADER START.

float4 psmain(meshOutput input) : SV_TARGET
{
    return input.color;
}

// PIXEL SHADER END.
