//  dxc -T cs_6_9 -E main -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkComputeCulling.spv vkComputeCulling.hlsl
//  add -fspv-debug=vulkan-with-source flag only for debug.
#include "common.hlsl"

meshJob cullMesh(uint dtid)
{
    meshJob job;
    
    job.meshletsInJob = MAX_UINT;
    job.isFirstPassJob = 0;
    
    // Accumilation runs on entire CMD buffer, no compaction.
    if (dtid < push.meshCount && hasFlag(push.cullingPassFlagBit, ACCUMILATION_PASS_FLAG_BIT))
    {
        command cmd = commandAccumilationBuffer[push.frameIndex][dtid];
        perDrawData dData = drawData[push.frameIndex];
        perInstanceAttr instanceAttr = perInstanceBuffer[cmd.instanceIndex + push.frameIndex][cmd.instanceOffset];
        perMeshAttributes meshAttr = perMeshBuffer[cmd.meshIndex][cmd.meshOffset];
        
        float4 worldBs = worldSpaceBS(float4(meshAttr.bsCenter.xyz, meshAttr.bsRadius), instanceAttr.modelTransform);
        
        if (!isInFrustum(dData, worldBs))
            return job;
        
        cullingData occData = calculateCullingData(worldBs, dData.projection, dData.view, dData.width, dData.height);

        if (isOcluded(occData, currentFrameHzbChain, dData.width, dData.height, push.hzbLength, false))
            return job;

        uint selectedLod = selectLodLevel(meshAttr, dData, instanceAttr.modelTransform);
        
        job.firstMeshletOffset = getMeshletOffset(cmd, selectedLod);
        job.instanceIndex = cmd.instanceIndex + push.frameIndex;
        job.instanceOffset = cmd.instanceOffset;
        job.meshletIndex = cmd.meshletIndex;
        job.meshletsInJob = meshletsInLod(cmd, selectedLod);
        
        return job;
    }
    
    // First opaque pass.
    if (dtid < push.meshCount && hasFlag(push.cullingPassFlagBit, FIRST_OPAQUE_PASS_FLAG_BIT))
    {
        command cmd = commandOpaqueBuffer[push.cmdOpaqueBufferIndex + push.frameIndex][dtid];
        perDrawData dData = drawData[push.frameIndex];
        perInstanceAttr instanceAttr = perInstanceBuffer[cmd.instanceIndex + push.frameIndex][cmd.instanceOffset];
        perMeshAttributes meshAttr = perMeshBuffer[cmd.meshIndex][cmd.meshOffset];
        
        float4 worldBs = worldSpaceBS(float4(meshAttr.bsCenter.xyz, meshAttr.bsRadius), instanceAttr.modelTransform);
             
        if (!isInFrustum(dData, worldBs))
            return job;
        
        cullingData occData = calculateCullingData(worldBs, dData.prevFrameProjection, dData.prevFrameView, dData.width, dData.height);
        
        if (isOcluded(occData, prevFrameHzbChain, dData.width, dData.height, push.hzbLength, true))
            return job;
        
        uint selectedLod = selectLodLevel(meshAttr, dData, instanceAttr.modelTransform);
        
        job.firstMeshletOffset = getMeshletOffset(cmd, selectedLod);
        job.instanceIndex = cmd.instanceIndex + push.frameIndex;
        job.instanceOffset = cmd.instanceOffset;
        job.meshletIndex = cmd.meshletIndex;
        job.meshletsInJob = meshletsInLod(cmd, selectedLod);
        job.isFirstPassJob = 1;
        
        return job;
    }
    
    // Second opaque pass.
    if (dtid < push.meshCount && hasFlag(push.cullingPassFlagBit, SECOND_OPAQUE_PASS_FLAG_BIT))
    {
        command cmd = commandOpaqueBuffer[push.cmdOpaqueBufferIndex + push.frameIndex][dtid];
        perDrawData dData = drawData[push.frameIndex];
        perInstanceAttr instanceAttr = perInstanceBuffer[cmd.instanceIndex + push.frameIndex][cmd.instanceOffset];
        perMeshAttributes meshAttr = perMeshBuffer[cmd.meshIndex][cmd.meshOffset];
            
        float4 worldBs = worldSpaceBS(float4(meshAttr.bsCenter.xyz, meshAttr.bsRadius), instanceAttr.modelTransform);
        
        if (!isInFrustum(dData, worldBs))
            return job;
            
        cullingData occData = calculateCullingData(worldBs, dData.prevFrameProjection, dData.prevFrameView, dData.width, dData.height);
        
        bool drawnFirstPass = !isOcluded(occData, prevFrameHzbChain, dData.width, dData.height, push.hzbLength, true);
        
        if (drawnFirstPass)
            return job;
        
        occData = calculateCullingData(worldBs, dData.projection, dData.view, dData.width, dData.height);
        
        if (isOcluded(occData, currentFrameHzbChain, dData.width, dData.height, push.hzbLength, false))
            return job;
        
        uint selectedLod = selectLodLevel(meshAttr, dData, instanceAttr.modelTransform);
        
        job.firstMeshletOffset = getMeshletOffset(cmd, selectedLod);
        job.instanceIndex = cmd.instanceIndex + push.frameIndex;
        job.instanceOffset = cmd.instanceOffset;
        job.meshletIndex = cmd.meshletIndex;
        job.meshletsInJob = meshletsInLod(cmd, selectedLod);
                    
        return job;
    }
    
    return job;
}

[numthreads(CULLING_THREAD_COUNT, 1, 1)]
void main(uint dtid : SV_DispatchThreadID)
{
    meshJob mesh = cullMesh(dtid);

    if (mesh.meshletsInJob == MAX_UINT)
        return;
    
    uint meshCount = mesh.meshletsInJob;
    uint jobCount = (meshCount + CULLING_THREAD_COUNT - 1) / CULLING_THREAD_COUNT;

    uint base;
    InterlockedAdd(jobBuffer[push.frameIndex][0].instanceIndex, jobCount, base);
    InterlockedMax(jobBuffer[push.frameIndex][0].instanceOffset, base + jobCount);

    for (uint i = 0; i < jobCount; i++)
    {
        uint slot = base + i;
        
        meshJob job = mesh;
        job.firstMeshletOffset = mesh.firstMeshletOffset + i * CULLING_THREAD_COUNT;
        job.meshletsInJob = min(CULLING_THREAD_COUNT, meshCount - i * CULLING_THREAD_COUNT);

        jobBuffer[push.frameIndex][1 + slot] = job;
    }
}
