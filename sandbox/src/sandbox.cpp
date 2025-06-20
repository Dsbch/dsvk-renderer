#include "sandbox.h"

#include <random>
#include <core/scene/scene.h>

static glm::mat4 getRandomTransform()
{
	static std::mt19937 rng(std::random_device{}());
	static std::uniform_real_distribution<float> distPos(-10.0f, 10.0f);
	static std::uniform_real_distribution<float> distRot(0.0f, 360.0f);
	static std::uniform_real_distribution<float> distScale(0.5f, 2.0f);

	glm::vec3 position(distPos(rng), distPos(rng), distPos(rng));
	glm::vec3 rotation(glm::radians(distRot(rng)), glm::radians(distRot(rng)), glm::radians(distRot(rng)));
	glm::vec3 scale(distScale(rng), distScale(rng), distScale(rng));

	glm::mat4 mat = glm::mat4(1.0f);
	mat = glm::translate(mat, position);
	mat = glm::rotate(mat, rotation.x, glm::vec3(1, 0, 0));
	mat = glm::rotate(mat, rotation.y, glm::vec3(0, 1, 0));
	mat = glm::rotate(mat, rotation.z, glm::vec3(0, 0, 1));
	mat = glm::scale(mat, scale);

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
		registry.emplace<engine::meshComponent>(
			c,
			std::make_shared<std::vector<engine::vertex>>(std::vector<engine::vertex>{
			// Front face
				{.position = { 0.5f, 0.5f, 0.5f }, .textureCoords = { 1.0f, 1.0f } },
				{ .position = { 0.5f, -0.5f,  0.5f}, .textureCoords = {0.0f, 1.0f} },
				{ .position = {-0.5f, -0.5f,  0.5f}, .textureCoords = {0.0f, 0.0f} },
				{ .position = {-0.5f,  0.5f,  0.5f}, .textureCoords = {1.0f, 0.0f} },

					// Back face
				{ .position = { 0.5f,  0.5f, -0.5f}, .textureCoords = {1.0f, 0.0f} },
				{ .position = { 0.5f, -0.5f, -0.5f}, .textureCoords = {0.0f, 0.0f} },
				{ .position = {-0.5f, -0.5f, -0.5f}, .textureCoords = {0.0f, 1.0f} },
				{ .position = {-0.5f,  0.5f, -0.5f}, .textureCoords = {1.0f, 1.0f} },
		}),
			std::make_shared<std::vector<uint32_t>>(std::vector<uint32_t>{
			// Front face
			0, 1, 2, 2, 3, 0,
				// Left face
				3, 2, 6, 6, 7, 3,
				// Right face
				0, 1, 5, 5, 4, 0,
				// Top face
				0, 3, 7, 7, 4, 0,
				// Bottom face
				1, 2, 6, 6, 5, 1,
				// Back face
				4, 5, 6, 6, 7, 4,
		}), meshUID);

		registry.emplace<engine::materialComponent>(
			c,
			material
		);

		registry.emplace<engine::transformComponent>(
			c,
			transform
		);
	}

	void sandboxSystem::spawnSphere(entt::registry& registry, const engine::materialComponent& material, glm::mat4 transform, uint32_t meshUID)
	{
		const uint32_t X_SEGMENTS = 64;
		const uint32_t Y_SEGMENTS = 32;
		std::vector<engine::vertex> vertices;
		std::vector<uint32_t> indices;

		for (uint32_t y = 0; y <= Y_SEGMENTS; ++y) {
			for (uint32_t x = 0; x <= X_SEGMENTS; ++x) {
				float xSegment = (float)x / X_SEGMENTS;
				float ySegment = (float)y / Y_SEGMENTS;

				float xPos = std::cos(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());
				float yPos = std::cos(ySegment * glm::pi<float>());
				float zPos = std::sin(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());

				glm::vec3 position = glm::vec3(xPos, yPos, zPos) * 0.5f;
				glm::vec2 texCoord = glm::vec2(xSegment, ySegment);

				// Approximate tangent using partial derivative in U direction
				float dPhi = glm::two_pi<float>() / X_SEGMENTS;
				glm::vec3 dpdu = glm::vec3(
					-std::sin(xSegment * glm::two_pi<float>()) * std::sin(ySegment * glm::pi<float>()),
					0.0f,
					std::cos(xSegment * glm::two_pi<float>()) * std::sin(ySegment * glm::pi<float>())
				);

				glm::vec3 tangent = glm::normalize(dpdu);

				vertices.push_back(engine::vertex{
					position,
					texCoord,
					tangent
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

		registry.emplace<engine::materialComponent>(
			c,
			material
		);

		registry.emplace<engine::transformComponent>(
			c,
			transform
		);
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
		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::o)
		{
			static auto uid = engine::genUID();

			auto texture = mCtx->mAmanager->loadTexture("../assets/textures/pirate-gold/pirate-gold_albedo.png");
			auto shader = mCtx->mAmanager->loadShader("../assets/shaders/vertexInstanced.glsl", "../assets/shaders/fragmentInstanced.glsl");

			texture.first->bind();
			auto slotID = texture.first->getSlotID();

			spawnSphere(registry, engine::materialComponent{ 
				texture.first, 
				texture.first, 
				texture.first, 
				texture.first, 
				texture.first, 
				shader.first, 
				{
					{"u_albedo", {int(slotID), 1}}
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
					{"u_albedo", {int(slotID), 1}}
				}
				}, getRandomTransform(), uid);
		}

		// spawn wood cube.
		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::t)
		{
			static auto uid = engine::genUID();

			auto texture = mCtx->mAmanager->loadTexture("../assets/textures/wood.jpg");
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
					{"u_albedo", {int(slotID), 1}}
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

int main(int argc, char* argv[])
{
	try
	{
		sandbox::sandbox e;
		if (auto err = e.checkError(); err)
		{
			LOGERROR(err.err());
			return 0;
		}
		e.run();
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
