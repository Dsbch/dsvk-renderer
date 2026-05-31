//  dxc -T ms_6_9 -E msmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshCompositeMs.spv vkComposite.hlsl
//  dxc -T ps_6_9 -E psmain -spirv -fvk-use-scalar-layout -Fo vkCompiled/vkMeshCompositePs.spv vkComposite.hlsl
//  dxc -T as_6_9 -E asmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshCompositeAs.spv vkComposite.hlsl
//  add -fspv-debug=vulkan-with-source flag only for debug.
#include "common.hlsl"

// TS START.

struct Payload{};

groupshared Payload payload;

[numthreads(1, 1, 1)]
void asmain(
    uint gtid : SV_GroupThreadID,
    uint dtid : SV_DispatchThreadID,
    uint gid : SV_GroupID
)
{    
    // Dispatch 1 meshlet for full screen pass.
    DispatchMesh(1, 1, 1, payload);
}
// TS END.

// MS START.

struct MeshOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

[outputtopology("triangle")]
[numthreads(3, 1, 1)]
void msmain(
    uint gtid : SV_GroupThreadID,
    in payload Payload meshPayload,
    out indices uint3 triangles[1],
    out vertices MeshOutput verts[3])
{
    SetMeshOutputCounts(3, 1);

    if (gtid == 0)
        triangles[0] = uint3(0, 1, 2);

    float2 uv = float2((gtid << 1) & 2, gtid & 2);
    verts[gtid].position = float4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
    verts[gtid].uv = uv;
}

// MESH SHADER END.

// PIXEL SHADER START.

bool isApproximatelyEqual(float a, float b)
{
    return abs(a - b) <= (abs(a) < abs(b) ? abs(b) : abs(a)) * EPSILON;
}

float max3(float3 v)
{
    return max(max(v.x, v.y), v.z);
}

float4 psmain(MeshOutput input) : SV_TARGET
{
    float revealage = reveal.Sample(revealSampler, input.uv).r;
    float4 accumulation = accum.Sample(accumSampler, input.uv);
    
    if (isApproximatelyEqual(revealage, 1.0f))
        discard;

    if (isinf(max3(abs(accumulation.rgb))))
        accumulation.rgb = float3(accumulation.a, accumulation.a, accumulation.a);

    float3 average_color = accumulation.rgb / max(accumulation.a, EPSILON);

    return float4(average_color, 1.0f - revealage);
}

// PIXEL SHADER END.
