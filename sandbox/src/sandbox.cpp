#include "sandbox.h"

#include <random>
#include <core/scene/scene.h>

static glm::mat4 getRandomTransform()
{
	static std::mt19937 rng(std::random_device{}());
	static std::uniform_real_distribution<float> distPos(-10.0f, 10.0f);
	static std::uniform_real_distribution<float> distRot(0.0f, 360.0f);

	glm::vec3 position(distPos(rng), distPos(rng), distPos(rng));
	glm::vec3 rotation(glm::radians(distRot(rng)), glm::radians(distRot(rng)), glm::radians(distRot(rng)));

	glm::mat4 mat = glm::mat4(1.0f);
	mat = glm::translate(mat, position);
	mat = glm::rotate(mat, rotation.x, glm::vec3(1, 0, 0));
	mat = glm::rotate(mat, rotation.y, glm::vec3(0, 1, 0));
	mat = glm::rotate(mat, rotation.z, glm::vec3(0, 0, 1));

	return mat;
}

namespace sandbox
{
	sandboxSystem::sandboxSystem(std::shared_ptr<engine::context> ctx)
		: engine::system(ctx)
	{}

	engine::error sandboxSystem::checkError()
	{
		return {};
	}

	void sandboxSystem::onUpdate(entt::registry& registry)
	{}

	void sandboxSystem::onRender(entt::registry& registry)
	{}

	void sandboxSystem::spawnDefaultCamera(entt::registry& registry)
	{
		auto c = registry.create();

		registry.emplace<engine::fpsCameraComponent>(
			c,
			std::make_unique<engine::fpsCamera>(
				mCtx,
				mCtx->config.inner.camera.fov,
				mCtx->config.inner.camera.nearPlane,
				mCtx->config.inner.camera.farPlane,
				mCtx->config.inner.wnd.width,
				mCtx->config.inner.wnd.height
			),
			true
		);

		registry.emplace<engine::inputListenerComponent>(
			c,
			std::vector<engine::key>{},
			std::vector<engine::key>{ engine::key::w, engine::key::a, engine::key::s, engine::key::d },
			true
		);
	}

