//  dxc -T cs_6_9 -E main -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkComputeCulling.spv vkComputeCulling.hlsl
//  add -fspv-debug=vulkan-with-source flag only for debug.
#include "common.hlsl"

[numthreads(CULLING_THREAD_COUNT, 1, 1)]
void main(uint gid : SV_GroupID, uint dtid : SV_DispatchThreadID, uint tid : SV_GroupThreadID)
{
    if (gid >= jobBuffer[push.frameIndex][0].instanceIndex)
        return;
    
    meshJob job = jobBuffer[push.frameIndex][gid + 1];
    
    if (tid >= job.meshletsInJob)
        return;
        
    bool visible = isMeshletVisible(tid, job);
    
    if (!visible)
        return;
    
    uint waveVisible = WaveActiveCountBits(visible);
    uint laneSlot = WavePrefixCountBits(visible);

    uint groupBase = 0;
    if (WaveIsFirstLane() && waveVisible > 0)
    {
        InterlockedAdd(compactBuffer[push.frameIndex][0].x, waveVisible, groupBase);
     
        InterlockedMax(compactBuffer[push.frameIndex][0].y, min(groupBase + waveVisible, push.compactCapacity - 1));
    }
    
    groupBase = WaveReadLaneFirst(groupBase);
    
    if (visible && (1 + groupBase + laneSlot) < push.compactCapacity)
    {
        compactBuffer[push.frameIndex][1 + groupBase + laneSlot] = uint4(
            job.meshletIndex, 
            job.firstMeshletOffset + tid,
            job.instanceIndex,
            job.instanceOffset
        );
    }
}