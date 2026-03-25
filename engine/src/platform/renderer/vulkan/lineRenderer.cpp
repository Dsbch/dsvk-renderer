#include <pch.h>
#include "lineRenderer.h"
#include "shader.h"

namespace engine
{
	error lineRenderer::init(
		std::shared_ptr<context> ctx,
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		VmaAllocator allocator,
		submit& is,
		VkBuffer UBObuffer,
		VkFormat depthFormat,
		VkFormat drawFormat,
		graphicsPreset preset
	)
	{
		mCtx = ctx;

		mBindings = lineBindings{
			.descriptorSet = 0,
			.totalDescriptorsCount = 2,
			.vertexBinding = 0,
			.perDrawDataBinding = 1,
		};

		mNeedDescrotprUpdate = false;

		mPreset = preset;

		mDeletionQueue.init(device);

		mVertexBuffer.init(device, allocator);

		error err = mVertexBuffer.build(is, nullptr, sizeof(glm::vec3) * 2 * 5000, 0);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = vulkanBuf, .vulkanBuf = &mVertexBuffer });

		err = initDescriptors(device, physicalDevice, UBObuffer);
		if (err)
			return err;

		err = initPipeline(device, depthFormat, drawFormat);
		if (err)
			return err;

		return {};
	}

	error lineRenderer::destroy()
	{
		mDeletionQueue.flushDeletonQueue();

		return {};
	}

	error lineRenderer::addLine(glm::vec3 p1, glm::vec3 p2)
	{
		mNeedDescrotprUpdate = true;

		mVertexData.push_back(lineVertex{ p1 });
		mVertexData.push_back(lineVertex{ p2 });

		return {};
	}

	error lineRenderer::updateDescriptors(VmaAllocator allocator, submit& is)
	{
		if (mNeedDescrotprUpdate)
		{
			mVertexBuffer.markBytesAsDead(mVertexBuffer.getLoadedBytes());

			error err = mVertexBuffer.updateBuffer(is, mVertexData.data(), sizeof(lineVertex) * mVertexData.size(), 0);
			if (err.err() == "buffer overflow")
			{
				mVertexBuffer.destroy();

				err = mVertexBuffer.build(is, mVertexData.data(), sizeof(lineVertex) * mVertexData.size() * 2, sizeof(lineVertex) * mVertexData.size());
				if (err)
					return err;
			}
			if (err)
				return err;
		}

		// Set ubo buffer write right away.
		std::vector<VkDescriptorBufferInfo> bufferInfo{
			VkDescriptorBufferInfo{.buffer = mVertexBuffer.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE}
		};

		auto writeInfo = descriptorSet::getWriteInfo(mBindings.vertexBinding, bufferInfo);
		mDescriptorSet.updateWrite(writeInfo);

		mNeedDescrotprUpdate = false;

		return {};
	}

	error lineRenderer::drawLines(VkCommandBuffer cmd)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.getPipeline().first);

		// bind the descriptor set.
		auto set = mDescriptorSet.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.getPipeline().second, mBindings.descriptorSet, 1, &set, 0, nullptr);

		vkCmdDraw(cmd, uint32_t(mVertexData.size()), 1, 0, 0);

		return {};
	}

	error lineRenderer::initPipeline(VkDevice device, VkFormat depthFormat, VkFormat drawFormat)
	{
		VkShaderModule vertexShaderModule;
		auto vertexShader = mCtx->mAmanager->getDefaultLineVertexShader();
		if (!vertexShader)
			return vertexShader.err();

		VkShaderModule pixelShaderModule;
		auto pixelShader = mCtx->mAmanager->getDefaultLinePixelShader();
		if (!pixelShader)
			return pixelShader.err();

		vertexShaderModule = static_cast<vulkanShader*>(const_cast<shader*>(vertexShader.value().get()))->mShaderModule;
		pixelShaderModule = static_cast<vulkanShader*>(const_cast<shader*>(pixelShader.value().get()))->mShaderModule;

		mPipeline.init(device);
		mPipeline.setShaders(vertexShaderModule, pixelShaderModule);

		mPipeline.setInputTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
		mPipeline.setPolygonMode(VK_POLYGON_MODE_FILL);

		mPipeline.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

		mPipeline.setMultisampling(sampleCounts(mPreset.msaa));

		mPipeline.disableBlending();

		mPipeline.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

		//connect the image format we will draw into, from draw image
		mPipeline.setColorAttachmentFormats({ drawFormat });
		mPipeline.setDepthFormat(depthFormat);

		error buildErr = mPipeline.build(VK_NULL_HANDLE, { mDescriptorSet.getDescriptorSet().second });
		if (buildErr)
			return buildErr;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = graphicsPipe, .graphicsPipe = &mPipeline });

		return {};
	}

	error lineRenderer::initDescriptors(VkDevice device, VkPhysicalDevice physicalDevice, VkBuffer UBObuffer)
	{
		error err = mDescriptorSet.init(
			device,
			physicalDevice,
			poolConstraints{
				.maxBuffersDescriptors = 1,
				.maxUniformBuffersDescriptors = 1,
			}
			);
		if (err)
			return err;

		// add bindings for buffers.
		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.vertexBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSet.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mBindings.perDrawDataBinding, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
			)
		);

		err = mDescriptorSet.build(VK_SHADER_STAGE_ALL, mBindings.totalDescriptorsCount);
		if (err)
			return err;

		// Set ubo buffer write right away.
		std::vector<VkDescriptorBufferInfo> bufferInfo{
			VkDescriptorBufferInfo{.buffer = UBObuffer, .offset = 0, .range = VK_WHOLE_SIZE }
		};

		auto writeInfo = descriptorSet::getWriteInfo(mBindings.perDrawDataBinding, bufferInfo, true);
		mDescriptorSet.updateWrite(writeInfo);

		mDeletionQueue.addDestroyTask(destroyTask{ .type = descSet, .descSet = &mDescriptorSet });

		return {};
	}
}
