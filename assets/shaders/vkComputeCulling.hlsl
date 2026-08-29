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
    uint cmdBufferCount;
    uint hzbLength;
    uint compactRule;
};

DEFINE_AS_PUSH_CONSTANT
pushConstant push;

bool isOcluded(cullingData occData, perDrawData dData)
{
    // Floor, because we will sample 2x2 texels for that sphere.
    uint neededChain = uint(floor(log2(max(1.0f, occData.pixelLength))));
                    
    neededChain = min(push.hzbLength - 1, neededChain);
                    
    // drawData.width >> neededChain => divide by 2 in power of neededChain.
    // drawData.width / 2 because zero mip starts with drawData.width / 2.
    int mipWidth = max(1, int(dData.width / 2) >> (neededChain));
    int mipHeight = max(1, int(dData.height / 2) >> (neededChain));

    float2 mipTexelCoords = occData.sphereCenterUV * float2(mipWidth, mipHeight) - 0.5f;

    int2 topLeftTexel = clamp(int2(floor(mipTexelCoords)), int2(0, 0), int2(mipWidth - 2, mipHeight - 2));

    float d00 = hzbChain[neededChain][topLeftTexel + int2(0, 0)];
    float d10 = hzbChain[neededChain][topLeftTexel + int2(1, 0)];
    float d01 = hzbChain[neededChain][topLeftTexel + int2(0, 1)];
    float d11 = hzbChain[neededChain][topLeftTexel + int2(1, 1)];

    float minDepth = min(min(d00, d10), min(d01, d11));
                
    return !((occData.closestDepth >= minDepth) || occData.closestDepth < 0.0f);
}

