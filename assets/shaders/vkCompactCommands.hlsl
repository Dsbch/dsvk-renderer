//  dxc -T cs_6_9 -E main -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkCompactCommands.spv vkCompactCommands.hlsl
//  add -fspv-debug=vulkan-with-source flag only for debug.
#include "common.hlsl"

struct pushConstant
{
    uint frameIndex;
    uint hzbMipLevel;
    uint mipWidth;
    uint mipHeight;
    uint cullingPassFlagBit;
    uint cmdOpaqueBufferIndex;
    uint meshletCount;
    uint hzbLength;
    uint compactRule;
};

DEFINE_AS_PUSH_CONSTANT
pushConstant push;

groupshared uint groupVisibleCount;
groupshared uint groupBase;

[numthreads(COMPACT_THREAD_COUNT, 1, 1)]
void main(uint dtid : SV_DispatchThreadID, uint gtid : SV_GroupIndex)
{
    if (gtid == 0)
    {
        groupVisibleCount = 0;
        groupBase = 0;
    }
    
    GroupMemoryBarrierWithGroupSync();

    bool visible = false;
    
    uint2 unpacked;

    if (dtid < push.meshletCount)
    {
        uint visData;
        if (hasFlag(push.cullingPassFlagBit, ACCUMILATION_PASS_FLAG_BIT))
            visData = accumilationVisabilityBuffer[push.frameIndex][dtid];
        else
            visData = opaqueVisabilityBuffer[push.cmdOpaqueBufferIndex + push.frameIndex][dtid];

        unpacked = unpackUint2(visData);
        
        visible = hasFlag(unpacked.x, push.compactRule);
    }

    uint laneSlot = WavePrefixCountBits(visible);
    uint waveVisibleCount = WaveActiveCountBits(visible);

    uint waveBaseInGroup = 0;
    if (WaveIsFirstLane() && waveVisibleCount > 0)
        InterlockedAdd(groupVisibleCount, waveVisibleCount, waveBaseInGroup);
    waveBaseInGroup = WaveReadLaneFirst(waveBaseInGroup);

    GroupMemoryBarrierWithGroupSync();

    if (gtid == 0 && groupVisibleCount > 0)
    {
        InterlockedAdd(compactBuffer[push.frameIndex][0].x, groupVisibleCount, groupBase);

        uint groupsNeeded = (groupBase + groupVisibleCount);

        InterlockedMax(compactBuffer[push.frameIndex][0].y, groupsNeeded);
    }

    GroupMemoryBarrierWithGroupSync();
    
    if (visible)
    {
        command cmd;
    
        if (hasFlag(push.cullingPassFlagBit, ACCUMILATION_PASS_FLAG_BIT))
            cmd = commandAccumilationBuffer[push.frameIndex][dtid];
        else
            cmd = commandOpaqueBuffer[push.cmdOpaqueBufferIndex + push.frameIndex][dtid];
        
        uint meshletOffset = getMeshletOffset(cmd, unpacked.y);
    
        compactBuffer[push.frameIndex][1 + groupBase + waveBaseInGroup + laneSlot] = uint4(cmd.meshletIndex, meshletOffset, cmd.instanceIndex + push.frameIndex, cmd.instanceOffset);
    }
}
 