	void sandboxSystem::spawnCube(entt::registry& registry, const engine::materialComponent& material, glm::mat4 transform, uint32_t meshUID)
	{
		auto c = registry.create();
		registry.emplace<engine::uidComponent>(c);

		std::vector<engine::vertex> vertices = {
			// +Z face (front)
			{.position = { -0.5f, -0.5f,  0.5f }, .textureCoords = { 0.0f, 0.0f }, .normal = { 0.0f, 0.0f, 1.0f } , .tangent = { 1.0f, 0.0f, 0.0f }},
			{.position = {  0.5f, -0.5f,  0.5f }, .textureCoords = { 1.0f, 0.0f }, .normal = { 0.0f, 0.0f, 1.0f } , .tangent = { 1.0f, 0.0f, 0.0f }},
			{.position = {  0.5f,  0.5f,  0.5f }, .textureCoords = { 1.0f, 1.0f }, .normal = { 0.0f, 0.0f, 1.0f } , .tangent = { 1.0f, 0.0f, 0.0f }},
			{.position = { -0.5f,  0.5f,  0.5f }, .textureCoords = { 0.0f, 1.0f }, .normal = { 0.0f, 0.0f, 1.0f } , .tangent = { 1.0f, 0.0f, 0.0f }},

			// -Z face (back)
			{.position = {  0.5f, -0.5f, -0.5f }, .textureCoords = { 0.0f, 0.0f },  .normal = { 0.0f, 0.0f, -1.0f } , .tangent = { -1.0f, 0.0f, 0.0f }},
			{.position = { -0.5f, -0.5f, -0.5f }, .textureCoords = { 1.0f, 0.0f },  .normal = { 0.0f, 0.0f, -1.0f } , .tangent = { -1.0f, 0.0f, 0.0f }},
			{.position = { -0.5f,  0.5f, -0.5f }, .textureCoords = { 1.0f, 1.0f },  .normal = { 0.0f, 0.0f, -1.0f } , .tangent = { -1.0f, 0.0f, 0.0f }},
			{.position = {  0.5f,  0.5f, -0.5f }, .textureCoords = { 0.0f, 1.0f },  .normal = { 0.0f, 0.0f, -1.0f } , .tangent = { -1.0f, 0.0f, 0.0f }},

			// -X face (left)
			{.position = { -0.5f, -0.5f, -0.5f }, .textureCoords = { 0.0f, 0.0f }, .normal = { -1.0f, 0.0f, 0.0f } , .tangent = { 0.0f, 0.0f, 1.0f }},
			{.position = { -0.5f, -0.5f,  0.5f }, .textureCoords = { 1.0f, 0.0f }, .normal = { -1.0f, 0.0f, 0.0f } , .tangent = { 0.0f, 0.0f, 1.0f }},
			{.position = { -0.5f,  0.5f,  0.5f }, .textureCoords = { 1.0f, 1.0f }, .normal = { -1.0f, 0.0f, 0.0f } , .tangent = { 0.0f, 0.0f, 1.0f }},
			{.position = { -0.5f,  0.5f, -0.5f }, .textureCoords = { 0.0f, 1.0f }, .normal = { -1.0f, 0.0f, 0.0f } , .tangent = { 0.0f, 0.0f, 1.0f }},

			// +X face (right)
			{.position = { 0.5f, -0.5f,  0.5f }, .textureCoords = { 0.0f, 0.0f },  .normal = { 1.0f, 0.0f, 0.0f } , .tangent = { 0.0f, 0.0f, -1.0f }},
			{.position = { 0.5f, -0.5f, -0.5f }, .textureCoords = { 1.0f, 0.0f },  .normal = { 1.0f, 0.0f, 0.0f } , .tangent = { 0.0f, 0.0f, -1.0f }},
			{.position = { 0.5f,  0.5f, -0.5f }, .textureCoords = { 1.0f, 1.0f },  .normal = { 1.0f, 0.0f, 0.0f } , .tangent = { 0.0f, 0.0f, -1.0f }},
			{.position = { 0.5f,  0.5f,  0.5f }, .textureCoords = { 0.0f, 1.0f },  .normal = { 1.0f, 0.0f, 0.0f } , .tangent = { 0.0f, 0.0f, -1.0f }},

			// +Y face (top)
			{.position = { -0.5f, 0.5f,  0.5f }, .textureCoords = { 0.0f, 0.0f }, .normal = { 0.0f, 1.0f, 0.0f } , .tangent = { 1.0f, 0.0f, 0.0f }},
			{.position = {  0.5f, 0.5f,  0.5f }, .textureCoords = { 1.0f, 0.0f }, .normal = { 0.0f, 1.0f, 0.0f } , .tangent = { 1.0f, 0.0f, 0.0f }},
			{.position = {  0.5f, 0.5f, -0.5f }, .textureCoords = { 1.0f, 1.0f }, .normal = { 0.0f, 1.0f, 0.0f } , .tangent = { 1.0f, 0.0f, 0.0f }},
			{.position = { -0.5f, 0.5f, -0.5f }, .textureCoords = { 0.0f, 1.0f }, .normal = { 0.0f, 1.0f, 0.0f } , .tangent = { 1.0f, 0.0f, 0.0f }},

			// -Y face (bottom)
			{.position = { -0.5f, -0.5f, -0.5f }, .textureCoords = { 0.0f, 0.0f }, .normal = { 0.0f, -1.0f, 0.0f } , .tangent = { 1.0f, 0.0f, 0.0f }},
			{.position = {  0.5f, -0.5f, -0.5f }, .textureCoords = { 1.0f, 0.0f }, .normal = { 0.0f, -1.0f, 0.0f } , .tangent = { 1.0f, 0.0f, 0.0f }},
			{.position = {  0.5f, -0.5f,  0.5f }, .textureCoords = { 1.0f, 1.0f }, .normal = { 0.0f, -1.0f, 0.0f } , .tangent = { 1.0f, 0.0f, 0.0f }},
			{.position = { -0.5f, -0.5f,  0.5f }, .textureCoords = { 0.0f, 1.0f }, .normal = { 0.0f, -1.0f, 0.0f } , .tangent = { 1.0f, 0.0f, 0.0f }},
		};

		std::vector<uint32_t> indices;
		for (int face = 0; face < 6; ++face) {
			uint32_t offset = face * 4;
			indices.push_back(offset + 0);
			indices.push_back(offset + 1);
			indices.push_back(offset + 2);
			indices.push_back(offset + 2);
			indices.push_back(offset + 3);
			indices.push_back(offset + 0);
		}

		registry.emplace<engine::meshComponent>(
			c,
			std::make_shared<std::vector<engine::vertex>>(std::move(vertices)),
			std::make_shared<std::vector<uint32_t>>(std::move(indices)),
			meshUID
		);

		registry.emplace<engine::materialComponent>(c, material);
		registry.emplace<engine::transformComponent>(c, transform);
	}

