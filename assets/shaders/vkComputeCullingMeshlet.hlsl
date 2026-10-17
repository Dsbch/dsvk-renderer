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
        cullingData occData = calculateCullingData(float4(worldBounds.center, worldBounds.radius), dData.projection, dData.view, dData.width, dData.height);
            
        return attrs.alphaType == BLEND_ALPHA_MODE && isInFrustum(dData, worldBounds) && !isOcluded(occData, currentFrameHzbChain, dData.width, dData.height, push.hzbLength);
    }
    
    // First opaque pass.
    if (hasFlag(push.cullingPassFlagBit, FIRST_OPAQUE_PASS_FLAG_BIT))
    {
        cullingData occData = calculateCullingData(float4(worldBounds.center, worldBounds.radius), dData.prevFrameProjection, dData.prevFrameView, dData.width, dData.height);
        
        return isFrontfaceMeshlet(dData, worldBounds) && isInFrustum(dData, worldBounds) && !isOcluded(occData, prevFrameHzbChain, dData.width, dData.height, push.hzbLength);
    }
    
    // Second opaque pass.
    if (hasFlag(push.cullingPassFlagBit, SECOND_OPAQUE_PASS_FLAG_BIT))
    {
        if (!isFrontfaceMeshlet(dData, worldBounds) || !isInFrustum(dData, worldBounds))
            return false;

        float4 sphere = float4(worldBounds.center, worldBounds.radius);

        if (job.isFirstPassJob)
        {
            cullingData prevOcc = calculateCullingData(sphere, dData.prevFrameProjection, dData.prevFrameView, dData.width, dData.height);

            if (!isOcluded(prevOcc, prevFrameHzbChain, dData.width, dData.height, push.hzbLength))
                return false;
        }

        cullingData occData = calculateCullingData(sphere, dData.projection, dData.view, dData.width, dData.height);

        return !isOcluded(occData, currentFrameHzbChain, dData.width, dData.height, push.hzbLength);
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