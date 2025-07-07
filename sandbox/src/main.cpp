#include <platform/renderer/vulkan/vulkanTests.h>

int main(int argc, char* argv[])
{
	try
	{
		auto ctx = std::make_shared<engine::context>(engine::cfg<engine::main>{});
		std::unique_ptr<engine::window> window = nullptr;

		auto testApp = vktest::vulkanTest(ctx);
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
