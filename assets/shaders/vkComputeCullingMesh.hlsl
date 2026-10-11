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

meshJob cullMeshes(uint dtid)
{
    meshJob job;
    
    job.meshletsInJob = MAX_UINT;
    
    // Accumilation runs on entire CMD buffer, no compaction.
    if (dtid < push.meshCount && hasFlag(push.cullingPassFlagBit, ACCUMILATION_PASS_FLAG_BIT))
    {
        command cmd = commandAccumilationBuffer[push.frameIndex][dtid];
        perDrawData dData = drawData[push.frameIndex];

        perInstanceAttr instanceAttr = perInstanceBuffer[cmd.instanceIndex + push.frameIndex][cmd.instanceOffset];

        // Get first lod level to reference a meshlet.
        uint meshletOffsetFirstLodLevel = getMeshletOffset(cmd, 1);
        perMeshAttributes meshAttr = perMeshBuffer[cmd.meshIndex][cmd.meshOffset];
        
        uint selectedLod = selectLodLevel(meshAttr, dData, instanceAttr.modelTransform);
        
        float4 worldBs = worldSpaceBS(float4(meshAttr.bsCenter.xyz, meshAttr.bsRadius), instanceAttr.modelTransform);
        
        cullingData occData = calculateCullingData(worldBs, dData);
            
        if (isInFrustum(dData, worldBs) && !isOcluded(occData, dData, push.hzbLength))
        {
            job.firstMeshletOffset = getMeshletOffset(cmd, selectedLod);
            job.instanceIndex = cmd.instanceIndex + push.frameIndex;
            job.instanceOffset = cmd.instanceOffset;
            job.meshletIndex = cmd.meshletIndex;
            job.meshletsInJob = meshletsInLod(cmd, selectedLod);
        }
        
        return job;
    }
    
    // First opaque pass only frustum test.
    if (dtid < push.meshCount && hasFlag(push.cullingPassFlagBit, FIRST_OPAQUE_PASS_FLAG_BIT))
    {
        bool visible = isVisabilityBitSet(opaqueMeshVisabilityBuffer[push.cmdOpaqueBufferIndex + push.frameIndex][dtid / 32], dtid);
        
        if (!visible)
            return job;
        
        command cmd = commandOpaqueBuffer[push.cmdOpaqueBufferIndex + push.frameIndex][dtid];
        perDrawData dData = drawData[push.frameIndex];
            
        perInstanceAttr instanceAttr = perInstanceBuffer[cmd.instanceIndex + push.frameIndex][cmd.instanceOffset];

        // Get first lod level to reference a mesh.
        uint meshletOffsetFirstLodLevel = getMeshletOffset(cmd, 1);
        perMeshAttributes meshAttr = perMeshBuffer[cmd.meshIndex][cmd.meshOffset];
        
        uint selectedLod = selectLodLevel(meshAttr, dData, instanceAttr.modelTransform);
        
        float4 worldBs = worldSpaceBS(float4(meshAttr.bsCenter.xyz, meshAttr.bsRadius), instanceAttr.modelTransform);
            
        visible = isInFrustum(dData, worldBs);
        
        if (!visible)
            writeMeshVisibilityBit(push.cmdOpaqueBufferIndex + push.frameIndex, dtid, visible);
        else
        {
            job.firstMeshletOffset = getMeshletOffset(cmd, selectedLod);
            job.instanceIndex = cmd.instanceIndex + push.frameIndex;
            job.instanceOffset = cmd.instanceOffset;
            job.meshletIndex = cmd.meshletIndex;
            job.meshletsInJob = meshletsInLod(cmd, selectedLod);
        }
        
        return job;
    }
    
    // Second pass frustum + oclussion cull.
    if (dtid < push.meshCount && hasFlag(push.cullingPassFlagBit, SECOND_OPAQUE_PASS_FLAG_BIT))
    {
        bool visible = isVisabilityBitSet(opaqueMeshVisabilityBuffer[push.cmdOpaqueBufferIndex + push.frameIndex][dtid / 32], dtid);
        
        // Retest each mesh that was drawn in first PASS. 
        if (visible)
        {
            command cmd = commandOpaqueBuffer[push.cmdOpaqueBufferIndex + push.frameIndex][dtid];
            perDrawData dData = drawData[push.frameIndex];
            
            perInstanceAttr instanceAttr = perInstanceBuffer[cmd.instanceIndex + push.frameIndex][cmd.instanceOffset];

            // Get first lod level to reference a meshlet.
            uint meshletOffsetFirstLodLevel = getMeshletOffset(cmd, 1);
            perMeshAttributes meshAttr = perMeshBuffer[cmd.meshIndex][cmd.meshOffset];
            
            float4 worldBs = worldSpaceBS(float4(meshAttr.bsCenter.xyz, meshAttr.bsRadius), instanceAttr.modelTransform);
                
            cullingData occData = calculateCullingData(worldBs, dData);
            
            visible = !isOcluded(occData, dData, push.hzbLength);
            
            if (!visible)
                writeMeshVisibilityBit(push.cmdOpaqueBufferIndex + push.frameIndex, dtid, visible);
            
            return job;
        }
        
        if (!visible)
        {
            command cmd = commandOpaqueBuffer[push.cmdOpaqueBufferIndex + push.frameIndex][dtid];
            perDrawData dData = drawData[push.frameIndex];
            
            // Meshlet was invisible prev frame so we process it.
            perInstanceAttr instanceAttr = perInstanceBuffer[cmd.instanceIndex + push.frameIndex][cmd.instanceOffset];

            // Get first lod level to reference a meshlet.
            uint meshletOffsetFirstLodLevel = getMeshletOffset(cmd, 1);
            perMeshAttributes meshAttr = perMeshBuffer[cmd.meshIndex][cmd.meshOffset];
        
            uint selectedLod = selectLodLevel(meshAttr, dData, instanceAttr.modelTransform);
    
            float4 worldBs = worldSpaceBS(float4(meshAttr.bsCenter.xyz, meshAttr.bsRadius), instanceAttr.modelTransform);
            
            cullingData occData = calculateCullingData(worldBs, dData);
            
            visible = isInFrustum(dData, worldBs) && !isOcluded(occData, dData, push.hzbLength);
            
            if (visible)
            {
                writeMeshVisibilityBit(push.cmdOpaqueBufferIndex + push.frameIndex, dtid, visible);
                
                job.firstMeshletOffset = getMeshletOffset(cmd, selectedLod);
                job.instanceIndex = cmd.instanceIndex + push.frameIndex;
                job.instanceOffset = cmd.instanceOffset;
                job.meshletIndex = cmd.meshletIndex;
                job.meshletsInJob = meshletsInLod(cmd, selectedLod);
            }
                    
            return job;
        }
    }
    
    return job;
}

[numthreads(CULLING_THREAD_COUNT, 1, 1)]
void main(uint dtid : SV_DispatchThreadID)
{
    meshJob mesh = cullMeshes(dtid);

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
