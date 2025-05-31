#include <application/application.h>
#include <base/logger/logger.h>
#include <core/layers/layer.h>
#include <core/scene/scene.h>
#include <core/scene/components.h>

// code below only for tests, TODO: move to scripts.
class testSystem : public engine::system {
private:
	void spawnDefaultCamera(entt::registry& registry)
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

		registry.emplace<engine::inputListenerComponent>(c, std::vector<engine::key>{}, std::vector<engine::key>{engine::key::w, engine::key::a, engine::key::s, engine::key::d}, true);
	}
public:
	testSystem(std::shared_ptr<engine::context> ctx) : engine::system(ctx) {}
	engine::error checkError()
	{
		return {};
	}

	void onUpdate(entt::registry& registry)
	{
	}

	void onRender(entt::registry& registry)
	{
	}

	void onEvent(entt::registry& registry, std::shared_ptr<engine::baseEvent> e)
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

		// transform staff.
		std::vector<entt::entity> toUpdate;
		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::k)
		{
			for (auto [entity, uid, mesh, transform] : registry.view<engine::uidComponent, engine::meshComponent, engine::transformComponent>().each())
			{
				transform.transform = glm::translate(transform.transform, glm::vec3(0.1f, 0.1f, 0.1f));
				toUpdate.push_back(entity);
			}
		}

		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::i)
		{
			for (auto [entity, uid, mesh, transform] : registry.view<engine::uidComponent, engine::meshComponent, engine::transformComponent>().each())
			{
				transform.transform = glm::rotate(transform.transform, glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				toUpdate.push_back(entity);
			}
		}

		for (auto e : toUpdate)
		{
			registry.emplace_or_replace<engine::applyTransformComponent>(e);
		}

		// spawn dynamic mesh.
		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::v)
		{
			auto texture = mCtx->mAmanager->loadTexture("../assets/textures/obsidian.jpg");
			auto shader = mCtx->mAmanager->loadShader("../assets/shaders/vertex.glsl", "../assets/shaders/fragment.glsl");

			auto getMovedCube = []()->std::vector<engine::vertex>
				{
					auto vertexes = std::vector<engine::vertex>{
						// Front face
						{ { 0.5f, 0.5f, 0.5f}, { 1.0f, 1.0f }, 0 },
						{ { 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, 0 },
						{ {-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, 0 },
						{ {-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, 0 },

						// Back face
					{ { 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, 0 },
					{ { 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, 0 },
					{ {-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, 0 },
					{ {-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, 0 },
					};

					std::mt19937 rng(std::random_device{}());
					std::uniform_real_distribution<float> dist(-10.0f, 10.0f);
					glm::vec3 offset(dist(rng), dist(rng), dist(rng));
					glm::mat4 transform = glm::translate(glm::mat4(1.0f), offset);

					for (auto& v : vertexes) {
						glm::vec4 pos = transform * glm::vec4(v.position, 1.0f);
						v.position = glm::vec3(pos);
					}

					return vertexes;
				};

			auto c = registry.create();
			registry.emplace<engine::uidComponent>(c);
			registry.emplace<engine::meshComponent>(
				c,
				getMovedCube(),
				std::vector<uint32_t>{
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
			});

			texture.first->bind();
			int slotID = texture.first->getSlotID();

			registry.emplace<engine::materialComponent>(
				c,
				texture.first,
				shader.first,
				engine::materialComponent::shaderUniformMap{
					{"u_textures[0]", {slotID, 1} }
				}
			);

			registry.emplace<engine::transformComponent>(
				c,
				glm::mat4(1.0f)
			);
		}

		// rmv dynamic mesh.
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

		// spawn instance mesh.
		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::r)
		{
			static auto uid = engine::genUID();

			auto texture = mCtx->mAmanager->loadTexture("../assets/textures/obsidian.jpg");
			auto shader = mCtx->mAmanager->loadShader("../assets/shaders/vertexInstanced.glsl", "../assets/shaders/fragmentInstanced.glsl");

			auto c = registry.create();
			registry.emplace<engine::uidComponent>(c);
			registry.emplace<engine::instancedMeshComponent>(
				c,
				std::make_shared<std::vector<engine::vertex>>(std::vector<engine::vertex>{
				// Front face
					{ { 0.5f, 0.5f, 0.5f}, { 1.0f, 1.0f }, 0 },
					{ { 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, 0 },
					{ {-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, 0 },
					{ {-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, 0 },

						// Back face
					{ { 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f}, 0 },
					{ { 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, 0 },
					{ {-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f}, 0 },
					{ {-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, 0 },
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
			}), uid);

			texture.first->bind();
			int slotID = texture.first->getSlotID();

			registry.emplace<engine::materialComponent>(
				c,
				texture.first,
				shader.first,
				engine::materialComponent::shaderUniformMap{
					{"u_textures[0]", {slotID, 1} }
				}
			);

			auto randomMat4 = []() -> glm::mat4 {
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
				};

			registry.emplace<engine::transformComponent>(
				c,
				randomMat4()
			);
		}

		// rmv instance mesh.
		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::q)
		{
			entt::entity toDelete;
			bool use = false;
			auto view = registry.view<engine::uidComponent, engine::instancedMeshComponent, engine::materialComponent>();
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

		// upd tranform of instc mesh.
		std::vector<entt::entity> toUpdateInst;
		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::k)
		{
			for (auto [entity, uid, mesh, transform] : registry.view<engine::uidComponent, engine::instancedMeshComponent, engine::transformComponent>().each())
			{
				transform.transform = glm::translate(transform.transform, glm::vec3(0.1f, 0.1f, 0.1f));
				toUpdateInst.push_back(entity);
				break;
			}
		}

		if (e->getEventType() == engine::eventType::keyUp && static_cast<engine::keyUpEvent*>(e.get())->getKey() == engine::key::i)
		{
			for (auto [entity, uid, mesh, transform] : registry.view<engine::uidComponent, engine::instancedMeshComponent, engine::transformComponent>().each())
			{
				transform.transform = glm::rotate(transform.transform, glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				toUpdateInst.push_back(entity);
				break;
			}
		}

		for (auto e : toUpdateInst)
		{
			registry.emplace_or_replace<engine::updateMeshComponent>(e);
		}
	}
};
// code above only for tests.

class editorLayer : public engine::layer
{
public:
	editorLayer(std::shared_ptr<engine::context> ctx)
		:
		engine::layer(ctx)
	{
		engine::scene::addUserSystem(std::make_unique<testSystem>(ctx));
	}

	bool onEvent(std::shared_ptr<engine::baseEvent> e)
	{
		return false;
	}

	void onRender()
	{
	}

	void onUpdate()
	{
	}

	engine::error checkError() const { return {}; };
};

class editor : public engine::application
{
public:
	editor() : engine::application()
	{
		if (mErr)
			return;

		pushOverlay(std::make_unique<editorLayer>(mCtx));
	}
};

int main(int argc, char* argv[])
{
	try
	{
		editor e;
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