//  dxc -T ms_6_9 -E msmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshMs.spv vkMesh.hlsl
//  dxc -T ps_6_9 -E psmain -spirv -fvk-use-scalar-layout -Fo vkCompiled/vkMeshPs.spv vkMesh.hlsl
//  dxc -T as_6_9 -E asmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshAs.spv vkMesh.hlsl
//  add -fspv-debug=vulkan-with-source flag only for debug.
#include "common.hlsl"

// UBO START.

ConstantBuffer<perDrawData> drawData : register(b6, space0);

// UBO END.

// TEXTURES START.

Texture2D materials[] : register(t7, space0);
SamplerState materialsSampler[] : register(s7, space0);

// TEXTURES END.

// DescriptorSets END.

// Push constant START.
struct pushConstant
{
    uint commandBufferOffset;
    uint meshletCount;
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
    uint lodLevel[THREADS_COUNT];
};

groupshared MeshShaderPayload payload;

[numthreads(THREADS_COUNT, 1, 1)]
void asmain(
    uint gtid : SV_GroupThreadID,
    uint dtid : SV_DispatchThreadID,
    uint gid : SV_GroupID
)
{
    const uint maxUint = 4294967295;
    
    float visible = false;
    
    // Not overdraw.
    if (dtid < push.meshletCount)
    {
        uint perInstanceIndex = commandBuffer[dtid + push.commandBufferOffset].instanceIndex;
        uint perInstanceOffset = commandBuffer[dtid + push.commandBufferOffset].instanceOffset;
    
        perInstanceAttr instanceAttr = perInstanceBuffer[perInstanceIndex][perInstanceOffset];
        uint selectedLod = selectLodLevel(drawData, instanceAttr.bsWorldCenter, instanceAttr.bsWorldRadius);
        uint meshletIdx = commandBuffer[dtid + push.commandBufferOffset].meshletIndex;
        uint meshletOffset = getMeshletOffset(selectedLod, dtid + push.commandBufferOffset);
    
        // Still have meshlets for that lodLevel.
            if (meshletOffset != maxUint)
            {
                meshlet mesh = meshletBuffer[meshletIdx][meshletOffset];
            
                visible =
                isFrontfaceMeshlet(drawData, instanceAttr.modelTransform, mesh.bounds.coneAxis, mesh.bounds.center, mesh.bounds.coneCutoff) &&
                isInFrustum(drawData, instanceAttr.modelTransform, mesh.bounds.center, mesh.bounds.radius);
            
                if (visible)
                {
                    uint index = WavePrefixCountBits(visible);
        
                    payload.perInstanceIndex[index] = perInstanceIndex;
                    payload.perInstanceOffset[index] = perInstanceOffset;
     
                    payload.lodLevel[index] = selectedLod;
        
                    payload.meshletIndex[index] = meshletIdx;
                    payload.meshletOffset[index] = meshletOffset;
                }
            }
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
    out vertices meshOutput vertices[THREADS_COUNT],
    out primitives meshletPrimitiveOut primitives[THREADS_COUNT])
{
    meshlet mesh = meshletBuffer[payload.meshletIndex[gid]][payload.meshletOffset[gid]];
    perInstanceAttr instanceAttr = perInstanceBuffer[payload.perInstanceIndex[gid]][payload.perInstanceOffset[gid]];
    
    SetMeshOutputCounts(mesh.vertexCount, mesh.triangleCount);
        
    if (gtid < mesh.triangleCount)
    {
        uint packed = primitiveBuffer[mesh.triangleBufferIndex][mesh.triangleBufferOffset + gtid];
         
        uint3 unpacked = unpackUint(packed);
        
        triangles[gtid] = unpacked;
        
        uint idx1 = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + unpacked.x] + mesh.vertexBufferOffset;
        uint idx2 = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + unpacked.y] + mesh.vertexBufferOffset;
        uint idx3 = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + unpacked.z] + mesh.vertexBufferOffset;
        
        primitives[gtid].cullPrimitive = isBackface(
                drawData,
                instanceAttr.modelTransform,
                vertexBuffer[mesh.vertexBufferIndex][idx1].position,
                vertexBuffer[mesh.vertexBufferIndex][idx2].position,
                vertexBuffer[mesh.vertexBufferIndex][idx3].position
            );
    }

    if (gtid < mesh.vertexCount)
    {
        uint vertexIndex = vertexIndexBuffer[mesh.indexBufferIndex][mesh.indexBufferOffset + gtid] + mesh.vertexBufferOffset;
        
        vertex v = vertexBuffer[mesh.vertexBufferIndex][vertexIndex];
        float4 worldPos = float4(transformPoint(instanceAttr.modelTransform, v.position), 1.0f);
        
        vertices[gtid].position = mul(drawData.useDebugCamera ? drawData.debugViewProjection : drawData.viewProjection, worldPos);
        
        float3x3 TBN = calculateTBN(instanceAttr.modelTransform.rotation, v);
        
        vertices[gtid].uv = vertexBuffer[mesh.vertexBufferIndex][vertexIndex].textureCoords;
        vertices[gtid].tangentCameraPos = mul(drawData.cameraPos, TBN);
        vertices[gtid].tangentWorldPos = mul(worldPos.xyz, TBN);
        vertices[gtid].tangentCameraFront = normalize(mul(drawData.cameraFront, TBN));
        vertices[gtid].albedoIndex = instanceAttr.globalMaterialOffset + v.localMaterialOffset * 3;
        vertices[gtid].normalIndex = instanceAttr.globalMaterialOffset + v.localMaterialOffset * 3 + 1;
        vertices[gtid].metallicRoughnessIndex = instanceAttr.globalMaterialOffset + v.localMaterialOffset * 3 + 2;
    }
}

// MESH SHADER END.

// PIXEL SHADER START.

// All calculations are made in tangent space.
float4 psmain(meshOutput input) : SV_TARGET
{
    float4 metalicRoughnes = materials[input.metallicRoughnessIndex].Sample(materialsSampler[input.metallicRoughnessIndex], input.uv);

    float4 albedo = materials[input.albedoIndex].Sample(materialsSampler[input.albedoIndex], input.uv);
    float4 normalTexture = materials[input.normalIndex].Sample(materialsSampler[input.normalIndex], input.uv);
    float3 normal = normalTexture.rgb * 2.0f - 1.0f;
    float metalic = metalicRoughnes.b;
    float roughnes = metalicRoughnes.g;
    
    // Discard non solid geometry, value is just a guess works for my cases for now.
    if (albedo.a < 0.5f)
        discard;
   
    normal = normalize(normal);
    
    albedo = float4(toRGB(albedo.rgb), albedo.a);
    
    // render equation.
    float3 V = normalize(input.tangentCameraPos - input.tangentWorldPos);
    float3 l0 = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 1; ++i)
    {
        float3 lightPos = input.tangentCameraPos + input.tangentCameraFront / 4.0f;
        float3 lightColor = float3(3.0f, 3.0f, 3.0f);

        float3 L = normalize(lightPos - input.tangentWorldPos);
        float3 H = normalize(L + V);

        // radiance per per light source.
        float3 radiance = lightRadiance(lightColor, length(lightPos - input.tangentWorldPos));

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
