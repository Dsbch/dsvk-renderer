//  dxc -T ms_6_9 -E msmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshAccumilationMs.spv vkAccumilation.hlsl
//  dxc -T ps_6_9 -E psmain -spirv -fvk-use-scalar-layout -Fo vkCompiled/vkMeshAccumilationPs.spv vkAccumilation.hlsl
//  dxc -T as_6_9 -E asmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshAccumilationAs.spv vkAccumilation.hlsl
//  add -fspv-debug=vulkan-with-source flag only for debug.
#define NEED_BINDINGS
#include "common.hlsl"

// DescriptorSets END.

// Push constant START.
struct pushConstant
{
    uint meshletCount;
    uint opaqueCmdBufferIndex;
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
    float visible = false;
    
    // Not overdraw.
    if (dtid < push.meshletCount)
    {
        command cmd = commandAccumilationBuffer[dtid];
        uint meshletOffset = getMeshletOffset(cmd, cmd.selectedLod);
    
        visible = hasFlag(cmd.visabilityBit, VISIBLE_FLAG_BIT);
            
        if (visible)
        {
            uint index = WavePrefixCountBits(visible);
        
            payload.perInstanceIndex[index] = cmd.instanceIndex;
            payload.perInstanceOffset[index] = cmd.instanceOffset;
            payload.meshletIndex[index] = cmd.meshletIndex;
            payload.meshletOffset[index] = meshletOffset;
        }
    }
    
    uint visibleCount = WaveActiveCountBits(visible);
    DispatchMesh(visibleCount, 1, 1, payload);
}
// TS END.

// MS START.

[outputtopology("triangle")]
[numthreads(THREADS_COUNT, 1, 1)]
void msmain(
                 uint gtid : SV_GroupThreadID,
                 uint gid : SV_GroupID,
    in payload MeshShaderPayload payload,
    out indices uint3 triangles[THREADS_COUNT],
    out vertices meshOutput vertices[THREADS_COUNT])
{
    meshlet mesh = meshletBuffer[payload.meshletIndex[gid]][payload.meshletOffset[gid]];
    perInstanceAttr instanceAttr = perInstanceBuffer[payload.perInstanceIndex[gid]][payload.perInstanceOffset[gid]];
    perMeshAttributes meshAttr = perMeshBuffer[mesh.perMeshBufferIndex][mesh.perMeshBufferOffset];
    
    SetMeshOutputCounts(mesh.vertexCount, mesh.triangleCount);
        
    if (gtid < mesh.triangleCount)
    {
        uint packed = primitiveBuffer[mesh.triangleBufferIndex][mesh.triangleBufferOffset + gtid];
         
        uint3 unpacked = unpackUint(packed);
        
        triangles[gtid] = unpacked;
    }

    if (gtid < mesh.vertexCount)
    {
        uint vertexOffset = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + gtid] + mesh.vertexBufferOffset;
        uint weightOffset = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + gtid] + mesh.weightBufferOffset;
        
        skinnedVertex skVertex = skinVertex(instanceAttr, meshAttr, mesh.vertexBufferIndex, vertexOffset, weightOffset, mesh.weightBufferIndex);
        
        float4 worldPos = float4(transformPoint(instanceAttr.modelTransform, skVertex.position), 1.0f);
        
        vertices[gtid].position = mul(drawData.useDebugCamera ? drawData.debugViewProjection : drawData.viewProjection, worldPos);
        
        vertices[gtid].uv = skVertex.textureCoords;
        vertices[gtid].albedoIndex = instanceAttr.globalMaterialOffset + mesh.localMaterialOffset * 3;
        vertices[gtid].normalIndex = instanceAttr.globalMaterialOffset + mesh.localMaterialOffset * 3 + 1;
        vertices[gtid].metallicRoughnessIndex = instanceAttr.globalMaterialOffset + mesh.localMaterialOffset * 3 + 2;

        vertices[gtid].worldPos = worldPos.xyz;
        vertices[gtid].cameraPos = drawData.cameraPos;
        vertices[gtid].normal = skVertex.normal;
        vertices[gtid].tangent = skVertex.tangent;
        vertices[gtid].rotation = instanceAttr.modelTransform.rotation;
        vertices[gtid].cameraFront = drawData.cameraFront;
    }
}

// MESH SHADER END.

// PIXEL SHADER END.

struct PSOutput
{
    float4 accum : SV_TARGET0;
    float4 reveal : SV_TARGET1;
};

// All calculations are made in tangent space.
PSOutput psmain(meshOutput input)
{
    float3x3 TBN = calculateTBN(input.rotation, input.tangent, input.normal);
    
    float3 cameraPos = mul(input.cameraPos, TBN);
    float3 cameraFront = normalize(mul(input.cameraFront, TBN));
    float3 worldPos = mul(input.worldPos, TBN);
    
    float4 metalicRoughnes = materials[input.metallicRoughnessIndex].Sample(materialsSampler[input.metallicRoughnessIndex], input.uv);

    float4 albedo = materials[input.albedoIndex].Sample(materialsSampler[input.albedoIndex], input.uv);
    float4 normalTexture = materials[input.normalIndex].Sample(materialsSampler[input.normalIndex], input.uv);
    float3 normal = normalTexture.rgb * 2.0f - 1.0f;
    float metalic = metalicRoughnes.b;
    float roughnes = metalicRoughnes.g;
    
    // Discard solid geometry.
    if (albedo.a >= 0.99f)
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
    
    float alpha = albedo.a;
    
    float weight = clamp(pow(min(1.0f, alpha * 10.0f) + 0.01f, 3.0f) * 1e8f *
                   pow(1.0f - input.position.z * 0.9f, 3.0f), 1e-2f, 3e3f);
    
    PSOutput output;
    output.accum = float4(color.rgb * alpha, alpha) * weight;
    output.reveal = float4(alpha, 0.0f, 0.0f, 1.0f);
    
    return output;
}

// PIXEL SHADER END.
