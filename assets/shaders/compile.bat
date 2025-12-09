@echo off
:: Enable ANSI escape sequences for colors (works in Windows 10+)
for /f "tokens=2 delims=:." %%a in ('ver') do if %%a geq 10 (
  >nul 2>&1 reg query HKCU\Console
)

chcp 65001

echo =====================================================
echo   🔨 Compiling Shaders with DXC
echo =====================================================
echo.

echo [1/4] 🟦 Compiling Mesh Shader...
dxc -T ms_6_9 -E msmain -spirv -fspv-target-env=vulkan1.3 -fspv-extension=SPV_EXT_mesh_shader -fspv-extension=SPV_EXT_descriptor_indexing -Fo vkCompiled/vkMeshMs.spv vkMesh.hlsl
if errorlevel 1 (
    echo ❌ Mesh shader compilation failed!
    exit /b 1
) else (
    echo ✅ Mesh shader compiled successfully.
)

echo.

echo [2/4] 🟩 Compiling Pixel Shader...
dxc -T ps_6_9 -E psmain -spirv -Fo vkCompiled/vkMeshPs.spv vkMesh.hlsl
if errorlevel 1 (
    echo ❌ Pixel shader compilation failed!
    exit /b 1
) else (
    echo ✅ Pixel shader compiled successfully.
)

echo.

echo [3/4] 🟨 Compiling Task Shader...
dxc -T as_6_9 -E asmain -spirv -Fo vkCompiled/vkMeshAs.spv vkMesh.hlsl
if errorlevel 1 (
    echo ❌ Task shader compilation failed!
    exit /b 1
) else (
    echo ✅ Task shader compiled successfully.
)

echo.

echo [4/4] 🟨 Compiling Compute Shader...
dxc -T cs_6_9 -E main -spirv -Fo vkCompiled/vkCompute.spv vkCompute.comp
if errorlevel 1 (
    echo ❌ Compute shader compilation failed!
    exit /b 1
) else (
    echo ✅ Compute shader compiled successfully.
)

echo.
echo 🎉 All shaders compiled successfully!
pause
