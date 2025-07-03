#version 440 core
out vec4 fragColor;
uniform sampler2D uAlbedo;
uniform sampler2D uNormal;
uniform sampler2D uMetalic;
uniform sampler2D uRoughness;
uniform sampler2D uAO;

in vsOUT {
    vec2 texCoords;
    vec3 tangentCameraPos;
    vec3 tangentFragmentPos;
} fsIN;

const float PI = 3.14159265359;

// it's just approxiamtion the formula itself quite complex and using radiant flux that we do not have.
vec3 lightRadiance(vec3 lightColor, float distance)
{
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = lightColor * attenuation;

    return radiance;
}

// baseReflectivity F0 really hard to calculate, so we use trick like that.
// for dielectrics we return approximation 0.04, for mettalic we return mix.
vec3 baseReflectivity(vec3 albedo, float metalic)
{
    vec3 f0 = vec3(0.04); // base reflectivity of dielectrics.
    f0 = mix(f0, albedo, metalic);

    return f0;
}

// The Fresnel equation describes the ratio of surface reflection at different surface angles.
vec3 fresnelSchlick(float cosTheta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// distributionGGX - approximates the amount the surface's microfacets are aligned to the halfway vector, influenced by the roughness of the surface; this is the primary function approximating the microfacets.
float distributionGGX(vec3 n, vec3 h, float roughness)
{
    // not sure why we use roughness^4.
    float a      = roughness*roughness;
    float a2     = a*a;
    float nDotH  = max(dot(n, h), 0.0);
    float nDotH2 = nDotH*nDotH;
    
    float num   = a2;
    float denom = (nDotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / denom;
}

// geometrySchlickGGX - describes the self-shadowing property of the microfacets. When a surface is relatively rough, the surface's microfacets can overshadow other microfacets reducing the light the surface reflects.
float geometrySchlickGGX(float nDotV, float roughness)
{
    // remap roughness.
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = nDotV;
    float denom = nDotV * (1.0 - k) + k;
    
    return num / denom;
}

// geometrySmith - is used for approximation of geometrySchlickGGX.
float geometrySmith(vec3 n, vec3 v, vec3 l, float roughness)
{
    float nDotV = max(dot(n, v), 0.0);
    float nDotL = max(dot(n, l), 0.0);
    float ggx2  = geometrySchlickGGX(nDotV, roughness);
    float ggx1  = geometrySchlickGGX(nDotL, roughness);
    
    return ggx1 * ggx2;
}

// here I decided to hardCode lightPositions and lightColors for now.
// keep in mind that they should be in tangent space! Right now space doesn't matter because it point lights.
// TODO: add them as uniforms and figure out how to do it dynamicly.
const vec3 lightPositions[3] = vec3[](
    vec3(0.0,  0.0, 0.0),
    vec3(5.0, 0.0, 0.0),
    vec3(-5.0, 0.0, 0.0)
);

const vec3 lightColors[3] = vec3[](
    vec3(180.0, 152.0, 150.0),
    vec3(25.0, 152.0, 25.0),
    vec3(180.0, 25.0, 25.0)
);

// all calculations in fragment shader should be in tangent space (because of normals).
void main()
{
    // gamma correction.
    vec3 albedo     = pow(texture(uAlbedo, fsIN.texCoords).rgb, vec3(2.2));
    float metallic  = texture(uMetalic, fsIN.texCoords).r;
    float roughness = texture(uRoughness, fsIN.texCoords).r;
    float ao        = texture(uAO, fsIN.texCoords).r;
    // this normal is in tangent space, so we use things in tangentSpace from fsIN.
    vec3 normal = normalize(texture(uNormal, fsIN.texCoords).rgb * 2.0 - 1.0);
    vec3 fromFragmentToCamera = normalize(fsIN.tangentCameraPos - fsIN.tangentFragmentPos);

    // render equation.
    vec3 l0 = vec3(0.0);
    for(int i = 0; i < 3; ++i) 
    {
        vec3 lightPos = lightPositions[i];
        vec3 lightColor = lightColors[i];

        vec3 fromFragmentToLight = normalize(lightPos - fsIN.tangentFragmentPos);
        vec3 halfway = normalize(fromFragmentToLight + fromFragmentToCamera);

        // radiance per per light source.
        vec3 radiance = lightRadiance(lightColor, length(lightPos - fsIN.tangentFragmentPos));

        // Cook-Torrance BRDF
        float d = distributionGGX(normal, halfway, roughness);   
        float g   = geometrySmith(normal, fromFragmentToCamera, fromFragmentToLight, roughness);      
        vec3 f    = fresnelSchlick(max(dot(halfway, fromFragmentToCamera), 0.0), baseReflectivity(albedo, metallic));

        vec3 numerator    = d * f * g; 
        float denominator = 4.0 * max(dot(normal, fromFragmentToCamera), 0.0) * max(dot(normal, fromFragmentToLight), 0.0) + 0.0001; 
        // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;

        // kS is equal to Fresnel
        vec3 kS = f;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;

        // scale light by nDotL
        float nDotL = max(dot(normal, fromFragmentToLight), 0.0);

        // add to outgoing radiance Lo
        l0 += (kD * albedo / PI + specular) * radiance * nDotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }    


    // ambient lighting.
    // note that in future you need to replace that ambient light with some Voxel Cone Tracing for reflections.
    // for now we just use AO texture.
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    vec3 color = ambient + l0;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    fragColor = vec4(color, 1.0);
}