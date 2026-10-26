//  dxc -T cs_6_9 -E main -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkHzbCs.spv vkComputeHZB.hlsl
//  add -fspv-debug=vulkan-with-source flag only for debug.
#include "common.hlsl"

float loadSourceDepth(uint2 coord)
{
    if (push.hzbMipLevel == 0)
        return originalZbuffer[coord];

    return currentFrameHzbChain[push.hzbMipLevel - 1][coord];
}

[numthreads(32, 32, 1)]
void main(uint2 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= push.mipWidth || dtid.y >= push.mipHeight)
        return;

    uint srcWidth, srcHeight;

    if (push.hzbMipLevel == 0)
        originalZbuffer.GetDimensions(srcWidth, srcHeight);
    else
        currentFrameHzbChain[push.hzbMipLevel - 1].GetDimensions(srcWidth, srcHeight);

    uint2 srcCoord = dtid * 2;

    uint2 footprint = uint2(2, 2);

    if (dtid.x == push.mipWidth - 1 && (srcWidth % 2 != 0))
        footprint.x = 3;

    if (dtid.y == push.mipHeight - 1 && (srcHeight % 2 != 0))
        footprint.y = 3;

    uint2 srcMax = uint2(srcWidth, srcHeight) - 1;
    float depth = loadSourceDepth(srcCoord);

    for (uint y = 0; y < footprint.y; y++)
        for (uint x = 0; x < footprint.x; x++)
            depth = min(depth, loadSourceDepth(min(srcCoord + uint2(x, y), srcMax)));

    currentFrameHzbChain[push.hzbMipLevel][dtid] = depth;
}