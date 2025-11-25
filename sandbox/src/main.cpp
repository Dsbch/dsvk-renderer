#include <application/application.h>

// GLOBAL TODO:
// Right now I need to:
// 1. Setup per meshlet buffer that will have indecies in perInstance buffer.
//    That new buffer will be used by task shader. (Each task shader should be run as (1, 1, 1) in local thread group).
//    And each task shader should be run only for one meshlet, then it selects index from per meshlet buffer and goes into instance buffer.
//    Also perMeshletBuffer should be updated each frame (basicly reupload of uin32_t) and inside shader it will be just one buffer without descriptor indexing.
// 2. Setup task shader.
// 3. Debug and suffer.

int main(int argc, char* argv[])
{
	try
	{
		engine::application app;
		engine::error err = app.checkError();
		if (err)
		{
			LOGERROR(err.err());
			return 0;
		}

		err = app.run();
		if (err)
		{
			LOGERROR(err.err());
			return 0;
		}
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
