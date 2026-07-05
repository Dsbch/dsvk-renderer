# 3D engine project.

    DONE:
        1. Full mesh shader geometry pass with instancing, and simple culling.
            Added culling types:
                1.1. Backface culling first in task shader next in mesh shader.
                1.2. Frustum culling in task shader.
        2. Integraion with ECS.
        3. Added material proccessing for PBR metallic workflow.
        4. Added OIT algoritm.
        5. Optimized OIT.
        6. Added concurrency library to the project libcoost.
        7. Added simple GPU profiling window.
        8. Added culling for animated meshlets.

    IN DEVELOPMENT:
        1. Frustum culling and lod level selection should be done first in compute shader where I generate CMD buffer opaque and accumilation pass, right now amplification rate is too low, I get now performance boost from frustum culling.
            1.1. Have to use prefix-sum on GPU algoritm after compute culling to optimize amplification rate. - DONE
        2. Two phase HZB occlision culling for opaque pass and accunilation pass. - DONE
        3. Prefix sum algoritm for higher amplification rate. - IN DEVELOPMENT.
        4. Figure out how to solve a problem with debug camera ovewriting depth buffer :(. - DONE.

    BUGS:
        1. Problem with flickering on new instance - FIXED, the problem was mapped command buffer. Now I need to move command buffer generation to GPU compute shader.
        2. Low performance in accumilaton pass.
		3. Semaphore is not deleting in submit.cpp. I can't delete it because it's still used in second submit as wait sema! When separating all data to per frame data. I need to delete them when I wait on renderFence for each frame.

    TODO:
		1. For each frame in flight I need to make separate animation, perInstance, draw buffers.
			1.1. Updates should be scheduled separetly. How do update them (with staging buffer or use mapped memmory???).
        1. Add jolt CPU side physics.
        2. Figure out how to do shadows, my goal is good hard and soft shadows.
        3. Global illumination and reflections with radiance cascades.
        4. Add postproccessing like bloom, focus etc.
        5. Get your bsdf and brdf together. Should use disney.

        Optional:
            1. Own file format. Ser/Dser of whole ECS.
            2. Own save files.
            3. Add guismos.

I'm currently working on my vulkan renderer.

Download lunargSDK for vulkan first https://vulkan.lunarg.com/sdk/home

premake5 --mode=sandbox vs2026

If you build in release you can load default model with "R", "Q" to delete instance "T" to rotate. 
Press "B" to enable debug camera to test culling.

Renderer isn't yet finished.