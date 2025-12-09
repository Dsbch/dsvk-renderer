//  dxc -T ms_6_9 -E msmain -spirv -fspv-target-env=vulkan1.3 -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkMeshMs.spv vkMesh.hlsl
//  dxc -T ps_6_9 -E psmain -spirv -Fo vkMeshPs.spv vkMesh.hlsl
//  dxc -T as_6_9 -E asmain -spirv -Fo vkMeshAs.spv vkMesh.hlsl
//  add -fspv-reflect flag only for debug.
#ifdef __spirv__
#define DEFINE_AS_PUSH_CONSTANT [[vk::push_constant]]
#else
#define DEFINE_AS_PUSH_CONSTANT
#endif

// INPUT START.

// DescriptorSet START.

struct vertex 
{
    float3 position;
    float  _pad0;

    float2 textureCoords;
    float2 _pad1;

    float3 normal;
    float  _pad2;

    float3 tangent;
    float  _pad3;
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
    
    
    float4x4 modelMatrix;
};

struct meshletToInstance
{
    uint instanceIndex;
    uint instanceOffset;
    
    uint meshletIndex;
    uint meshletOffset;
};

StructuredBuffer<vertex> vertexBuffer[] : register(t0, space0);
StructuredBuffer<uint> vertexIndexBuffer[] : register(t1, space0);
StructuredBuffer<uint> primitiveBuffer[] : register(t2, space0);
StructuredBuffer<meshlet> meshletBuffer[] : register(t3, space0);
StructuredBuffer<perInstanceAttr> perInstanceBuffer[] : register(t4, space0);
StructuredBuffer<meshletToInstance> meshletToInstanceBuffer : register(t5, space0);

Texture2D albedo[] : register(t6, space0);
SamplerState albedoSamplers[] : register(s6, space0);

// DescriptorSets END.

// Push constant START.

struct pushConstants
{
    float4x4 viewProjection;
};

DEFINE_AS_PUSH_CONSTANT pushConstants push;

// Push constant END.

// INPUT END.

// TS START.

struct MeshShaderPayload
{
    meshlet meshlet[64];
    perInstanceAttr instanceAttr[64];
};

groupshared MeshShaderPayload payload;


[numthreads(64, 1, 1)]
void asmain(
    uint gtid : SV_GroupThreadID,
    uint dtid : SV_DispatchThreadID,
    uint gid :  SV_GroupID
)
{
    // TODO: add culling.
    payload.instanceAttr[gtid] = perInstanceBuffer[meshletToInstanceBuffer[dtid].instanceIndex][meshletToInstanceBuffer[dtid].instanceOffset];
    payload.meshlet[gtid] = meshletBuffer[meshletToInstanceBuffer[dtid].meshletIndex][meshletToInstanceBuffer[dtid].meshletOffset];
    
    DispatchMesh(64, 1, 1, payload);
}

// TS END.

// MS START.

struct meshOutput
{
    float4 position : SV_POSITION;
    float3 color    : COLOR;
    float2 uv       : TEXCOORD0;
};

[outputtopology("triangle")]
[numthreads(64, 1, 1)]
void msmain(
                 uint       gtid : SV_GroupThreadID, 
                 uint       gid  : SV_GroupID,
    in payload   MeshShaderPayload payload,
    out indices  uint3      triangles[64],
    out vertices meshOutput vertices[64]) 
{
    meshlet mesh = payload.meshlet[gtid];
    perInstanceAttr instanceAttr = payload.instanceAttr[gtid];
    
    SetMeshOutputCounts(mesh.vertexCount, mesh.triangleCount);
       
    if (gtid < mesh.triangleCount)
    {
        //
        // meshopt stores the triangle offset in bytes since it stores the
        // triangle indices as 3 consecutive bytes. 
        //
        // Since we repacked those 3 bytes to a 32-bit uint, our offset is now
        // aligned to 4 and we can easily grab it as a uint without any 
        // additional offset math.
        //
        uint packed = primitiveBuffer[mesh.triangleBufferIndex][mesh.triangleBufferOffset + gtid];
        uint vIdx0  = (packed >>  0) & 0xFF;
        uint vIdx1  = (packed >>  8) & 0xFF;
        uint vIdx2  = (packed >> 16) & 0xFF;
        triangles[gtid] = uint3(vIdx0, vIdx1, vIdx2);
    }

    if (gtid < mesh.vertexCount) 
    {
        uint vertexIndex = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + gtid] + mesh.vertexBufferOffset;

        vertices[gtid].position = mul(push.viewProjection, mul(float4(vertexBuffer[mesh.vertexBufferIndex][vertexIndex].position, 1.0), instanceAttr.modelMatrix));
        
        float3 color = float3(
            float(gid & 1),
            float(gid & 3) / 4,
            float(gid & 7) / 8);

        vertices[gtid].color = color;
        vertices[gtid].uv = vertexBuffer[0][vertexIndex].textureCoords;
    }
}

// MESH SHADER END.

// PIXEL SHADER START.

float4 psmain(meshOutput input) : SV_TARGET
{
    // TODO: need to implement CPU side index mapping for textures.
    // When texture gets deleted from registry we can get problems.
    // float4 color = albedo[?].Sample(albedoSamplers[0], input.uv);

    return float4(input.color, 1);;
}

// PIXEL SHADER END.