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
};

struct perInstanceAttr
{
    uint albedoIndeex;
    uint roughnessIndex;
    uint normalIndex;
    uint metalicIndex;
    uint aoIndex;
    
    float3 bsCenter;
    float bsRadius;
    
    float4x4 modelMatrix;
};

struct meshletToInstance
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
StructuredBuffer<meshletToInstance> meshletToInstanceBuffer : register(t2, space0);
StructuredBuffer<uint> vertexIndexBuffer[] : register(t3, space0);
StructuredBuffer<uint> primitiveBuffer[] : register(t4, space0);
StructuredBuffer<meshlet> meshletBuffer[] : register(t5, space0);

// SSBO END.

// TEXTURES START.

Texture2D albedo[] : register(t6, space0);
SamplerState albedoSamplers[] : register(s6, space0);

// TEXTURES END.

// DescriptorSets END.

// Push constant START.

struct pushConstant
{
    uint meshletCount;
    float3 cameraPos;
    float4x4 viewProjection;
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

uint getMeshletOffset(uint lodLevel, uint dtid)
{
    uint result;
    
    switch (lodLevel)
    {
        case 2:
            result = meshletToInstanceBuffer[dtid].meshletOffset2;
            break;
        case 3:
            result = meshletToInstanceBuffer[dtid].meshletOffset3;
            break;
        case 4:
            result = meshletToInstanceBuffer[dtid].meshletOffset4;
            break;
        default:
            result = meshletToInstanceBuffer[dtid].meshletOffset1;
            break;
    }
    
    return result;
}

uint selectLodLevel(float4x4 model, float4x4 viewProjection, float3 bsCenter, float bsRadius)
{
    // TODO: implement lod selection.
    return 4;
}

[numthreads(THREADS_COUNT, 1, 1)]
void asmain(
    uint gtid : SV_GroupThreadID,
    uint dtid : SV_DispatchThreadID,
    uint gid : SV_GroupID
)
{
    const uint maxUint = 4294967295;
    
    float visible = dtid < push.meshletCount;
  
    // Not overdraw.
    if (visible)
    {
        uint perInstanceIndex = meshletToInstanceBuffer[dtid].instanceIndex;
        uint perInstanceOffset = meshletToInstanceBuffer[dtid].instanceOffset;
    
        perInstanceAttr instanceAttr = perInstanceBuffer[perInstanceIndex][perInstanceOffset];
        uint selectedLod = selectLodLevel(instanceAttr.modelMatrix, push.viewProjection, instanceAttr.bsCenter, instanceAttr.bsRadius);
        uint meshletOffset = getMeshletOffset(selectedLod, dtid);
    
        // Still have meshlets for that lodLevel.
        visible = meshletOffset != maxUint;
        if (visible)
        {
            // TODO: add culling.
            visible = true;
            
            uint index = WavePrefixCountBits(visible);
        
            payload.perInstanceIndex[index] = perInstanceIndex;
            payload.perInstanceOffset[index] = perInstanceOffset;
     
            payload.lodLevel[index] = selectedLod;
        
            payload.meshletIndex[index] = meshletToInstanceBuffer[dtid].meshletIndex;
            payload.meshletOffset[index] = meshletOffset;
        }
    
    }
        
    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, payload);
}
// TS END.

// MS START.

struct meshOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
};

[outputtopology("triangle")]
[numthreads(THREADS_COUNT, 1, 1)]
void msmain(
                 uint gtid : SV_GroupThreadID,
                 uint gid : SV_GroupID,
    in payload MeshShaderPayload payload,
    out indices uint3 triangles[THREADS_COUNT],
    out vertices meshOutput vertices[THREADS_COUNT])
{
    meshlet mesh = meshletBuffer[payload.meshletIndex[gid]][payload.meshletOffset[gid]];
    perInstanceAttr instanceAttr = perInstanceBuffer[payload.perInstanceIndex[gid]][payload.perInstanceOffset[gid]];
    
    SetMeshOutputCounts(mesh.vertexCount, mesh.triangleCount);
       
    if (gtid < mesh.triangleCount)
    {
        // meshopt stores the triangle offset in bytes since it stores the
        // triangle indices as 3 consecutive bytes. 
        //
        // Since we repacked those 3 bytes to a 32-bit uint, our offset is now
        // aligned to 4 and we can easily grab it as a uint without any 
        // additional offset math.
        uint packed = primitiveBuffer[mesh.triangleBufferIndex][mesh.triangleBufferOffset + gtid];
        
        uint vIdx0 = (packed >> 0) & 0xFF;
        uint vIdx1 = (packed >> 8) & 0xFF;
        uint vIdx2 = (packed >> 16) & 0xFF;
        
        triangles[gtid] = uint3(vIdx0, vIdx1, vIdx2);
    }

    if (gtid < mesh.vertexCount)
    {
        uint vertexIndex = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + gtid] + mesh.vertexBufferOffset;

        vertices[gtid].position = mul(push.viewProjection, mul(instanceAttr.modelMatrix, float4(vertexBuffer[mesh.vertexBufferIndex][vertexIndex].position, 1.0)));
        
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