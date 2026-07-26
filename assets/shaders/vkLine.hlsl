//  dxc -T vs_6_9 -E vsmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkLineVs.spv vkLine.hlsl
//  dxc -T ps_6_9 -E psmain -spirv -Fo -fvk-use-scalar-layout -Fo vkCompiled/vkLinePs.spv vkLine.hlsl
#include "common.hlsl"

// INPUT START.

// Push constant START.
struct pushConstant
{
    uint frameIndex;
};

DEFINE_AS_PUSH_CONSTANT
pushConstant push;


// SSBO START.

struct lineVertex
{
    float3 position;
};

StructuredBuffer<lineVertex> positionBuffer : register(t0, space0);

// SSBO END.

// UBO START.

ConstantBuffer<perDrawData> drawData[] : register(b1, space0);

// UBO END.

// INPUT END.

// VS START.

struct vertexOutput
{
    float4 position : SV_POSITION;
};

vertexOutput vsmain(uint vertexID : SV_VertexID)
{
    vertexOutput result;
    perDrawData dData = drawData[push.frameIndex];
    
    lineVertex v = positionBuffer[vertexID];
    
    result.position = mul(dData.useDebugCamera ? dData.debugViewProjection : dData.viewProjection, float4(v.position.xyz, 1.0f));

    return result;
}

// VS END.

// PIXEL SHADER START.

float4 psmain(vertexOutput input) : SV_TARGET
{
    return float4(0.0f, 0.0f, 1.0f, 1.0f);
}

// PIXEL SHADER END.
