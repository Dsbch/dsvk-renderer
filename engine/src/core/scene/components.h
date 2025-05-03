#pragma once

#include <pch.h>
#include <random>
#include "platform/renderer/vertex.h"
#include "platform/renderer/opengl/shader.h"
#include "platform/renderer/opengl/texture.h"

namespace engine
{
	inline uint32_t genUID()
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());
		static std::uniform_int_distribution<uint32_t> distrib(0, UINT32_MAX);

		return distrib(gen);
	}

	struct uidComponent
	{
		uint32_t uid;

		uidComponent()
			: uid(genUID()) {}
	};

	struct tagComponent
	{
		std::string tag;

		tagComponent(const std::string& tag)
			: tag(tag) {}
	};

	struct dynamicMeshComponent
	{
		std::vector<vertex> meshData;
		std::vector<uint32_t> indexData;
		bool needUpdate;

		dynamicMeshComponent(std::vector<vertex>&& meshData, std::vector<uint32_t>&& indexData)
			: meshData(std::move(meshData)), indexData(std::move(indexData)), needUpdate(false) {}
	};


	struct staticMeshComponent
	{
		std::vector<vertex> meshData;
		std::vector<uint32_t> indexData;

		staticMeshComponent(std::vector<vertex>&& meshData, std::vector<uint32_t>&& indexData)
			: meshData(std::move(meshData)), indexData(std::move(indexData)) {}
	};

	struct materialComponent
	{
		std::unique_ptr<shaderProgram> shader;
		std::unique_ptr<texture> texture;

		materialComponent(std::unique_ptr<shaderProgram>&& shader, std::unique_ptr<shaderProgram>&& texure)
			: shader(std::move(shader)), texture(std::move(texture)) {}
	};
}