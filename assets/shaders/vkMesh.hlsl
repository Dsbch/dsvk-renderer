//  dxc -T ms_6_9 -E msmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshMs.spv vkMesh.hlsl
//  dxc -T ps_6_9 -E psmain -spirv -fvk-use-scalar-layout -Fo vkCompiled/vkMeshPs.spv vkMesh.hlsl
//  dxc -T as_6_9 -E asmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshAs.spv vkMesh.hlsl
//  add -fspv-debug=vulkan-with-source flag only for debug.
#define NEED_BINDINGS
#include "common.hlsl"

// DescriptorSets END.

// Push constant START.
struct pushConstant
{
	uint frameIndex;
    uint cmdBufferCount;
    uint cmdOpaqueBufferIndex;
};

DEFINE_AS_PUSH_CONSTANT
pushConstant push;

// Push constant END.

// INPUT END.

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
    bool visible = dtid < visabilityBuffer[push.frameIndex][0];
    
    // Not overdraw.
    if (visible)
    {
        command cmd = commandOpaqueBuffer[push.cmdOpaqueBufferIndex][visabilityBuffer[push.frameIndex][dtid + 4]];
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

groupshared float3 sharedPositions[THREADS_COUNT];

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
    out vertices meshOutput vertices[THREADS_COUNT],
    out primitives meshletPrimitiveOut primitives[THREADS_COUNT])
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
        
        sharedPositions[gtid] = skVertex.position;
        
        vertices[gtid].position = mul(dData.useDebugCamera ? dData.debugViewProjection : dData.viewProjection, worldPos);
        
        vertices[gtid].uv = skVertex.textureCoords;
        vertices[gtid].materialBase = instanceAttr.globalMaterialOffset + mesh.localMaterialOffset * 3;
        vertices[gtid].worldPos = worldPos.xyz;
        vertices[gtid].normal = rotate(instanceAttr.modelTransform.rotation, skVertex.normal);
        vertices[gtid].tangent = float4(rotate(instanceAttr.modelTransform.rotation, skVertex.tangent.xyz), skVertex.tangent.w);
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    if (gtid < mesh.triangleCount)
    {
        uint packed = primitiveBuffer[mesh.triangleBufferIndex][mesh.triangleBufferOffset + gtid];
         
        uint3 unpacked = unpackUint(packed);
        
        triangles[gtid] = unpacked;
        
        primitives[gtid].cullPrimitive = isBackface(
                dData,
                instanceAttr.modelTransform,
                sharedPositions[unpacked.x],
                sharedPositions[unpacked.y],
                sharedPositions[unpacked.z]
            );
    }
}

// MESH SHADER END.

// PIXEL SHADER START.

// All calculations are made in tangent space.
float4 psmain(meshOutput input) : SV_TARGET
{
    // Model rotation is already baked into tangent and normal.
    float3x3 TBN = calculateTBN(float4(0, 0, 0, 1), input.tangent, input.normal);
    perDrawData dData = drawData[push.frameIndex];
    
    float3 cameraPos = mul(dData.cameraPos, TBN);
    float3 worldPos = mul(input.worldPos, TBN);
    float3 cameraFront = normalize(mul(dData.cameraFront, TBN));
    
    float4 metalicRoughnes = materials[input.materialBase + 2].Sample(materialsSampler[input.materialBase + 2], input.uv);

    float4 albedo = materials[input.materialBase].Sample(materialsSampler[input.materialBase], input.uv);
    float4 normalTexture = materials[input.materialBase + 1].Sample(materialsSampler[input.materialBase + 1], input.uv);
    float3 normal = normalTexture.rgb * 2.0f - 1.0f;
    float metalic = metalicRoughnes.b;
    float roughnes = metalicRoughnes.g;
    
    // Discard non solid geometry, in case for cutoff.
    if (albedo.a < 0.99f)
        discard;
   
    normal = normalize(normal);

    albedo = float4(toRGB(albedo.rgb), albedo.a);
    
    // render equation.
    float3 V = normalize(cameraPos - worldPos);
    float3 l0 = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 1; ++i)
    {
        float3 lightPos = cameraPos + cameraFront / 4.0f;
        float3 lightColor = float3(3.0f, 3.0f, 3.0f);

        float3 L = normalize(lightPos - worldPos);
        float3 H = normalize(L + V);

        // radiance per per light source.
        float3 radiance = lightRadiance(lightColor, length(lightPos - worldPos));

        // Cook-Torrance BRDF
        float d = distributionGGX(normal, H, roughnes);
        float g = geometrySmith(normal, V, L, roughnes);
        float3 f = fresnelSchlick(max(dot(H, V), 0.0), baseReflectivity(albedo.rgb, metalic));

        float3 numerator = d * f * g;
        float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0) + 0.0001;
        // + 0.0001 to prevent divide by zero
        float3 specular = numerator / denominator;

        // kS is equal to Fresnel
        float3 kS = f;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
        
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metalic;

        // scale light by nDotL
        float nDotL = max(dot(normal, L), 0.0f);
        
        // add to outgoing radiance Lo
        l0 += (kD * albedo.rgb / PI + specular) * radiance * nDotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

     // Add simple ambient lighting (constant 0.03 should be replaced with IBL or VCT)
    float3 ambient = float3(0.03, 0.03, 0.03) * albedo.rgb * normalTexture.a;
    
    float3 color = ambient + l0;

    // HDR tonemapping
    color = color / (color + float3(1.0f, 1.0f, 1.0f));
    
    color = toSRGB(color);
    
    return float4(color, albedo.a);
}

// PIXEL SHADER END.