	void sandboxSystem::spawnSphere(entt::registry& registry, const engine::materialComponent& material, glm::mat4 transform, uint32_t meshUID)
	{
		const uint32_t X_SEGMENTS = 256;
		const uint32_t Y_SEGMENTS = 128;
		std::vector<engine::vertex> vertices;
		std::vector<uint32_t> indices;

		for (uint32_t y = 0; y <= Y_SEGMENTS; ++y) {
			for (uint32_t x = 0; x <= X_SEGMENTS; ++x) {
				float xSegment = (float)x / X_SEGMENTS;
				float ySegment = (float)y / Y_SEGMENTS;

				float xPos = std::cos(xSegment * glm::two_pi<float>()) * std::sin(ySegment * glm::pi<float>());
				float yPos = std::cos(ySegment * glm::pi<float>());
				float zPos = std::sin(xSegment * glm::two_pi<float>()) * std::sin(ySegment * glm::pi<float>());

				glm::vec3 position = glm::vec3(xPos, yPos, zPos) * 0.5f;
				glm::vec2 texCoord = glm::vec2(xSegment, ySegment);
				glm::vec3 normal = glm::normalize(position);

				glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
				if (std::abs(normal.y) > 0.99f)
					up = glm::vec3(1.0f, 0.0f, 0.0f);

				glm::vec3 tangent = glm::normalize(glm::cross(up, normal));

				vertices.push_back(engine::vertex{
					.position = position,
					.textureCoords = texCoord,
					.normal = normal,
					.tangent = tangent
					});
			}
		}

		for (uint32_t y = 0; y < Y_SEGMENTS; ++y) {
			for (uint32_t x = 0; x < X_SEGMENTS; ++x) {
				uint32_t i0 = y * (X_SEGMENTS + 1) + x;
				uint32_t i1 = (y + 1) * (X_SEGMENTS + 1) + x;
				uint32_t i2 = (y + 1) * (X_SEGMENTS + 1) + x + 1;
				uint32_t i3 = y * (X_SEGMENTS + 1) + x + 1;

				indices.push_back(i0);
				indices.push_back(i1);
				indices.push_back(i2);

				indices.push_back(i0);
				indices.push_back(i2);
				indices.push_back(i3);
			}
		}

		auto c = registry.create();
		registry.emplace<engine::uidComponent>(c);
		registry.emplace<engine::meshComponent>(
			c,
			std::make_shared<std::vector<engine::vertex>>(std::move(vertices)),
			std::make_shared<std::vector<uint32_t>>(std::move(indices)),
			meshUID
		);
		registry.emplace<engine::materialComponent>(c, material);
		registry.emplace<engine::transformComponent>(c, transform);
	}

	void sandboxSystem::updateTransform(entt::registry& registry, glm::mat4 translate)
	{
		entt::entity toUpdate;

		for (auto [entity, uid, mesh, transform] : registry.view<engine::uidComponent, engine::meshComponent, engine::transformComponent>().each())
		{
			transform.transform *= translate;
			toUpdate = entity;
			break;
		}

		registry.emplace_or_replace<engine::applyTransformComponent>(toUpdate);
	}

	void sandboxSystem::onEvent(entt::registry& registry, std::shared_ptr<engine::baseEvent> e)
	{
		// camera stuff.
		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::b)
		{
			auto cameraView = registry.view<engine::fpsCameraComponent>();

			for (auto [entity, camera] : cameraView.each())
			{
				if (camera.isActive)
					return;
			}

			spawnDefaultCamera(registry);
		}

