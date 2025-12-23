#include <pch.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf.h>
#include <glm/gtc/quaternion.hpp>
#include <meshoptimizer.h>
#include "gltf.h"
#include "platform/renderer/vertex.h"

namespace engine
{
	static error loadMeshFromGLTF(const std::string& path, std::vector<vertex>& outVertices, std::vector<uint32_t>& outIndices)
	{
		cgltf_options options{};
		cgltf_data* data = nullptr;

		if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success)
			return { "can't open file {}", path };

		if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success)
		{
			cgltf_free(data);
			return { "can't load buffers for gtlf: {}", path };
		}

		outVertices.clear();
		outIndices.clear();

		auto getNodeLocalTransform = [](const cgltf_node* node) -> glm::mat4
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

		std::function<glm::mat4(const cgltf_node*)> getNodeWorldTransform = [&](const cgltf_node* node) -> glm::mat4
			{
				if (!node->parent) return getNodeLocalTransform(node);
				return getNodeWorldTransform(node->parent) * getNodeLocalTransform(node);
			};

		auto processPrimitive = [&](const cgltf_primitive& prim, const glm::mat4& transform)
			{
				if (prim.type != cgltf_primitive_type_triangles) return;

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
					return;

				uint32_t baseIndex = static_cast<uint32_t>(outVertices.size());

				for (size_t i = 0; i < positionAccessor->count; ++i)
				{
					vertex v{};

					float pos[3]{};
					cgltf_accessor_read_float(positionAccessor, i, pos, 3);
					glm::vec4 localPos(pos[0], pos[1], pos[2], 1.0f);
					v.position = glm::vec3(transform * localPos);

					if (normalAccessor)
					{
						float norm[3]{};
						cgltf_accessor_read_float(normalAccessor, i, norm, 3);
						glm::vec3 n(norm[0], norm[1], norm[2]);
						v.normal = glm::normalize(glm::mat3(glm::transpose(glm::inverse(transform))) * n);
					}

					if (texcoordAccessor)
					{
						float uv[2]{};
						cgltf_accessor_read_float(texcoordAccessor, i, uv, 2);
						v.textureCoords = glm::vec2(uv[0], uv[1]);
					}

					outVertices.push_back(v);
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

					for (size_t i = 0; i < indexAccessor->count; ++i)
					{
						const uint8_t* elem = bufferStart + i * stride;
						uint32_t index = 0;

						switch (indexAccessor->component_type)
						{
						case cgltf_component_type_r_16u:
							index = *reinterpret_cast<const uint16_t*>(elem); break;
						case cgltf_component_type_r_32u:
							index = *reinterpret_cast<const uint32_t*>(elem); break;
						case cgltf_component_type_r_8u:
							index = *reinterpret_cast<const uint8_t*>(elem); break;
						default:
							continue;
						}

						outIndices.push_back(baseIndex + index);
					}
				}
			};

		for (size_t ni = 0; ni < data->nodes_count; ++ni)
		{
			const cgltf_node* node = &data->nodes[ni];
			if (!node->mesh)
				continue;

			glm::mat4 transform = getNodeWorldTransform(node);
			const cgltf_mesh& mesh = *node->mesh;

			for (size_t pri = 0; pri < mesh.primitives_count; ++pri) {
				processPrimitive(mesh.primitives[pri], transform);
			}
		}

		cgltf_free(data);

		return {};
	}

	std::pair<glm::vec3, float> calculateBoundingSphere(const std::vector<vertex>& vertices)
	{
		auto findFarthest = [&](glm::vec3 point)-> glm::vec3
			{
				glm::vec3 result{ 0.0f };
				float maxLength = 0.0f;

				for (auto& v : vertices)
				{
					auto length = glm::length(v.position - point);

					if (length > maxLength)
					{
						maxLength = length;
						result = v.position;
					}
				}

				return result;
			};

		std::pair<glm::vec3, float> result{ glm::vec3(1.0f), 0.0f };

		glm::vec3 first{ vertices[std::rand() % vertices.size()].position };
		glm::vec3 second = findFarthest(first);
		glm::vec3 third = findFarthest(second);

		glm::vec3 potentialCenter = (second + third) / 2.0f;
		float potentialRadius = glm::length(third - potentialCenter);

		for (auto& v : vertices)
		{
			glm::vec3 toCenter = v.position - potentialCenter;
			float crntRadius = glm::length(toCenter);

			if (crntRadius > potentialRadius)
			{
				float newRadius = potentialRadius + (crntRadius - potentialRadius) / 2.0f;
				potentialCenter += toCenter - (toCenter * (newRadius / crntRadius));
				potentialRadius = newRadius;
			}
		}

		result.first = potentialCenter;
		result.second = potentialRadius;

		return result;
	}

	withError<mesh> loadMesh(const std::string& path, size_t maxVert, size_t maxTriangles, float coneWieght)
	{
		std::vector<vertex> vertexBuf;
		std::vector<uint32_t> indexBuf;

		error err = loadMeshFromGLTF(path, vertexBuf, indexBuf);
		if (err)
			return err;

		mesh result{
			.index = dataWithLodLevels<uint32_t>{
				.second = 0,
				.third = 0,
				.fourth = 0,
				.data = std::make_shared<std::vector<uint32_t>>()
			},
			.primitive = dataWithLodLevels<uint32_t>{
				.second = 0,
				.third = 0,
				.fourth = 0,
				.data = std::make_shared<std::vector<uint32_t>>()
			},
			.mesh = dataWithLodLevels<meshlet>{
				.second = 0,
				.third = 0,
				.fourth = 0,
				.data = std::make_shared<std::vector<meshlet>>()
			},
			.bsCenter = glm::vec3(0.f),
			.bsRadius = 0.0f
		};

		for (size_t i = 0; i < 4; i++)
		{
			// preapre offsets.
			switch (i)
			{
			case 1:
			{
				result.index.second = uint32_t(result.index.data->size());
				result.mesh.second = uint32_t(result.mesh.data->size());
				result.primitive.second = uint32_t(result.primitive.data->size());
				break;
			}
			case 2:
			{
				result.index.third = uint32_t(result.index.data->size());
				result.mesh.third = uint32_t(result.mesh.data->size());
				result.primitive.third = uint32_t(result.primitive.data->size());
				break;
			}
			case 3:
			{
				result.index.fourth = uint32_t(result.index.data->size());
				result.mesh.fourth = uint32_t(result.mesh.data->size());
				result.primitive.fourth = uint32_t(result.primitive.data->size());
				break;
			}
			}

			std::vector<unsigned int> remap(indexBuf.size());
			size_t vertex_count = meshopt_generateVertexRemap(
				remap.data(),
				indexBuf.data(),
				indexBuf.size(),
				&vertexBuf.front().position.x,
				vertexBuf.size(),
				sizeof(vertex)
			);

			if (vertex_count == 0)
				return error{ "vertex count is zero" };

			std::vector<uint32_t> remappedIndexBuffer(indexBuf.size());
			meshopt_remapIndexBuffer(remappedIndexBuffer.data(), indexBuf.data(), indexBuf.size(), remap.data());

			if (i == 0)
			{
				result.vertex = std::make_shared<std::vector<vertex>>(vertex_count);
				meshopt_remapVertexBuffer(result.vertex->data(), vertexBuf.data(), vertexBuf.size(), sizeof(vertex), remap.data());
			}

			// simplify index buffer for lod levels.
			std::vector<uint32_t> simplyfiedIndexBuf;
			simplyfiedIndexBuf.resize(remappedIndexBuffer.size());

			const float errorLevel = 0.01f;
			size_t actualSize = meshopt_simplify(
				simplyfiedIndexBuf.data(),
				remappedIndexBuffer.data(),
				remappedIndexBuffer.size(),
				&vertexBuf.front().position.x,
				vertexBuf.size(),
				sizeof(vertex),
				remappedIndexBuffer.size() / (i + 1),
				errorLevel
			);

			if (actualSize > simplyfiedIndexBuf.size())
				return error{ "wrong actual size of simplified index buffer" };

			simplyfiedIndexBuf.resize(actualSize);

			const size_t maxMeshlets = meshopt_buildMeshletsBound(simplyfiedIndexBuf.size(), maxVert, maxTriangles);
			if (maxMeshlets == 0)
				return error{ "maxMeshlets is zero" };

			std::vector<meshopt_Meshlet> meshlets;
			std::vector<uint8_t> meshletTriangles;

			meshlets.resize(maxMeshlets);
			auto index = std::vector<uint32_t>(maxMeshlets * maxVert);
			meshletTriangles.resize(maxMeshlets * maxTriangles * 3);

			size_t meshletCount = meshopt_buildMeshlets(
				meshlets.data(),											// Output: array of meshopt_Meshlet
				index.data(),												// Output: array of uint32_t - meshlet to mesh index mappings
				meshletTriangles.data(),									// Output: array of uint8_t - triangle indices
				simplyfiedIndexBuf.data(),									// Input: pointer mesh vertex indices
				simplyfiedIndexBuf.size(),									// Input: number of vertex indices
				&result.vertex->front().position.x,							// Input: pointer to vertex positions
				result.vertex->size(),										// Input: number of vertex positions	
				sizeof(vertex),												// Input: stride of vertex position elements
				maxVert,													// Input: maximum number of vertices per meshlet
				maxTriangles,												// Input: maximum number of triangles per meshlet
				coneWieght													// Input: cone weight (we'll discuss this eventually...maybe)
			);

			if (meshletCount == 0)
				return error{ "meshletCount is zero" };

			auto& last = meshlets[meshletCount - 1];
			index.resize(last.vertex_offset + last.vertex_count);
			meshletTriangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
			meshlets.resize(meshletCount);

			for (auto& m : meshlets)
			{
				meshopt_optimizeMeshlet(&index[m.vertex_offset], &meshletTriangles[m.triangle_offset], m.triangle_count, m.vertex_count);
			}

			for (auto& idx : index)
			{
				result.index.data->push_back(idx);
			}

			auto primitive = std::vector<uint32_t>();
			for (auto& m : meshlets)
			{
				// Save triangle offset for current meshlet
				uint32_t triangleOffset = uint32_t((primitive.size()));

				// Repack to uint32_t
				for (uint32_t k = 0; k < m.triangle_count; ++k)
				{
					uint32_t i0 = 3 * k + 0 + m.triangle_offset;
					uint32_t i1 = 3 * k + 1 + m.triangle_offset;
					uint32_t i2 = 3 * k + 2 + m.triangle_offset;

					uint8_t  vIdx0 = meshletTriangles[i0];
					uint8_t  vIdx1 = meshletTriangles[i1];
					uint8_t  vIdx2 = meshletTriangles[i2];
					uint32_t packed = ((static_cast<uint32_t>(vIdx0) & 0xFF) << 0) |
						((static_cast<uint32_t>(vIdx1) & 0xFF) << 8) |
						((static_cast<uint32_t>(vIdx2) & 0xFF) << 16);
					primitive.push_back(packed);
				}

				// Update triangle offset for current meshlet
				m.triangle_offset = triangleOffset;
			}

			for (auto& prim : primitive)
			{
				result.primitive.data->push_back(prim);
			}

			auto meshletBuff = std::vector<meshlet>();
			meshletBuff.reserve(meshletCount);

			uint32_t indexLodOffset = 0;
			uint32_t primitiveLodOffset = 0;
			switch (i)
			{
			case 1:
			{
				indexLodOffset = result.index.second;
				primitiveLodOffset = result.primitive.second;
				break;
			}
			case 2:
			{
				indexLodOffset = result.index.third;
				primitiveLodOffset = result.primitive.third;
				break;
			}
			case 3:
			{
				indexLodOffset = result.index.fourth;
				primitiveLodOffset = result.primitive.fourth;
				break;
			}
			}

			for (size_t k = 0; k < meshlets.size(); k++)
			{
				meshletBuff.push_back(
					meshlet{
						.indexBufferIndex = 0,
						.indexBufferOffset = meshlets[k].vertex_offset + indexLodOffset,
						.vertexBufferIndex = 0,
						.vertexBufferOffset = 0,
						.vertexCount = meshlets[k].vertex_count,
						.triangleBufferIndex = 0,
						.triangleBufferOffset = meshlets[k].triangle_offset + primitiveLodOffset,
						.triangleCount = meshlets[k].triangle_count
					}
				);
			}

			for (auto& m : meshletBuff)
			{
				result.mesh.data->push_back(m);
			}
		}

		auto bs = calculateBoundingSphere(*result.vertex.get());
		result.bsCenter = bs.first;
		result.bsRadius = bs.second;

		return result;
	}
}