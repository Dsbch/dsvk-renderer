#include <pch.h>
#include <cgltf.h>
#include "primitiveProcessor.h"

namespace engine
{
	glm::mat4 getNodeWorldTransform(const cgltf_node* node)
	{
		auto getNodeLocalTransform = [](const cgltf_node* node)
			{
				if (node->has_matrix)
				{
					glm::mat4 m;
					memcpy(&m[0][0], node->matrix, sizeof(float) * 16);
					return m;
				}
				else
				{
					glm::vec3 translation(0.0f);
					translation = glm::vec3(node->translation[0], node->translation[1], node->translation[2]);

					glm::quat rotation(1, 0, 0, 0);
					rotation = glm::quat(node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]);

					glm::vec3 scale(1.0f);
					scale = glm::vec3(node->scale[0], node->scale[1], node->scale[2]);

					return glm::translate(glm::mat4(1.0f), translation)
						* glm::mat4_cast(rotation)
						* glm::scale(glm::mat4(1.0f), scale);
				}
			};

		if (!node->parent)
			return getNodeLocalTransform(node);

		return getNodeWorldTransform(node->parent) * getNodeLocalTransform(node);
	}

	primitive processPrimitive(const cgltf_primitive& prim, const glm::mat4& transform, cgltf_material* materials)
	{
		primitive result;

		if (prim.type != cgltf_primitive_type_triangles)
			return result;

		const cgltf_accessor* positionAccessor = nullptr;
		const cgltf_accessor* normalAccessor = nullptr;
		const cgltf_accessor* texcoordAccessor = nullptr;

		for (size_t ai = 0; ai < prim.attributes_count; ++ai)
		{
			const cgltf_attribute& attr = prim.attributes[ai];
			switch (attr.type)
			{
				case cgltf_attribute_type_position: positionAccessor = attr.data; break;
				case cgltf_attribute_type_normal: normalAccessor = attr.data; break;
				case cgltf_attribute_type_texcoord: texcoordAccessor = attr.data; break;
				default: break;
			}
		}

		if (!positionAccessor || positionAccessor->component_type != cgltf_component_type_r_32f || positionAccessor->type != cgltf_type_vec3)
			return result;

		result.vertecies.reserve(positionAccessor->count);

		for (size_t i = 0; i < positionAccessor->count; ++i)
		{
			vertex v {
				.localMaterialOffset = uint32_t(prim.material - materials),
			};

			float pos[3]{};
			cgltf_accessor_read_float(positionAccessor, i, pos, 3);
			glm::vec4 localPos(pos[0], pos[1], pos[2], 1.0f);
			v.position = glm::vec3(transform * localPos);

			if (normalAccessor)
			{
				float norm[3]{};
				cgltf_accessor_read_float(normalAccessor, i, norm, 3);
				glm::vec3 n(norm[0], norm[1], norm[2]);
				v.normal = glm::normalize(glm::transpose(glm::inverse(glm::mat3(transform))) * n);
			}

			if (texcoordAccessor)
			{
				float uv[2]{};
				cgltf_accessor_read_float(texcoordAccessor, i, uv, 2);
				v.textureCoords = glm::vec2(uv[0], uv[1]);
			}

			result.vertecies.push_back(v);
		}

		if (prim.indices)
		{
			const cgltf_accessor* indexAccessor = prim.indices;
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

			if (prim.indices)
				result.indicies.reserve(indexAccessor->count);

			for (size_t i = 0; i < indexAccessor->count; ++i)
			{
				const uint8_t* elem = bufferStart + i * stride;
				uint32_t index = 0;

				switch (indexAccessor->component_type)
				{
				case cgltf_component_type_r_16u:
					index = *reinterpret_cast<const uint16_t*>(elem);
					break;
				case cgltf_component_type_r_32u:
					index = *reinterpret_cast<const uint32_t*>(elem);
					break;
				case cgltf_component_type_r_8u:
					index = *reinterpret_cast<const uint8_t*>(elem);
					break;
				default:
					continue;
				}

				result.indicies.push_back(index);
			}
		}

		return result;
	}
}