[numthreads(THREADS_COUNT, 1, 1)]
void main(uint dtid : SV_DispatchThreadID)
{
    // Accumilation runs on entire CMD buffer, no compaction.
    if (dtid < push.cmdBufferCount && hasFlag(push.cullingPassFlagBit, ACCUMILATION_PASS_FLAG_BIT))
    {
        command cmd = commandAccumilationBuffer[push.frameIndex][dtid];
        perDrawData dData = drawData[push.frameIndex];

        perInstanceAttr instanceAttr = perInstanceBuffer[cmd.instanceIndex + push.frameIndex][cmd.instanceOffset];

            // Get first lod level to reference a meshlet.
        uint meshletOffsetFirstLodLevel = getMeshletOffset(cmd, 1);
        meshlet mesh = meshletBuffer[cmd.meshletIndex][meshletOffsetFirstLodLevel];
        perMeshAttributes meshAttr = perMeshBuffer[mesh.perMeshBufferIndex][mesh.perMeshBufferOffset];
        
        uint selectedLod = selectLodLevel(meshAttr, dData, instanceAttr.modelTransform);
        uint meshletOffset = getMeshletOffset(cmd, selectedLod);
            
            // Overdraw for current lod level.
        if (meshletOffset == MAX_UINT)
        {
            commandAccumilationBuffer[push.frameIndex][dtid].visabilityBit = NOT_VISIBLE_FLAG_BIT;
            return;
        }
            
        mesh = meshletBuffer[cmd.meshletIndex][meshletOffset];
            
        meshletBounds worldBounds = worldSpaceMeshletBounds(mesh.bounds, instanceAttr.modelTransform, meshAttr);
            
        cullingData occData = calculateCullingData(float4(worldBounds.center, worldBounds.radius), dData);
            
        bool visible = mesh.alphaType == BLEND_ALPHA_MODE && isInFrustum(dData, worldBounds) && !isOcluded(occData, dData);
             
        commandAccumilationBuffer[push.frameIndex][dtid].selectedLod = selectedLod;
        commandAccumilationBuffer[push.frameIndex][dtid].visabilityBit = visible ? VISIBLE_FIRST_PASS_FLAG_BIT : NOT_VISIBLE_FLAG_BIT;
            
        return;
    }
    
    // First opaque pass only frustum test.
    if (dtid < push.cmdBufferCount && hasFlag(push.cullingPassFlagBit, FIRST_OPAQUE_PASS_FLAG_BIT))
    {
        command cmd = commandOpaqueBuffer[push.cmdOpaqueBufferIndex][dtid];
        perDrawData dData = drawData[push.frameIndex];

        if (!hasFlag(cmd.visabilityBit, VISIBLE_FIRST_PASS_FLAG_BIT | VISIBLE_SECOND_PASS_FLAG_BIT))
            return;
            
        perInstanceAttr instanceAttr = perInstanceBuffer[cmd.instanceIndex + push.frameIndex][cmd.instanceOffset];

        // Get first lod level to reference a mesh.
        uint meshletOffsetFirstLodLevel = getMeshletOffset(cmd, 1);
        perMeshAttributes meshAttr = perMeshBuffer[cmd.meshIndex][cmd.meshOffset];
        
        uint selectedLod = selectLodLevel(meshAttr, dData, instanceAttr.modelTransform);
    
        uint meshletOffset = getMeshletOffset(cmd, selectedLod);
            
        // Overdraw for current lod level.
        if (meshletOffset == MAX_UINT)
        {
            commandOpaqueBuffer[push.cmdOpaqueBufferIndex][dtid].visabilityBit = NOT_VISIBLE_FLAG_BIT;
            return;
        }
            
        meshlet mesh = meshletBuffer[cmd.meshletIndex][meshletOffset];
            
        meshletBounds worldBounds = worldSpaceMeshletBounds(mesh.bounds, instanceAttr.modelTransform, meshAttr);
            
        // Cone culling doesn't work for animated meshlets. On CPU cone calculation is wrong.
        bool visible = isFrontfaceMeshlet(dData, worldBounds) && isInFrustum(dData, worldBounds);
            
        commandOpaqueBuffer[push.cmdOpaqueBufferIndex][dtid].selectedLod = selectedLod;
        commandOpaqueBuffer[push.cmdOpaqueBufferIndex][dtid].visabilityBit = visible ? VISIBLE_FIRST_PASS_FLAG_BIT : NOT_VISIBLE_FLAG_BIT;
            
        return;
    }
    
    // Second pass frustum + oclussion cull.
    if (dtid < push.cmdBufferCount && hasFlag(push.cullingPassFlagBit, SECOND_OPAQUE_PASS_FLAG_BIT))
    {
        command cmd = commandOpaqueBuffer[push.cmdOpaqueBufferIndex][dtid];
        perDrawData dData = drawData[push.frameIndex];
     
        // Retest each meshlet that was drawn in first PASS. 
        if (hasFlag(cmd.visabilityBit, VISIBLE_FIRST_PASS_FLAG_BIT))
        {
            perInstanceAttr instanceAttr = perInstanceBuffer[cmd.instanceIndex + push.frameIndex][cmd.instanceOffset];

            // Get first lod level to reference a meshlet.
            uint meshletOffsetFirstLodLevel = getMeshletOffset(cmd, 1);
            meshlet mesh = meshletBuffer[cmd.meshletIndex][meshletOffsetFirstLodLevel];
            perMeshAttributes meshAttr = perMeshBuffer[mesh.perMeshBufferIndex][mesh.perMeshBufferOffset];
        
            uint meshletOffset = getMeshletOffset(cmd, cmd.selectedLod);
            
            // Overdraw for current lod level.
            if (meshletOffset == MAX_UINT)
            {
                commandOpaqueBuffer[push.cmdOpaqueBufferIndex][dtid].visabilityBit = NOT_VISIBLE_FLAG_BIT;
                return;
            }
            
            mesh = meshletBuffer[cmd.meshletIndex][meshletOffset];
            
            meshletBounds worldBounds = worldSpaceMeshletBounds(mesh.bounds, instanceAttr.modelTransform, meshAttr);
                
            cullingData occData = calculateCullingData(float4(worldBounds.center, worldBounds.radius), dData);
            
            commandOpaqueBuffer[push.cmdOpaqueBufferIndex][dtid].visabilityBit = isOcluded(occData, dData) ? NOT_VISIBLE_FLAG_BIT : VISIBLE_FIRST_PASS_FLAG_BIT;
                
            return;
        }
        
        if (hasFlag(cmd.visabilityBit, NOT_VISIBLE_FLAG_BIT))
        {
            // Meshlet was invisible prev frame so we process it.
            perInstanceAttr instanceAttr = perInstanceBuffer[cmd.instanceIndex + push.frameIndex][cmd.instanceOffset];

            // Get first lod level to reference a meshlet.
            uint meshletOffsetFirstLodLevel = getMeshletOffset(cmd, 1);
            perMeshAttributes meshAttr = perMeshBuffer[cmd.meshIndex][cmd.meshOffset];
        
            uint selectedLod = selectLodLevel(meshAttr, dData, instanceAttr.modelTransform);
    
            uint meshletOffset = getMeshletOffset(cmd, selectedLod);
            
            // Overdraw for current lod level.
            if (meshletOffset == MAX_UINT)
            {
                commandOpaqueBuffer[push.cmdOpaqueBufferIndex][dtid].visabilityBit = NOT_VISIBLE_FLAG_BIT;
                return;
            }
            
            meshlet mesh = meshletBuffer[cmd.meshletIndex][meshletOffset];
            
            meshletBounds worldBounds = worldSpaceMeshletBounds(mesh.bounds, instanceAttr.modelTransform, meshAttr);
                    
            // Cone culling doesn't work for animated meshlets. On CPU cone calculation is wrong.
            bool visible = isFrontfaceMeshlet(dData, worldBounds) && isInFrustum(dData, worldBounds);
        
            if (!visible)
            {
                commandOpaqueBuffer[push.cmdOpaqueBufferIndex][dtid].visabilityBit = NOT_VISIBLE_FLAG_BIT;
                return;
            }
                
            cullingData occData = calculateCullingData(float4(worldBounds.center, worldBounds.radius), dData);
            
            commandOpaqueBuffer[push.cmdOpaqueBufferIndex][dtid].selectedLod = selectedLod;
            commandOpaqueBuffer[push.cmdOpaqueBufferIndex][dtid].visabilityBit = isOcluded(occData, dData) ? NOT_VISIBLE_FLAG_BIT : (hasFlag(cmd.visabilityBit, VISIBLE_FIRST_PASS_FLAG_BIT)) ? VISIBLE_FIRST_PASS_FLAG_BIT : VISIBLE_SECOND_PASS_FLAG_BIT;
            return;
        
        }
    }
}