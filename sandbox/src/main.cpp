#include <platform/renderer/vulkan/vulkanTests.h>


// GLOBAL TODO:
// Right now I need to:
// 1. Setup per meshlet buffer that will have indecies in perInstance buffer.
//    That new buffer will be used by task shader. (Each task shader should be run as (1, 1, 1) in local thread group).
//    And each task shader should be run only for one meshlet, then it selects index from per meshlet buffer and goes into instance buffer.
//    Also perMeshletBuffer should be updated each frame (basicly reupload of uin32_t) and inside shader it will be just one buffer without descriptor indexing.
// 2. Setup task shader.
// 3. Debug and suffer.
//

int main(int argc, char* argv[])
{
	try
	{
		auto ctx = std::make_shared<engine::context>(engine::cfg<engine::main>{});

		ctx->config.inner.wnd.width = 300;
		ctx->config.inner.wnd.height = 400;

		auto testApp = engine::vulkanTest(ctx);
		if (auto err = testApp.checkError(); err)
		{
			LOGERROR(err.err());
			return 0;
		}

		testApp.run();
	}
	catch (const std::exception& exc)
	{
		LOGERROR("exception was caught in run std::exception: {}", exc.what());
	}
	catch (...)
	{
		LOGERROR("exception was caught in run");
	}

	return 0;
}
