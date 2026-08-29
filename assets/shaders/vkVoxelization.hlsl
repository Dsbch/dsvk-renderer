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

// TS START.

struct MeshShaderPayload
{
    uint meshletIndex[THREADS_COUNT];
    uint meshletOffset[THREADS_COUNT];
    uint perInstanceIndex[THREADS_COUNT];
    uint perInstanceOffset[THREADS_COUNT];
};

groupshared MeshShaderPayload payload;

[numthreads(THREADS_COUNT, 1, 1)]
void asmain(
    uint gtid : SV_GroupThreadID,
    uint dtid : SV_DispatchThreadID,
    uint gid : SV_GroupID
)
{
    bool visible = dtid < push.cmdBufferCount;
    
    // Not overdraw.
    if (visible)
    {
        command cmd = commandOpaqueBuffer[push.cmdOpaqueBufferIndex][dtid];
        uint meshletOffset = getMeshletOffset(cmd, cmd.selectedLod);
    
        uint index = WavePrefixCountBits(visible);
        
        payload.perInstanceIndex[index] = cmd.instanceIndex + push.frameIndex;
        payload.perInstanceOffset[index] = cmd.instanceOffset;
        payload.meshletIndex[index] = cmd.meshletIndex;
        payload.meshletOffset[index] = meshletOffset;
    }
    
    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, payload);
}
// TS END.

// MS START.

struct meshletPrimitiveOut
{
    bool cullPrimitive : SV_CULLPRIMITIVE;
};

[outputtopology("triangle")]
[numthreads(THREADS_COUNT, 1, 1)]
void msmain(
                 uint gtid : SV_GroupThreadID,
                 uint gid : SV_GroupID,
    in payload MeshShaderPayload payload,
    out indices uint3 triangles[THREADS_COUNT],
    out vertices meshOutput vertices[THREADS_COUNT]
)
{
    meshlet mesh = meshletBuffer[payload.meshletIndex[gid]][payload.meshletOffset[gid]];
    perInstanceAttr instanceAttr = perInstanceBuffer[payload.perInstanceIndex[gid]][payload.perInstanceOffset[gid]];
    perMeshAttributes meshAttr = perMeshBuffer[mesh.perMeshBufferIndex][mesh.perMeshBufferOffset];
    perDrawData dData = drawData[push.frameIndex];
    
    SetMeshOutputCounts(mesh.vertexCount, mesh.triangleCount);
        
    if (gtid < mesh.vertexCount)
    {
        uint vertexOffset = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + gtid] + mesh.vertexBufferOffset;
        uint weightOffset = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + gtid] + mesh.weightBufferOffset;
        
        instanceAttr.jointIndex += push.frameIndex;
        
        skinnedVertex skVertex = skinVertex(instanceAttr, meshAttr, mesh.vertexBufferIndex, vertexOffset, weightOffset, mesh.weightBufferIndex);
        
        float4 worldPos = float4(transformPoint(instanceAttr.modelTransform, skVertex.position), 1.0f);
        
        vertices[gtid].position = mul(dData.viewProjectionVoxel, worldPos);
        
        vertices[gtid].uv = skVertex.textureCoords;
        vertices[gtid].materialBase = instanceAttr.globalMaterialOffset + mesh.localMaterialOffset * 3;
        vertices[gtid].worldPos = mul(dData.viewVoxel, worldPos).xyz;
        vertices[gtid].normal = rotate(instanceAttr.modelTransform.rotation, skVertex.normal);
        vertices[gtid].tangent = float4(rotate(instanceAttr.modelTransform.rotation, skVertex.tangent.xyz), skVertex.tangent.w);
    }
    
    if (gtid < mesh.triangleCount)
    {
        uint packed = primitiveBuffer[mesh.triangleBufferIndex][mesh.triangleBufferOffset + gtid];
         
        uint3 unpacked = unpackUint(packed);
        
        triangles[gtid] = unpacked;
    }
}

// MESH SHADER END.

// PIXEL SHADER START.

void psmain(meshOutput input)
{
    input.worldPos += drawData[push.frameIndex].voxelSceneUpperBound * 0.5f;
    
    int3 gridCoords = clamp(int3(input.worldPos * drawData[push.frameIndex].voxelGridExtent / drawData[push.frameIndex].voxelSceneUpperBound), 0, drawData[push.frameIndex].voxelGridExtent - 1);
    
    float4 albedo = materials[input.materialBase].SampleLevel(materialsSampler[input.materialBase], input.uv, 0);
    
    clipMap[gridCoords] = albedo;
}

// PIXEL SHADER END.
