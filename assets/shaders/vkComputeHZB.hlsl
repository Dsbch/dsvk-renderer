//  dxc -T cs_6_9 -E main -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkHzbCs.spv vkComputeHZB.hlsl
#ifdef __spirv__
#define DEFINE_AS_PUSH_CONSTANT [[vk::push_constant]]
#else
#define DEFINE_AS_PUSH_CONSTANT
#endif

#define THREADS_COUNT 32

struct pushConstant
{
    uint hzbMipLevel;
    uint width;
    uint height;
};

DEFINE_AS_PUSH_CONSTANT
pushConstant push;

Texture2D<float> originalZbuffer : register(t0, space0);
RWTexture2D<float> hzbChain[] : register(u1, space0);

[numthreads(THREADS_COUNT, THREADS_COUNT, 1)]
void main(uint2 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= push.width || dtid.y >= push.height)
        return;

    float depth;
    uint2 srcCoord = dtid * 2;
    
    if (push.hzbMipLevel == 0)
    {
        float d0 = originalZbuffer[srcCoord + uint2(0, 0)];
        float d1 = originalZbuffer[srcCoord + uint2(1, 0)];
        float d2 = originalZbuffer[srcCoord + uint2(0, 1)];
        float d3 = originalZbuffer[srcCoord + uint2(1, 1)];

        depth = depth = min(min(d0, d1), min(d2, d3));
    }
    else
    {
        float d0 = hzbChain[push.hzbMipLevel - 1][srcCoord + uint2(0, 0)];
        float d1 = hzbChain[push.hzbMipLevel - 1][srcCoord + uint2(1, 0)];
        float d2 = hzbChain[push.hzbMipLevel - 1][srcCoord + uint2(0, 1)];
        float d3 = hzbChain[push.hzbMipLevel - 1][srcCoord + uint2(1, 1)];

        depth = depth = min(min(d0, d1), min(d2, d3));
    }

    hzbChain[push.hzbMipLevel][dtid] = depth;
}