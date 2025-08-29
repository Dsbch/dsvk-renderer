#pragma once

#include <pch.h>
#include "base/context/context.h"
#include "vertexArrayObject.h"
#include "arrayObject.h"
#include "shader.h"
#include "texture.h"

namespace engine
{
	class renderer
	{
	public:
		renderer(std::shared_ptr<context> ctx) : mCtx(ctx) {};
		virtual ~renderer() = default;
		virtual std::string getVersion() const = 0;
		virtual error checkError() const = 0;
		virtual void changeViewPort(uint32_t width, uint32_t height) const = 0;
		virtual void clear() const = 0;
		virtual void render() const = 0;
		virtual void render(const shaderProgram* shader, cubeMap* albedoTexture, const vertexArrayObject* vao) const = 0;
		virtual void render(const shaderProgram* shader, texture* albedoTexture, const vertexArrayObject* vao) const = 0;
		virtual void render(const shaderProgram* shader, texture* albedoTexture, const vertexArrayObject* vao, uint32_t instanceCount) const = 0;
		virtual void render(const shaderProgram* shader, texture* albedoTexture, const vertexArrayObject* vao, const dynamicArrayObject* indirectBuffer, size_t indirectBufferSize) const = 0;
	protected:
		error mErr;
		std::shared_ptr<context> mCtx;
	};

	// Код ниже просто для накидки на будущую архитектуру мешей/текстур.
	//struct mesh
	//{
	//	std::vector<uint32_t> vertexBuffer;
	//	std::vector<uint32_t> indexBuffer;

	//	bool isMeshlets;
	//	std::vector<uint32_t> primitiveBuffer;
	//	std::vector<uint32_t> vertexIndexBuffer;
	//};

	//struct meshHandle
	//{
	//	// погугли сколько лод левелов делать + как их делать :).
	//	std::array<mesh, 4> lodLevels;

	//	// функция для определения хеша.
	//	uint32_t getHesh() const;
	//};

	//class shaderProgram
	//{
	//public:
	//	shaderProgram(const std::vector<uint8_t>& vertexSPIRV, const std::vector<uint8_t>& fragmestSPIRV);
	//	shaderProgram(const std::vector<uint8_t>& meshShaderSPIRV, const std::vector<uint8_t>& fragmestSPIRV);
	//	
	//	virtual ~shaderProgram() = default;
	//private:
	//};

	//struct material
	//{
	// std::shared_ptr<shaderProgram> shader;
	// 
	// std::shared_ptr<texture> albedoTexture;
	// std::shared_ptr<texture> roughnessTexture;
	// std::shared_ptr<texture> normalTexture;
	// std::shared_ptr<texture> metalicTexture;
	// std::shared_ptr<texture> aoTexture;
	//};

	//struct model
	//{
	//	material mat;
	//	meshHandle mesh;
	//	glm::mat4 transform;
	//	// Скорее всего тут будет инфа с анимациями или ещё чем-то.
	//};

	// Про этот класс не особо уверен, скорее всего лучше будет сделать его таким образом,
	// что всегда будет thread на фоне который будет заниматся обновлением данных на gpu со всеми локировками, 
	// этот вопрос нужно очень хорошо изучить.
	// 
	// Можно его разбить на несколько, но при этом он не должен быть в курсе за ECS вообще.
	// Конечно будет ECS класс выше, который будет просто собирать model структуру и дергать addToRender и render.
	// 
	// Этот класс будет ответственнен за:
	// 1. Инстансинг
	// 2. Culling on GPU
	// 3. И короче всем :).
	//class renderer
	//{
	//public:
	//	// Добавление в очередь на рендер.
	//	virtual void addToRender(const model& m) = 0;
	//	// Рендер всей очереди.
	//	virtual void render() = 0;
	//};
}
