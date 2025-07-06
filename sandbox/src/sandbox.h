#pragma once

#include <entt/entt.hpp>
#include <application/application.h>
#include <core/layers/layer.h>
#include <core/scene/systems/system.h>
#include <core/scene/components.h>

namespace sandbox
{
    class sandboxSystem : public engine::system 
    {
    public:
        sandboxSystem(std::shared_ptr<engine::context> ctx);
        engine::error checkError();
        void onUpdate(entt::registry& registry);
        void onRender(entt::registry& registry);
        void onEvent(entt::registry& registry, std::shared_ptr<engine::baseEvent> e);

    private:
        void spawnDefaultCamera(entt::registry& registry);
        
        void spawnCube(entt::registry& registry, const engine::materialComponent& material, glm::mat4 transform, uint32_t meshUID);
        void spawnSphere(entt::registry& registry, const engine::materialComponent& material, glm::mat4 transform, uint32_t meshUID);
        void updateTransform(entt::registry& registry, glm::mat4 translate);
    };

    class sandboxLayer : public engine::layer 
    {
    public:
        sandboxLayer(std::shared_ptr<engine::context> ctx);
        bool onEvent(std::shared_ptr<engine::baseEvent> e);
        void onRender();
        void onUpdate();
        engine::error checkError() const;
    };

    class sandbox : public engine::application 
    {
    public:
        sandbox();
    };
}