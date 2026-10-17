//  dxc -T cs_6_9 -E main -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkComputeCulling.spv vkComputeCulling.hlsl
//  add -fspv-debug=vulkan-with-source flag only for debug.
#include "common.hlsl"

meshJob cullMesh(uint dtid)
{
    meshJob job;
    
    job.meshletsInJobIsFirstPass = packUint2(MAX_UINT, 0);
    
    // Voxel pass.
    if (hasFlag(push.cullingPassFlagBit, VOXEL_PASS_FLAG_BIT))
    {
        command cmd = commandOpaqueBuffer[push.cmdOpaqueBufferIndex + push.frameIndex][dtid];
        
        uint2 packed = unpackUint2(cmd.meshInstanceIndex);
        
        perDrawData dData = drawData[push.frameIndex];
        perInstanceAttr instanceAttr = perInstanceBuffer[packed.y + push.frameIndex][cmd.instanceOffset];
        perMeshAttributes meshAttr = perMeshBuffer[packed.x][cmd.meshOffset];
        
        uint selectedLod = selectLodLevel(meshAttr, dData, instanceAttr.modelTransform);
        
        job.firstMeshletOffset = getMeshletOffset(cmd, selectedLod);
        job.instanceMeshletIndex = packUint2(packed.y + push.frameIndex, cmd.meshletIndex);
        job.instanceOffset = cmd.instanceOffset;
        job.meshletsInJobIsFirstPass = packUint2(meshletsInLod(cmd, selectedLod), 0);
        
        return job;
    }
    
    // Accumilation runs on entire CMD buffer, no compaction.
    if (dtid < push.meshCount && hasFlag(push.cullingPassFlagBit, ACCUMILATION_PASS_FLAG_BIT))
    {
        command cmd = commandAccumilationBuffer[push.frameIndex][dtid];
        
        uint2 packed = unpackUint2(cmd.meshInstanceIndex);
        
        perDrawData dData = drawData[push.frameIndex];
        perInstanceAttr instanceAttr = perInstanceBuffer[packed.y + push.frameIndex][cmd.instanceOffset];
        perMeshAttributes meshAttr = perMeshBuffer[packed.x][cmd.meshOffset];
        
        float4 worldBs = worldSpaceBS(float4(meshAttr.bsCenter.xyz, meshAttr.bsRadius), instanceAttr.modelTransform);
        
        if (!isInFrustum(dData, worldBs))
            return job;
        
        cullingData occData = calculateCullingData(worldBs, dData.projection, dData.view, dData.width, dData.height);

        if (isOcluded(occData, currentFrameHzbChain, dData.width, dData.height, push.hzbLength, false))
            return job;

        uint selectedLod = selectLodLevel(meshAttr, dData, instanceAttr.modelTransform);
        
        job.firstMeshletOffset = getMeshletOffset(cmd, selectedLod);
        job.instanceMeshletIndex = packUint2(packed.y + push.frameIndex, cmd.meshletIndex);
        job.instanceOffset = cmd.instanceOffset;
        job.meshletsInJobIsFirstPass = packUint2(meshletsInLod(cmd, selectedLod), 0);
        
        return job;
    }
    
    // First opaque pass.
    if (dtid < push.meshCount && hasFlag(push.cullingPassFlagBit, FIRST_OPAQUE_PASS_FLAG_BIT))
    {
        command cmd = commandOpaqueBuffer[push.cmdOpaqueBufferIndex + push.frameIndex][dtid];
        
        uint2 packed = unpackUint2(cmd.meshInstanceIndex);
        
        perDrawData dData = drawData[push.frameIndex];
        perInstanceAttr instanceAttr = perInstanceBuffer[packed.y + push.frameIndex][cmd.instanceOffset];
        perMeshAttributes meshAttr = perMeshBuffer[packed.x][cmd.meshOffset];
        
        float4 worldBs = worldSpaceBS(float4(meshAttr.bsCenter.xyz, meshAttr.bsRadius), instanceAttr.modelTransform);
             
        if (!isInFrustum(dData, worldBs))
            return job;
        
        cullingData occData = calculateCullingData(worldBs, dData.prevFrameProjection, dData.prevFrameView, dData.width, dData.height);
        
        if (isOcluded(occData, prevFrameHzbChain, dData.width, dData.height, push.hzbLength, true))
            return job;
        
        uint selectedLod = selectLodLevel(meshAttr, dData, instanceAttr.modelTransform);
        
        job.firstMeshletOffset = getMeshletOffset(cmd, selectedLod);
        job.instanceMeshletIndex = packUint2(packed.y + push.frameIndex, cmd.meshletIndex);
        job.instanceOffset = cmd.instanceOffset;
        job.meshletsInJobIsFirstPass = packUint2(meshletsInLod(cmd, selectedLod), 1);
        
        return job;
    }
    
    // Second opaque pass.
    if (dtid < push.meshCount && hasFlag(push.cullingPassFlagBit, SECOND_OPAQUE_PASS_FLAG_BIT))
    {
        command cmd = commandOpaqueBuffer[push.cmdOpaqueBufferIndex + push.frameIndex][dtid];
        
        uint2 packed = unpackUint2(cmd.meshInstanceIndex);
        
        perDrawData dData = drawData[push.frameIndex];
        perInstanceAttr instanceAttr = perInstanceBuffer[packed.y + push.frameIndex][cmd.instanceOffset];
        perMeshAttributes meshAttr = perMeshBuffer[packed.x][cmd.meshOffset];
            
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
        job.instanceMeshletIndex = packUint2(packed.y + push.frameIndex, cmd.meshletIndex);
        job.instanceOffset = cmd.instanceOffset;
        job.meshletsInJobIsFirstPass = packUint2(meshletsInLod(cmd, selectedLod), 0);
                    
        return job;
    }
    
    return job;
}

[numthreads(CULLING_THREAD_COUNT, 1, 1)]
void main(uint dtid : SV_DispatchThreadID)
{
    meshJob job = cullMesh(dtid);

    if (unpackUint2(job.meshletsInJobIsFirstPass).x == MAX_UINT16)
        return;
    
    uint meshCount = unpackUint2(job.meshletsInJobIsFirstPass).x;
    uint jobCount = (meshCount + CULLING_THREAD_COUNT - 1) / CULLING_THREAD_COUNT;

    uint base;
    InterlockedAdd(jobBuffer[push.frameIndex][0].instanceMeshletIndex, jobCount, base);

    for (uint i = 0; i < jobCount; i++)
    {
        uint slot = base + i;
        
        meshJob crntJob = job;
        crntJob.firstMeshletOffset = job.firstMeshletOffset + i * CULLING_THREAD_COUNT;
        crntJob.meshletsInJobIsFirstPass = packUint2(min(CULLING_THREAD_COUNT, meshCount - i * CULLING_THREAD_COUNT), unpackUint2(job.meshletsInJobIsFirstPass).y);

        jobBuffer[push.frameIndex][1 + slot] = crntJob;
    }
}
