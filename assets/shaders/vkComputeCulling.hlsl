//  dxc -T cs_6_9 -E main -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkComputeCulling.spv vkComputeCulling.hlsl
//  add -fspv-debug=vulkan-with-source flag only for debug.
#include "common.hlsl"

#define FIRST_OPAQUE_PASS_FLAG_BIT              (1 << 0)
#define SECOND_OPAQUE_PASS_FLAG_BIT             (1 << 1)
#define ACCUMILATION_PASS_FLAG_BIT              (1 << 2)

struct pushConstant
{
    uint hzbMipLevel;
    uint mipWidth;
    uint mipHeight;
    uint cullingPassFlagBit;
    uint opaqueCmdBufferIndex;
    uint meshletCount;
    uint hzbLength;
};

DEFINE_AS_PUSH_CONSTANT
pushConstant push;

Texture2D<float> originalZbuffer : register(t0, space0);
RWTexture2D<float> hzbChain[] : register(u1, space0);
RWStructuredBuffer<command> commandOpaqueBuffer[] : register(u2, space0);
RWStructuredBuffer<command> commandAccumilationBuffer : register(u3, space0);
StructuredBuffer<perMeshAttributes> perMeshBuffer[] : register(t4, space0);
StructuredBuffer<meshlet> meshletBuffer[] : register(t5, space0);
StructuredBuffer<perInstanceAttr> perInstanceBuffer[] : register(t6, space0);
ConstantBuffer<perDrawData> drawData : register(b7, space0);

struct occlusionCullingData
{
    // BS vertical/horizontal pixel length.
    float pixelLength;
    // Center of a sphere for zero mip level, meaning for original image.
    float2 sphereCenterUV;
    // Closest depth, from center to camera (0, 0).
    float closestDepth;
};

occlusionCullingData calculateOcclusionCullingData(float4 worldSpaceSphere, perDrawData drawData)
{
    occlusionCullingData result;
    
    // View space.
    float4 vsCenter = mul(drawData.view, float4(worldSpaceSphere.xyz, 1.0f));
    float4 vsBorder = float4(vsCenter.x, vsCenter.y + worldSpaceSphere.w, vsCenter.zw);
    float4 vsClosestToCamera = float4(vsCenter.x, vsCenter.y, vsCenter.z + worldSpaceSphere.w, vsCenter.w);
    
    // Clip space.
    float4 clipToCamera = mul(drawData.projection, vsClosestToCamera);
    float4 clipCenter = mul(drawData.projection, vsCenter);
    float4 clipBorder = mul(drawData.projection, vsBorder);
    
    // NDC space.
    float2 ndcToCamera = clipToCamera.xy / clipToCamera.w;
    
    result.closestDepth = clipToCamera.z / clipToCamera.w;
    
    float2 ndcCenter = clipCenter.xy / clipCenter.w;
    float2 ndcBorder = clipBorder.xy / clipBorder.w;
    
    float2 ndcNormalizedCenter = ((ndcCenter + 1.0f) / 2.0f);
    float2 ndcNormalizedBorder = ((ndcBorder + 1.0f) / 2.0f);
    
    uint2 ndcCenterPixel = uint2(uint(ndcNormalizedCenter.x * float(drawData.width)), uint(ndcNormalizedCenter.y * float(drawData.height)));
    uint2 ndcBorderPixel = uint2(uint(ndcNormalizedBorder.x * float(drawData.width)), uint(ndcNormalizedBorder.y * float(drawData.height)));
    
    result.pixelLength = 2.0f * length(float2(ndcCenterPixel) - float2(ndcBorderPixel));
    result.sphereCenterUV = ndcNormalizedCenter.xy;
    
    return result;
}

