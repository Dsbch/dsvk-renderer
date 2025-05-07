#pragma once

#include <pch.h>
#include <random>
#include "core/camera/camera.h"
#include "core/events/events.h"
#include "platform/renderer/vertex.h"
#include "platform/renderer/opengl/shader.h"
#include "platform/renderer/opengl/texture.h"

namespace engine
{
	static uint32_t genUID()
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

	struct deleteComponent {};
	struct updateMeshComponent {};

	struct dynamicMeshComponent
	{
		std::vector<vertex> meshData;
		std::vector<uint32_t> indexData;

		dynamicMeshComponent(std::vector<vertex>&& meshData, std::vector<uint32_t>&& indexData)
			: meshData(std::move(meshData)), indexData(std::move(indexData)) {}
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
		std::shared_ptr<shaderProgram> shader;
		std::shared_ptr<texture> tex;

		bool operator<(const materialComponent& other) const
		{
			return shader->getID() < other.shader->getID() && tex->getID() < other.tex->getID();
		}

		materialComponent(std::shared_ptr<texture> tex, std::shared_ptr<shaderProgram> shader)
			: shader(shader), tex(tex) {}
	};

	struct inputListenerComponent
	{
		std::vector<key> keyUp;
		std::vector<key> keyDown;
		bool mouseMove;

		inputListenerComponent(std::vector<key>&& keyUp, std::vector<key>&& keyDown, bool mouseMove) : keyUp(std::move(keyUp)), keyDown(std::move(keyDown)), mouseMove(mouseMove) {}
	};

	struct fpsCameraComponent
	{
		std::unique_ptr<fpsCamera> camera;
		bool isActive;

		fpsCameraComponent(std::unique_ptr<fpsCamera>&& camera, bool isActive) : camera(std::move(camera)), isActive(isActive) {}
	};
}