		// spawn sphere.
		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::e)
		{
			static auto uid = engine::genUID();

			auto textureAlbedo = mCtx->mAmanager->loadTexture("../assets/textures/ribbed/rusty-ribbed-metal_albedo.png");
			auto textureAO = mCtx->mAmanager->loadTexture("../assets/textures/ribbed/rusty-ribbed-metal_ao.png");
			auto textureMetallic = mCtx->mAmanager->loadTexture("../assets/textures/ribbed/rusty-ribbed-metal_metallic.png");
			auto textureNormal = mCtx->mAmanager->loadTexture("../assets/textures/ribbed/rusty-ribbed-metal_normal-ogl.png");
			auto textureRoughness = mCtx->mAmanager->loadTexture("../assets/textures/ribbed/rusty-ribbed-metal_roughness.png");
			auto shader = mCtx->mAmanager->loadShader("../assets/shaders/vertexInstanced.glsl", "../assets/shaders/fragmentInstanced.glsl");

			std::function<int()> albedo = [tex = textureAlbedo.first]() -> int {
				tex->bind();
				return int(tex->getSlotID());
				};

			std::function<int()> normal = [tex = textureNormal.first]() -> int {
				tex->bind();
				return int(tex->getSlotID());
				};

			std::function<int()> metallic = [tex = textureMetallic.first]() -> int {
				tex->bind();
				return int(tex->getSlotID());
				};

			std::function<int()> roughness = [tex = textureRoughness.first]() -> int {
				tex->bind();
				return int(tex->getSlotID());
				};

			std::function<int()> ao = [tex = textureAO.first]() -> int {
				tex->bind();
				return int(tex->getSlotID());
				};

			spawnSphere(registry, engine::materialComponent{
				textureAlbedo.first,
				textureRoughness.first,
				textureNormal.first,
				textureMetallic.first,
				textureAO.first,
				shader.first,
				{
					{ "uAO",		{ao, 1}			},
					{ "uAlbedo",	{albedo, 1}		},
					{ "uNormal",	{normal, 1}		},
					{ "uMetalic",	{metallic, 1}	},
					{ "uRoughness", {roughness, 1}	},
				}
				}, getRandomTransform(), uid);
		}

		// spawn rusted sphere.
		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::p)
		{
			static auto uid = engine::genUID();

			auto textureAlbedo = mCtx->mAmanager->loadTexture("../assets/textures/rusted-sphere/rustediron2_basecolor.png");
			auto textureAO = mCtx->mAmanager->loadTexture("../assets/textures/rusted-sphere/rustediron2_ao.png");
			auto textureMetallic = mCtx->mAmanager->loadTexture("../assets/textures/rusted-sphere/rustediron2_metallic.png");
			auto textureNormal = mCtx->mAmanager->loadTexture("../assets/textures/rusted-sphere/rustediron2_normal.png");
			auto textureRoughness = mCtx->mAmanager->loadTexture("../assets/textures/rusted-sphere/rustediron2_roughness.png");
			auto shader = mCtx->mAmanager->loadShader("../assets/shaders/vertexInstanced.glsl", "../assets/shaders/fragmentInstanced.glsl");

			std::function<int()> albedo = [tex = textureAlbedo.first]() -> int {
				tex->bind();
				return int(tex->getSlotID());
				};

			std::function<int()> normal = [tex = textureNormal.first]() -> int {
				tex->bind();
				return int(tex->getSlotID());
				};

			std::function<int()> metallic = [tex = textureMetallic.first]() -> int {
				tex->bind();
				return int(tex->getSlotID());
				};

			std::function<int()> roughness = [tex = textureRoughness.first]() -> int {
				tex->bind();
				return int(tex->getSlotID());
				};

			std::function<int()> ao = [tex = textureAO.first]() -> int {
				tex->bind();
				return int(tex->getSlotID());
				};

			spawnSphere(registry, engine::materialComponent{
				textureAlbedo.first,
				textureRoughness.first,
				textureNormal.first,
				textureMetallic.first,
				textureAO.first,
				shader.first,
				{
					{ "uAO",		{ao, 1}			},
					{ "uAlbedo",	{albedo, 1}		},
					{ "uNormal",	{normal, 1}		},
					{ "uMetalic",	{metallic, 1}	},
					{ "uRoughness", {roughness, 1}	},
				}
				}, getRandomTransform(), uid);
		}

		// spawn obisida cube.
		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::r)
		{
			static auto uid = engine::genUID();

			auto texture = mCtx->mAmanager->loadTexture("../assets/textures/obsidian.jpg");
			auto shader = mCtx->mAmanager->loadShader("../assets/shaders/vertexInstanced.glsl", "../assets/shaders/fragmentInstanced.glsl");

			texture.first->bind();
			auto slotID = texture.first->getSlotID();

			spawnCube(registry, engine::materialComponent{
				texture.first,
				texture.first,
				texture.first,
				texture.first,
				texture.first,
				shader.first,
				{
					{"uAlbedo", {int(slotID), 1}}
				}
				}, getRandomTransform(), uid);
		}

		// spawn pirate cube.
		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::t)
		{
			static auto uid = engine::genUID();

			auto textureAlbedo = mCtx->mAmanager->loadTexture("../assets/textures/pirate-gold/pirate-gold_albedo.png");
			auto textureAO = mCtx->mAmanager->loadTexture("../assets/textures/pirate-gold/pirate-gold_ao.png");
			auto textureMetallic = mCtx->mAmanager->loadTexture("../assets/textures/pirate-gold/pirate-gold_metallic.png");
			auto textureNormal = mCtx->mAmanager->loadTexture("../assets/textures/pirate-gold/pirate-gold_normal-ogl.png");
			auto textureRoughness = mCtx->mAmanager->loadTexture("../assets/textures/pirate-gold/pirate-gold_roughness.png");
			auto shader = mCtx->mAmanager->loadShader("../assets/shaders/vertexInstanced.glsl", "../assets/shaders/fragmentInstanced.glsl");

			std::function<int()> albedo = [tex = textureAlbedo.first]() -> int {
				tex->bind();
				return int(tex->getSlotID());
				};

			std::function<int()> normal = [tex = textureNormal.first]() -> int {
				tex->bind();
				return int(tex->getSlotID());
				};

			std::function<int()> metallic = [tex = textureMetallic.first]() -> int {
				tex->bind();
				return int(tex->getSlotID());
				};

			std::function<int()> roughness = [tex = textureRoughness.first]() -> int {
				tex->bind();
				return int(tex->getSlotID());
				};

			std::function<int()> ao = [tex = textureAO.first]() -> int {
				tex->bind();
				return int(tex->getSlotID());
				};


			spawnCube(registry, engine::materialComponent{
				textureAlbedo.first,
				textureRoughness.first,
				textureNormal.first,
				textureMetallic.first,
				textureMetallic.first,
				shader.first,
				{
					{ "uAO",		{ao, 1}			},
					{ "uAlbedo",	{albedo, 1}		},
					{ "uNormal",	{normal, 1}		},
					{ "uMetalic",	{metallic, 1}	},
					{ "uRoughness", {roughness, 1}	},
				}
				},
				getRandomTransform(), uid);
		}

		// rmv instance mesh.
		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::q)
		{
			entt::entity toDelete;
			bool use = false;
			auto view = registry.view<engine::uidComponent, engine::meshComponent, engine::materialComponent>();
			int i = 0;
			for (auto [entity, uid, mesh, mat] : view.each())
			{
				if (i == 1)
				{
					use = true;
					toDelete = entity;
				}
				i++;
			}

			if (use)
				registry.emplace<engine::deleteComponent>(toDelete);
		}

		// move random entity.
		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::i)
		{
			updateTransform(registry, glm::translate(glm::mat4(1.0f), glm::vec3(0.1f, 0.0f, 0.0f)));
		}

		// rotate random entity.
		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::k)
		{
			updateTransform(registry, glm::rotate(glm::mat4(1.0f), glm::radians(5.0f), glm::vec3(1.0f)));
		}

		// spawn skybox.
		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::x)
		{
			static auto uid = engine::genUID();

			auto skybox = mCtx->mAmanager->loadCubeMap(
				{
					"../assets/skybox/right.jpg",
					"../assets/skybox/left.jpg",
					"../assets/skybox/top.jpg",
					"../assets/skybox/bottom.jpg",
					"../assets/skybox/front.jpg",
					"../assets/skybox/back.jpg"
				}
			);
			auto shader = mCtx->mAmanager->loadShader("../assets/shaders/vertexSkybox.glsl", "../assets/shaders/fragmentSkybox.glsl");

			auto c = registry.create();
			registry.emplace<engine::uidComponent>(c);
			registry.emplace<engine::skyboxComponent>(c, true, shader.first, skybox.first);
		}
	}

	sandboxLayer::sandboxLayer(std::shared_ptr<engine::context> ctx)
		: engine::layer(ctx)
	{
		engine::scene::addUserSystem(std::make_unique<sandboxSystem>(ctx));
	}

	bool sandboxLayer::onEvent(std::shared_ptr<engine::baseEvent> e)
	{
		return false;
	}

	void sandboxLayer::onRender()
	{}

	void sandboxLayer::onUpdate()
	{}

	engine::error sandboxLayer::checkError() const {
		return {};
	}

	sandbox::sandbox() : engine::application()
	{
		if (mErr) return;
		pushOverlay(std::make_unique<sandboxLayer>(mCtx));
	}
}
