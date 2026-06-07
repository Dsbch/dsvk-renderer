#include <pch.h>
#include <cgltf.h>
#include <glm/gtc/type_ptr.hpp>
#include "primitiveProcessor.h"

namespace engine
{
	glm::mat4 getNodeLocalTransformMat4(const cgltf_node* node)
	{
		if (node->has_matrix)
			return glm::make_mat4(node->matrix);

		glm::vec3 translation(0.0f);
		glm::quat rotation(1, 0, 0, 0);
		glm::vec3 scale(1.0f);

		if (node->has_translation)
			translation = glm::vec3(node->translation[0], node->translation[1], node->translation[2]);
		if (node->has_rotation)
			rotation = glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]);
		if (node->has_scale)
			scale = glm::vec3(node->scale[0], node->scale[1], node->scale[2]);

		return glm::translate(glm::mat4(1.0f), translation)
			* glm::mat4_cast(rotation)
			* glm::scale(glm::mat4(1.0f), scale);
	}

	transform getNodeLocalTransform(const cgltf_node* node)
	{
		glm::vec3 translation(0.0f);
		glm::quat rotation(1, 0, 0, 0);
		glm::vec3 scale(1.0f);

		if (node->has_translation)
			translation = glm::vec3(node->translation[0], node->translation[1], node->translation[2]);
		if (node->has_rotation)
			rotation = glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]);
		if (node->has_scale)
			scale = glm::vec3(node->scale[0], node->scale[1], node->scale[2]);

		return transform{
			.translation = translation,
			.scale = scale,
			.rotation = rotation,
		};
	}

	glm::mat4 getNodeWorldTransformMat4(const cgltf_node* node)
	{
		if (!node->parent)
			return getNodeLocalTransformMat4(node);

		return getNodeWorldTransformMat4(node->parent) * getNodeLocalTransformMat4(node);
	}

	withError<primitive> processPrimitive(const cgltf_primitive& prim, bool skinned, uint32_t jointOffset)
	{
		primitive result{};

		const cgltf_accessor* positionAccessor = nullptr;
		const cgltf_accessor* normalAccessor = nullptr;
		const cgltf_accessor* texcoordAccessor = nullptr;
		const cgltf_accessor* jointsAccessor = nullptr;
		const cgltf_accessor* weightsAccessor = nullptr;

		int texcoordIndex = 0;
		if (prim.material && prim.material->has_pbr_metallic_roughness)
			texcoordIndex = prim.material->pbr_metallic_roughness.base_color_texture.texcoord;

		auto processAccessors = [&]()
			{
				for (size_t ai = 0; ai < prim.attributes_count; ++ai)
				{
					const cgltf_attribute& attr = prim.attributes[ai];
					switch (attr.type)
					{
					case cgltf_attribute_type_position: positionAccessor = attr.data; break;
					case cgltf_attribute_type_normal: normalAccessor = attr.data; break;
					case cgltf_attribute_type_joints:
					{
						if (!jointsAccessor)
							jointsAccessor = attr.data;

						break;
					}
					case cgltf_attribute_type_weights:
					{
						if (!weightsAccessor)
							weightsAccessor = attr.data;

						break;
					}
					case cgltf_attribute_type_texcoord:
					{
						if (attr.index == texcoordIndex)
							texcoordAccessor = attr.data;

						break;
					}
					default: break;
					}
				}
			};

		auto processVertecies = [&]()
			{
				result.positions.reserve(positionAccessor->count);
				result.normal.reserve(normalAccessor->count);

				if (skinned)
				{
					result.jointIndices.reserve(jointsAccessor->count);
					result.weights.reserve(weightsAccessor->count);
				}

				for (size_t i = 0; i < positionAccessor->count; ++i)
				{
					float pos[3]{};
					float normal[3]{};
					float uv[2]{};

					uint32_t joints[4]{};
					float weights[4]{};

					if (!positionAccessor || !cgltf_accessor_read_float(positionAccessor, i, pos, 3))
						return error{ "can't read position" };

					if (!normalAccessor || !cgltf_accessor_read_float(normalAccessor, i, normal, 3))
						return error{ "can't read normal" };

					if (texcoordAccessor)
						cgltf_accessor_read_float(texcoordAccessor, i, uv, 2);

					if (skinned && (!jointsAccessor || !weightsAccessor || !cgltf_accessor_read_uint(jointsAccessor, i, joints, 4) || !cgltf_accessor_read_float(weightsAccessor, i, weights, 4)))
						return error{ "can't read joints or weights" };

					joints[0] += jointOffset;
					joints[1] += jointOffset;
					joints[2] += jointOffset;
					joints[3] += jointOffset;

					result.positions.push_back(glm::vec4{ pos[0], pos[1], pos[2], uv[0] });
					result.normal.push_back(glm::vec4{ normal[0], normal[1], normal[2], uv[1] });

					if (skinned)
					{
						result.jointIndices.push_back(glm::uvec4{ joints[0], joints[1], joints[2], joints[3] });
						result.weights.push_back(glm::vec4{ weights[0], weights[1], weights[2], weights[3] });
					}
				}

				return error{};
			};

		auto processIndecies = [&]()
			{
				const cgltf_accessor* indexAccessor = prim.indices;

				if (!indexAccessor)
					return error{ "index accessor is undefined" };

				const uint8_t* bufferStart = reinterpret_cast<const uint8_t*>(
					indexAccessor->buffer_view->buffer->data) +
					indexAccessor->buffer_view->offset + indexAccessor->offset;

				size_t stride = 1;
				if (indexAccessor->stride)
				{
					stride = indexAccessor->stride;
				}
				else if (indexAccessor->component_type == cgltf_component_type_r_16u)
				{
					stride = 2;
				}
				else if (indexAccessor->component_type == cgltf_component_type_r_32u)
				{
					stride = 4;
				}

				result.indicies.reserve(indexAccessor->count);

				for (size_t i = 0; i < indexAccessor->count; ++i)
				{
					const uint8_t* elem = bufferStart + i * stride;
					uint32_t indices = 0;

					switch (indexAccessor->component_type)
					{
					case cgltf_component_type_r_16u:
						indices = *reinterpret_cast<const uint16_t*>(elem);
						break;
					case cgltf_component_type_r_32u:
						indices = *reinterpret_cast<const uint32_t*>(elem);
						break;
					case cgltf_component_type_r_8u:
						indices = *reinterpret_cast<const uint8_t*>(elem);
						break;
					default:
						return error{ "index accessor is invalid" };
					}

					result.indicies.push_back(indices);
				}

				return error{};
			};

		if (prim.type != cgltf_primitive_type_triangles)
			return result;

		processAccessors();

		error err = processVertecies();
		if (err)
			return err;

		err = processIndecies();
		if (err)
			return err;

		return result;
	}
}
