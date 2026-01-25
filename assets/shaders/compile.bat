@echo off
:: Enable ANSI escape sequences for colors (works in Windows 10+)
for /f "tokens=2 delims=:." %%a in ('ver') do if %%a geq 10 (
  >nul 2>&1 reg query HKCU\Console
)

chcp 65001

echo =====================================================
echo            Compiling Shaders with DXC
echo =====================================================
echo.

echo Compiling Mesh Shader...
dxc -T ms_6_9 -E msmain -spirv -fvk-use-scalar-layout -fspv-target-env=vulkan1.3 -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshMs.spv vkMesh.hlsl
if %errorlevel% neq 0 (
    echo Mesh shader compilation failed!
    pause
) else (
    echo Mesh shader compiled successfully.
)

echo.

echo Compiling Pixel Shader...
dxc -T ps_6_9 -E psmain -spirv -fvk-use-scalar-layout -fspv-target-env=vulkan1.3 -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshPs.spv vkMesh.hlsl
if %errorlevel% neq 0 (
    echo Pixel shader compilation failed!
    pause
) else (
    echo Pixel shader compiled successfully.
)

echo.

echo Compiling Task Shader...
dxc -T as_6_9 -E asmain -spirv -fvk-use-scalar-layout -fspv-target-env=vulkan1.3 -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshAs.spv vkMesh.hlsl
if %errorlevel% neq 0 (
    echo Task shader compilation failed!
    pause
) else (
    echo Task shader compiled successfully.
)

echo.

echo Compiling Vertex line Shader...
dxc -T vs_6_9 -E vsmain -spirv -fspv-target-env=vulkan1.3 -fvk-use-scalar-layout -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkLineVs.spv vkLine.hlsl
if %errorlevel% neq 0 (
    echo Vertex line Shader compilation failed!
    pause
) else (
    echo Vertex line Shader compiled successfully.
)

echo.

echo.

echo Compiling Pixel Line Shader...
dxc -T ps_6_9 -E psmain -spirv -Fo -fvk-use-scalar-layout -Fo vkCompiled/vkLinePs.spv vkLine.hlsl
if %errorlevel% neq 0 (
    echo Pixel Line Shader compilation failed!
    pause
) else (
    echo Pixel Line Shader compiled successfully.
)

echo.

pause
