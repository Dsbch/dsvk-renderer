//  dxc -T vs_6_9 -E vsmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkLineVs.spv vkLine.hlsl
//  dxc -T ps_6_9 -E psmain -spirv -Fo -fvk-use-scalar-layout -Fo vkCompiled/vkLinePs.spv vkLine.hlsl
#ifdef __spirv__
#define DEFINE_AS_PUSH_CONSTANT [[vk::push_constant]]
#else
#define DEFINE_AS_PUSH_CONSTANT
#endif

// INPUT START.

// SSBO START.

struct lineVertex
{
    float3 position;
};

StructuredBuffer<lineVertex> positionBuffer : register(t0, space0);

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

ConstantBuffer<perDrawData> drawData : register(b1, space0);

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
    
    lineVertex v = positionBuffer[vertexID];
    
    result.position = mul(drawData.useDebugCamera ? drawData.debugViewProjection : drawData.viewProjection, float4(v.position.xyz, 1.0f));

    return result;
}

// VS END.

// PIXEL SHADER START.

float4 psmain(vertexOutput input) : SV_TARGET
{
    return float4(0.0f, 0.0f, 1.0f, 1.0f);
}

// PIXEL SHADER END.
