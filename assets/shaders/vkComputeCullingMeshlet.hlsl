//  dxc -T cs_6_9 -E main -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkComputeCulling.spv vkComputeCulling.hlsl
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
    uint meshCount;
    uint hzbLength;
    uint compactCapacity;
};

DEFINE_AS_PUSH_CONSTANT
pushConstant push;

bool isMeshletVisible(uint tid, meshJob job)
{
    meshletAttributes attrs = meshletAttributesBuffer[job.meshletIndex][job.firstMeshletOffset + tid];
    perDrawData dData = drawData[push.frameIndex];
    perInstanceAttr instanceAttr = perInstanceBuffer[job.instanceIndex][job.instanceOffset];
    meshletBounds worldBounds = worldSpaceMeshletBounds(attrs.bounds, instanceAttr.modelTransform);
    
    // Accumilation runs on entire CMD buffer, no compaction.
    if (hasFlag(push.cullingPassFlagBit, ACCUMILATION_PASS_FLAG_BIT))
    {
        cullingData occData = calculateCullingData(float4(worldBounds.center, worldBounds.radius), dData);
            
        return attrs.alphaType == BLEND_ALPHA_MODE && isInFrustum(dData, worldBounds) && !isOcluded(occData, dData, push.hzbLength);
    }
    
    // First opaque pass only frustum test.
    if (hasFlag(push.cullingPassFlagBit, FIRST_OPAQUE_PASS_FLAG_BIT))
    {
        // Cone culling doesn't work for animated meshlets. On CPU cone calculation is wrong.
        return isFrontfaceMeshlet(dData, worldBounds) && isInFrustum(dData, worldBounds);
    }
    
    // Second pass frustum + oclussion cull.
    if (hasFlag(push.cullingPassFlagBit, SECOND_OPAQUE_PASS_FLAG_BIT))
    {
        // Cone culling doesn't work for animated meshlets. On CPU cone calculation is wrong.
        bool visible = isFrontfaceMeshlet(dData, worldBounds) && isInFrustum(dData, worldBounds);
        
        if (!visible)
            return visible;
                
        cullingData occData = calculateCullingData(float4(worldBounds.center, worldBounds.radius), dData);
        
        return visible && !isOcluded(occData, dData, push.hzbLength);
    }
    
    return false;
}

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