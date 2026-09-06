//  dxc -T ms_6_9 -E msmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshVoxelRayMarchMs.spv vkVoxelRayMarch.hlsl
//  dxc -T ps_6_9 -E psmain -spirv -fvk-use-scalar-layout -Fo vkCompiled/vkMeshVoxelRayMarchPs.spv vkVoxelRayMarch.hlsl
//  dxc -T as_6_9 -E asmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshVoxelRayMarchAs.spv vkVoxelRayMarch.hlsl
//  add -fspv-debug=vulkan-with-source flag only for debug.
#include "common.hlsl"

struct pushConstant
{
    uint frameIndex;
    uint cmdBufferCount;
    uint cmdOpaqueBufferIndex;
};

DEFINE_AS_PUSH_CONSTANT
pushConstant push;

// TS START.

struct Payload
{
};

groupshared Payload payload;

[numthreads(1, 1, 1)]
void asmain(
    uint gtid : SV_GroupThreadID,
    uint dtid : SV_DispatchThreadID,
    uint gid : SV_GroupID
)
{
    // Dispatch 1 meshlet for full screen pass.
    DispatchMesh(1, 1, 1, payload);
}
// TS END.

// MS START.

struct MeshOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

[outputtopology("triangle")]
[numthreads(3, 1, 1)]
void msmain(
    uint gtid : SV_GroupThreadID,
    in payload Payload meshPayload,
    out indices uint3 triangles[1],
    out vertices MeshOutput verts[3])
{
    SetMeshOutputCounts(3, 1);

    if (gtid == 0)
        triangles[0] = uint3(0, 1, 2);

    float2 uv = float2((gtid << 1) & 2, gtid & 2);
    verts[gtid].position = float4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
    verts[gtid].uv = uv;
}

// MESH SHADER END.

// PIXEL SHADER START.

bool intersectBox(float3 origin, float3 dir, float3 bmin, float3 bmax, out float tEnter, out float tExit)
{
    float3 inv = 1.0f / dir;

    float3 ta = (bmin - origin) * inv;
    float3 tb = (bmax - origin) * inv;

    float3 tlo = min(ta, tb);
    float3 thi = max(ta, tb);

    tEnter = max(max(tlo.x, tlo.y), tlo.z);
    tExit = min(min(thi.x, thi.y), thi.z);

    return tExit >= max(tEnter, 0.0f);
}

float4 psmain(MeshOutput input) : SV_TARGET
{
    perDrawData dData = drawData[push.frameIndex];
    
    float3 dir = pixelToRayDir(input.position.xy, dData);
    float3 pos = dData.cameraPos;
    
    const float voxelSize = float(dData.voxelSceneExtent) / float(dData.clipMapResolution);
    const float voxelExtentWS = dData.voxelSceneExtent * 0.5f;
    
    // if we outside of voxel box I need to snap pos to intersection.
    float tEnter, tExit;
    if (!intersectBox(pos, dir, -voxelExtentWS.xxx, voxelExtentWS.xxx, tEnter, tExit))
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    
    if (tEnter > 0.0f)
        pos += dir * tEnter;
        
    const float3 delta = voxelSize.xxx / abs(dir);
    
    float3 g = pos + voxelExtentWS;

    int3 voxelPos = worldPosToVoxel(pos, dData);
    float3 offset = (g - voxelPos * voxelSize);
    float3 traveled = -offset / dir;
    
    for (int i = 0; i < 3 * dData.clipMapResolution; i++)
    {
        float4 voxel = clipMap[voxelPos];
        
        if (voxel.w > 0.0f)
            return voxel;
        
        if (traveled.x + delta.x < traveled.y + delta.y && traveled.x + delta.x < traveled.z + delta.z)
        {
            traveled.x += delta.x;
            voxelPos.x += dir.x < 0.0f ? -1 : 1;
        }
        else if (traveled.y + delta.y < traveled.z + delta.z)
        {
            traveled.y += delta.y;
            voxelPos.y += dir.y < 0.0f ? -1 : 1;
        }
        else
        {
            traveled.z += delta.z;
            voxelPos.z += dir.z < 0.0f ? -1 : 1;
        }
        
        if (any(voxelPos < 0) || any(voxelPos >= int(dData.clipMapResolution)))
            break;
    }
    
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
}

// PIXEL SHADER END.
