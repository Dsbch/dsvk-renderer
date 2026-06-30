//  dxc -T cs_6_9 -E main -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkComputeCulling.spv vkComputeCulling.hlsl
#ifdef __spirv__
#define DEFINE_AS_PUSH_CONSTANT [[vk::push_constant]]
#else
#define DEFINE_AS_PUSH_CONSTANT
#endif

#define THREADS_COUNT 32

struct pushConstant
{
    uint hzbMipLevel;
    uint width;
    uint height;
};

DEFINE_AS_PUSH_CONSTANT
pushConstant push;

struct command
{
    uint instanceIndex;
    uint instanceOffset;
    
    uint meshletIndex;
    uint meshletOffset1;
    uint meshletOffset2;
    uint meshletOffset3;
    uint meshletOffset4;
    
    uint visabilityBit;
};

bool hasFlag(uint mask, uint flag)
{
    return (mask & flag) == flag;
}

Texture2D<float> originalZbuffer : register(t0, space0);
Texture2D<float> hzbChain[] : register(u1, space0);
StructuredBuffer<command> commandOpaqueBuffer[] : register(t2, space0);
StructuredBuffer<command> commandAccumilationBuffer : register(t3, space0);

[numthreads(THREADS_COUNT, 1, 1)]
void main(uint2 dtid : SV_DispatchThreadID)
{

}