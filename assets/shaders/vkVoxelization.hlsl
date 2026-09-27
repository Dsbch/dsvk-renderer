//  dxc -T ms_6_9 -E msmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkVoxelizationMs.spv vkVoxelization.hlsl
//  dxc -T ps_6_9 -E psmain -spirv -fvk-use-scalar-layout -Fo vkCompiled/vkVoxelizationPs.spv vkVoxelization.hlsl
//  dxc -T as_6_9 -E asmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkVoxelizationAs.spv vkVoxelization.hlsl
//  add -fspv-debug=vulkan-with-source flag only for debug.
#include "common.hlsl"

// DescriptorSets END.

struct pushConstant
{
    uint frameIndex;
    uint cmdBufferCount;
    uint cmdOpaqueBufferIndex;
};

DEFINE_AS_PUSH_CONSTANT
pushConstant push;

// MS START.

[outputtopology("triangle")]
[numthreads(THREADS_COUNT, 1, 1)]
void msmain(
                 uint gtid : SV_GroupThreadID,
                 uint gid : SV_GroupID,
    out indices uint3 triangles[THREADS_COUNT],
    out vertices meshOutput vertices[THREADS_COUNT]
)
{
    command cmd = commandOpaqueBuffer[push.cmdOpaqueBufferIndex + push.frameIndex][gid];
    meshlet mesh = meshletBuffer[cmd.meshletIndex][cmd.meshletOffset1];
    perInstanceAttr instanceAttr = perInstanceBuffer[cmd.instanceIndex + push.frameIndex][cmd.instanceOffset];
    perDrawData dData = drawData[push.frameIndex];
    
    SetMeshOutputCounts(mesh.vertexCount, mesh.triangleCount);
     
    if (gtid < mesh.triangleCount)
    {
        uint packed = primitiveBuffer[mesh.triangleBufferIndex][mesh.triangleBufferOffset + gtid];
         
        uint3 unpacked = unpackUint3(packed);
        
        triangles[gtid] = unpacked;
    }
    
    if (gtid < mesh.vertexCount)
    {
        uint vertexOffset = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + gtid] + mesh.vertexBufferOffset;
        uint weightOffset = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + gtid] + mesh.weightBufferOffset;
        
        instanceAttr.jointIndex += push.frameIndex;
        
        skinnedVertex skVertex = skinVertex(mesh.weightBufferIndex != MAX_UINT, instanceAttr, mesh.vertexBufferIndex, vertexOffset, weightOffset, mesh.weightBufferIndex);
        
        float4 worldPos = float4(transformPoint(instanceAttr.modelTransform, skVertex.position), 1.0f);
        
        vertices[gtid].position = mul(dData.viewProjectionVoxel, worldPos);
        
        vertices[gtid].uv = skVertex.textureCoords;
        vertices[gtid].materialBase = instanceAttr.globalMaterialOffset + mesh.localMaterialOffset * 3;
        vertices[gtid].worldPos = mul(dData.viewVoxel, worldPos).xyz;
        vertices[gtid].normal = rotate(instanceAttr.modelTransform.rotation, skVertex.normal);
        vertices[gtid].tangent = float4(rotate(instanceAttr.modelTransform.rotation, skVertex.tangent.xyz), skVertex.tangent.w);
    }
}

// MESH SHADER END.

// PIXEL SHADER START.

void psmain(meshOutput input)
{
    int3 voxelCoords = worldPosToVoxel(input.worldPos, drawData[push.frameIndex]);
    
    float4 albedo = materials[input.materialBase].SampleLevel(materialsSampler[input.materialBase], input.uv, 0);
    
    clipMap[voxelCoords] = float4(albedo.xyz, 1.0f);
}

// PIXEL SHADER END.
