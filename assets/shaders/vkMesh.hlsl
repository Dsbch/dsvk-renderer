//  dxc -T ms_6_9 -E msmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshMs.spv vkMesh.hlsl
//  dxc -T ps_6_9 -E psmain -spirv -fvk-use-scalar-layout -Fo vkCompiled/vkMeshPs.spv vkMesh.hlsl
//  dxc -T as_6_9 -E asmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshAs.spv vkMesh.hlsl
//  add -fspv-debug=vulkan-with-source flag only for debug.
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

// MS START.

groupshared float3 sharedPositions[VERTICES_PER_MESHLET];

struct meshletPrimitiveOut
{
    bool cullPrimitive : SV_CULLPRIMITIVE;
};

[outputtopology("triangle")]
[numthreads(MESHLET_THREAD_COUNT, 1, 1)]
void msmain(
                 uint gtid : SV_GroupThreadID,
                 uint gid : SV_GroupID,
    out indices uint3 triangles[TRIANGLES_PER_MESHLET],
    out vertices meshOutput vertices[VERTICES_PER_MESHLET],
    out primitives meshletPrimitiveOut primitives[TRIANGLES_PER_MESHLET])
{
    uint4 asData = compactBuffer[push.frameIndex][gid + 1];
    
    meshlet mesh = meshletBuffer[asData.x][asData.y];
    perInstanceAttr instanceAttr = perInstanceBuffer[asData.z][asData.w];
    perDrawData dData = drawData[push.frameIndex];
    
    instanceAttr.jointIndex += push.frameIndex;
    
    SetMeshOutputCounts(mesh.vertexCount, mesh.triangleCount);
    
    uint triangleID = gtid * 3;
    uint vertexID = gtid * 2;
    uint packedTriangles[3];
    
    [unroll]
    for (uint i = 0; i < TRIANGLE_LOOPS; i++)
    {
        if ((triangleID + i)  >= mesh.triangleCount)
            break;
        
        packedTriangles[i] = primitiveBuffer[mesh.triangleBufferIndex][mesh.triangleBufferOffset + triangleID + i];
    }
            
    [loop]
    for (uint k = 0; k < VERTEX_LOOPS; k++)
    {
        uint idx = vertexID + k;
        
        if (idx >= mesh.vertexCount)
            break;
            
        uint vertexOffset = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + idx] + mesh.vertexBufferOffset;
        uint weightOffset = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + idx] + mesh.weightBufferOffset;
        
        skinnedVertex skVertex = skinVertex(mesh.weightBufferIndex != MAX_UINT, instanceAttr, mesh.vertexBufferIndex, vertexOffset, weightOffset, mesh.weightBufferIndex);
        
        float4 worldPos = float4(transformPoint(instanceAttr.modelTransform, skVertex.position), 1.0f);
        
        sharedPositions[idx] = worldPos.xyz;
        
        vertices[idx].position = mul(dData.useDebugCamera ? dData.debugViewProjection : dData.viewProjection, worldPos);
        
        vertices[idx].uv = skVertex.textureCoords;
        vertices[idx].materialBase = instanceAttr.globalMaterialOffset + mesh.localMaterialOffset * 3;
        vertices[idx].worldPos = worldPos.xyz;
        vertices[idx].normal = rotate(instanceAttr.modelTransform.rotation, skVertex.normal);
        vertices[idx].tangent = float4(rotate(instanceAttr.modelTransform.rotation, skVertex.tangent.xyz), skVertex.tangent.w);
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    [unroll]
    for (uint d = 0; d < TRIANGLE_LOOPS; d++)
    {
        if ((triangleID + d) >= mesh.triangleCount)
            break;
        
        uint3 unpacked = unpackUint3(packedTriangles[d]);
        
        triangles[triangleID + d] = unpacked;
        
        primitives[triangleID + d].cullPrimitive = isBackface(
                dData,
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
    float4 albedo = materials[input.materialBase].Sample(materialsSampler[input.materialBase], input.uv);
    
    // Discard non solid geometry, in case for cutoff.
    if (albedo.a < 0.99f)
        discard;
    
    // Model rotation is already baked into tangent and normal.
    float3x3 TBN = calculateTBN(float4(0, 0, 0, 1), input.tangent, input.normal);
    perDrawData dData = drawData[push.frameIndex];
    
    float3 cameraPos = mul(dData.cameraPos, TBN);
    float3 worldPos = mul(input.worldPos, TBN);
    float3 cameraFront = normalize(mul(dData.cameraFront, TBN));
    
    float4 metalicRoughnes = materials[input.materialBase + 2].Sample(materialsSampler[input.materialBase + 2], input.uv);
    
    float4 normalTexture = materials[input.materialBase + 1].Sample(materialsSampler[input.materialBase + 1], input.uv);
    float3 normal = normalTexture.rgb * 2.0f - 1.0f;
    float metalic = metalicRoughnes.b;
    float roughnes = metalicRoughnes.g;
    
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