[numthreads(THREADS_COUNT, 1, 1)]
void main(uint dtid : SV_DispatchThreadID)
{
    if (dtid < push.meshletCount)
    {
        if (hasFlag(push.cullingPassFlagBit, ACCUMILATION_PASS_FLAG_BIT))
        {
            command cmd = commandAccumilationBuffer[dtid];
        
            perInstanceAttr instanceAttr = perInstanceBuffer[cmd.instanceIndex][cmd.instanceOffset];

            // Get first lod level to reference a meshlet.
            uint meshletOffsetFirstLodLevel = getMeshletOffset(cmd, 1);
            meshlet mesh = meshletBuffer[cmd.meshletIndex][meshletOffsetFirstLodLevel];
            perMeshAttributes meshAttr = perMeshBuffer[mesh.perMeshBufferIndex][mesh.perMeshBufferOffset];
        
            uint selectedLod = selectLodLevel(meshAttr, drawData, instanceAttr.modelTransform);
            uint meshletOffset = getMeshletOffset(cmd, selectedLod);
            
            // Overdraw for current lod level.
            if (meshletOffset == MAX_UINT)
            {
                commandAccumilationBuffer[dtid].visabilityBit = NOT_VISIBLE_FLAG_BIT;
                return;
            }
            
            mesh = meshletBuffer[cmd.meshletIndex][meshletOffset];
            
            meshletBounds worldBounds = worldSpaceMeshletBounds(mesh.bounds, instanceAttr.modelTransform, meshAttr);
            
            bool visible = mesh.alphaType == BLEND_ALPHA_MODE && isInFrustum(drawData, worldBounds);
            
            commandAccumilationBuffer[dtid].selectedLod = selectedLod;
            commandAccumilationBuffer[dtid].visabilityBit = visible ? VISIBLE_FLAG_BIT : NOT_VISIBLE_FLAG_BIT;
            
            return;
        }
        
        if (hasFlag(push.cullingPassFlagBit, FIRST_OPAQUE_PASS_FLAG_BIT))
        {
            command cmd = commandOpaqueBuffer[push.opaqueCmdBufferIndex][dtid];
            
            // Meshlet was visible prev frame so we process it.
            if (hasFlag(cmd.visabilityBit, VISIBLE_FLAG_BIT) || hasFlag(cmd.visabilityBit, VISIBLE_CURRENT_FRAME_FLAG_BIT))
            {
                perInstanceAttr instanceAttr = perInstanceBuffer[cmd.instanceIndex][cmd.instanceOffset];

                // Get first lod level to reference a meshlet.
                uint meshletOffsetFirstLodLevel = getMeshletOffset(cmd, 1);
                meshlet mesh = meshletBuffer[cmd.meshletIndex][meshletOffsetFirstLodLevel];
                perMeshAttributes meshAttr = perMeshBuffer[mesh.perMeshBufferIndex][mesh.perMeshBufferOffset];
        
                uint selectedLod = selectLodLevel(meshAttr, drawData, instanceAttr.modelTransform);
    
                uint meshletOffset = getMeshletOffset(cmd, selectedLod);
            

                // Overdraw for current lod level.
                if (meshletOffset == MAX_UINT)
                {
                    commandAccumilationBuffer[dtid].visabilityBit = NOT_VISIBLE_CURRENT_FRAME_FLAG_BIT;
                    return;
                }
            
                mesh = meshletBuffer[cmd.meshletIndex][meshletOffset];
            
                meshletBounds worldBounds = worldSpaceMeshletBounds(mesh.bounds, instanceAttr.modelTransform, meshAttr);
            
                // Cone culling doesn't work for animated meshlets. On CPU cone calculation is wrong.
            
                bool visible = isFrontfaceMeshlet(drawData, worldBounds) &&
                    isInFrustum(drawData, worldBounds);
            
                commandOpaqueBuffer[push.opaqueCmdBufferIndex][dtid].selectedLod = selectedLod;
                commandOpaqueBuffer[push.opaqueCmdBufferIndex][dtid].visabilityBit = visible ? VISIBLE_CURRENT_FRAME_FLAG_BIT : NOT_VISIBLE_CURRENT_FRAME_FLAG_BIT;
            }
            
            return;
        }
        
        if (hasFlag(push.cullingPassFlagBit, SECOND_OPAQUE_PASS_FLAG_BIT))
        {
            command cmd = commandOpaqueBuffer[push.opaqueCmdBufferIndex][dtid];
        
            // Preapre for next frame.
            if (hasFlag(cmd.visabilityBit, NOT_VISIBLE_CURRENT_FRAME_FLAG_BIT))
            {
                commandOpaqueBuffer[push.opaqueCmdBufferIndex][dtid].visabilityBit = NOT_VISIBLE_FLAG_BIT;
                return;
            }
            
            // Retest each meshlet that was drawn in first PASS. 
            if (hasFlag(cmd.visabilityBit, VISIBLE_CURRENT_FRAME_FLAG_BIT))
            {
                perInstanceAttr instanceAttr = perInstanceBuffer[cmd.instanceIndex][cmd.instanceOffset];

                // Get first lod level to reference a meshlet.
                uint meshletOffsetFirstLodLevel = getMeshletOffset(cmd, 1);
                meshlet mesh = meshletBuffer[cmd.meshletIndex][meshletOffsetFirstLodLevel];
                perMeshAttributes meshAttr = perMeshBuffer[mesh.perMeshBufferIndex][mesh.perMeshBufferOffset];
        
                uint meshletOffset = getMeshletOffset(cmd, cmd.selectedLod);
            
                // Overdraw for current lod level.
                if (meshletOffset == MAX_UINT)
                {
                    commandAccumilationBuffer[dtid].visabilityBit = NOT_VISIBLE_FLAG_BIT;
                    return;
                }
            
                mesh = meshletBuffer[cmd.meshletIndex][meshletOffset];
            
                meshletBounds worldBounds = worldSpaceMeshletBounds(mesh.bounds, instanceAttr.modelTransform, meshAttr);
                
                occlusionCullingData occData = calculateOcclusionCullingData(float4(worldBounds.center, worldBounds.radius), drawData);
            
                // Floor, because we will sample 2x2 texels for that sphere.
                uint neededChain = uint(floor(log2(max(1.0f, occData.pixelLength))));
                    
                neededChain = min(push.hzbLength - 1, neededChain);
                    
                // drawData.width >> neededChain => divide by 2 in power of neededChain.
                // drawData.width / 2 because zero mip starts with drawData.width / 2.
                int mipWidth = max(1, int(drawData.width / 2) >> (neededChain));
                int mipHeight = max(1, int(drawData.height / 2) >> (neededChain));

                float2 mipTexelCoords = occData.sphereCenterUV * float2(mipWidth, mipHeight) - 0.5f;

                int2 topLeftTexel = clamp(int2(floor(mipTexelCoords)), int2(0, 0), int2(mipWidth - 2, mipHeight - 2));

                float d00 = hzbChain[neededChain][topLeftTexel + int2(0, 0)];
                float d10 = hzbChain[neededChain][topLeftTexel + int2(1, 0)];
                float d01 = hzbChain[neededChain][topLeftTexel + int2(0, 1)];
                float d11 = hzbChain[neededChain][topLeftTexel + int2(1, 1)];

                float minDepth = min(min(d00, d10), min(d01, d11));
                
                bool visible = (occData.closestDepth >= minDepth) || occData.closestDepth < 0.0f;
                
                commandOpaqueBuffer[push.opaqueCmdBufferIndex][dtid].visabilityBit = !visible ? NOT_VISIBLE_FLAG_BIT : VISIBLE_FLAG_BIT;
                
                return;
            }
            
            // Meshlet was invisible prev frame so we process it.
            if (hasFlag(cmd.visabilityBit, NOT_VISIBLE_FLAG_BIT))
            {
                perInstanceAttr instanceAttr = perInstanceBuffer[cmd.instanceIndex][cmd.instanceOffset];

                // Get first lod level to reference a meshlet.
                uint meshletOffsetFirstLodLevel = getMeshletOffset(cmd, 1);
                meshlet mesh = meshletBuffer[cmd.meshletIndex][meshletOffsetFirstLodLevel];
                perMeshAttributes meshAttr = perMeshBuffer[mesh.perMeshBufferIndex][mesh.perMeshBufferOffset];
        
                uint selectedLod = selectLodLevel(meshAttr, drawData, instanceAttr.modelTransform);
    
                uint meshletOffset = getMeshletOffset(cmd, selectedLod);
            
                // Overdraw for current lod level.
                if (meshletOffset == MAX_UINT)
                {
                    commandAccumilationBuffer[dtid].visabilityBit = NOT_VISIBLE_FLAG_BIT;
                    return;
                }
            
                mesh = meshletBuffer[cmd.meshletIndex][meshletOffset];
            
                meshletBounds worldBounds = worldSpaceMeshletBounds(mesh.bounds, instanceAttr.modelTransform, meshAttr);
            
                // Cone culling doesn't work for animated meshlets. On CPU cone calculation is wrong.
               
                bool visible = isFrontfaceMeshlet(drawData, worldBounds) &&
                    isInFrustum(drawData, worldBounds);
            
                // Oclussion culling.
                if (visible)
                {
                    occlusionCullingData occData = calculateOcclusionCullingData(float4(worldBounds.center, worldBounds.radius), drawData);
            
                    // Floor, because we will sample 2x2 texels for that sphere.
                    uint neededChain = uint(floor(log2(max(1.0f, occData.pixelLength))));
                    
                    neededChain = min(push.hzbLength - 1, neededChain);
                    
                    // drawData.width >> neededChain => divide by 2 in power of neededChain.
                    // drawData.width / 2 because zero mip starts with drawData.width / 2.
                    int mipWidth = max(1, int(drawData.width / 2) >> (neededChain));
                    int mipHeight = max(1, int(drawData.height / 2) >> (neededChain));

                    float2 mipTexelCoords = occData.sphereCenterUV * float2(mipWidth, mipHeight) - 0.5f;

                    int2 topLeftTexel = clamp(int2(floor(mipTexelCoords)), int2(0, 0), int2(mipWidth - 2, mipHeight - 2));

                    float d00 = hzbChain[neededChain][topLeftTexel + int2(0, 0)];
                    float d10 = hzbChain[neededChain][topLeftTexel + int2(1, 0)];
                    float d01 = hzbChain[neededChain][topLeftTexel + int2(0, 1)];
                    float d11 = hzbChain[neededChain][topLeftTexel + int2(1, 1)];

                    float minDepth = min(min(d00, d10), min(d01, d11));
                    
                    visible = (occData.closestDepth >= minDepth) || occData.closestDepth < 0.0f;
                }
                
                commandOpaqueBuffer[push.opaqueCmdBufferIndex][dtid].selectedLod = selectedLod;
                commandOpaqueBuffer[push.opaqueCmdBufferIndex][dtid].visabilityBit = visible ? VISIBLE_CURRENT_FRAME_FLAG_BIT : NOT_VISIBLE_FLAG_BIT;
            }
            
            return;
        }
    }
}