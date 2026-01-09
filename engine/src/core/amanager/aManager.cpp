#include <pch.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define CGLTF_IMPLEMENTATION

#include <stb_image.h>
#include <stb_image_write.h>
#include <stb_rect_pack.h>
#include "aManager.h"
#include "gltf.h"
#include "platform/renderer/renderer.h"

#include <cgltf.h>
#include <meshoptimizer.h>
#include <glm/gtc/quaternion.hpp>

namespace engine
{
	static std::string openAndRead(const std::string& name)
	{
		std::ifstream f(name, std::ifstream::in);

		std::stringstream s;

		std::string line;
		while (f)
		{

			if (std::getline(f, line))
			{
				s << line << std::endl;
			}
		}

		f.close();

		return s.str();
	}

	static uint32_t key(const std::string& path)
	{
		return crc32(reinterpret_cast<const uint8_t*>(std::filesystem::canonical(path).string().data()), std::filesystem::canonical(path).string().size());
	}

	withError<std::shared_ptr<texture>> aManager::getTexture(const std::string& path)
	{
		auto it = mLoadedTextures.find(key(path));
		if (it != mLoadedTextures.end())
			return it->second;
		else
			return error{ "tried to access not loaded texture." };
	}

	withError<std::shared_ptr<shader>> aManager::getShader(const std::string& path)
	{
		auto it = mLoadedShaders.find(key(path));
		if (it != mLoadedShaders.end())
			return it->second;
		else
			return error{ "tried to access not loaded texture." };
	}

	withError<std::shared_ptr<shader>> aManager::loadShader(const std::string& path)
	{
		if (!makeShader)
			return error{ "makeShader wasn't set" };

		auto loadRes = getShader(path);
		if (loadRes)
			return loadRes.value();

		std::ifstream file(path, std::ios::ate | std::ios::binary);
		if (!file.is_open())
			return error{ "can't open file {}", path };

		size_t fileSize = (size_t)file.tellg();
		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

		file.seekg(0);
		file.read((char*)buffer.data(), fileSize);
		file.close();

		auto shader = makeShader(buffer);
		if (!shader)
			shader.err();

		mLoadedShaders[key(path)] = shader.value();

		return shader.value();
	}

	withError<std::shared_ptr<texture>> aManager::loadTexture(const std::string& path)
	{
		if (!makeTexture)
			return error{ "makeTexture wasn't set" };

		auto loadRes = getTexture(path);
		if (loadRes)
			return loadRes.value();

		int width, height, nrChannels;
		uint8_t* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
		if (!data)
		{
			return error{ "can't load texture {}", path };;
		}

		auto texture = makeTexture(data, width, height, intToChannel(nrChannels));
		if (!texture)
			texture.err();

		stbi_image_free(data);

		mLoadedTextures[key(path)] = texture.value();

		return texture.value();
	}

	void aManager::setMakeShaderFunc(std::function<withError<std::shared_ptr<shader>>(const std::vector<uint32_t>& src)>&& func)
	{
		makeShader = std::move(func);
	}

	void aManager::setMakeTextureFunc(std::function<withError<std::shared_ptr<texture>>(uint8_t* data, int width, int heigth, imageChannel channel)>&& func)
	{
		makeTexture = std::move(func);
	}

	withError<std::shared_ptr<shader>> aManager::getDefaultTaskShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkMeshAs.spv";

		return loadShader(path);
#endif // VULKAN

		return error{ "not implemented" };
	}

	withError<std::shared_ptr<shader>> aManager::getDefaultMeshShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkMeshMs.spv";

		return loadShader(path);
#endif // VULKAN

		return error{ "not implemented" };
	}

	withError<std::shared_ptr<shader>> aManager::getDefaultPixelShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkMeshPs.spv";

		return loadShader(path);
#endif // VULKAN

		return error{ "not implemented" };
	}

	withError<std::shared_ptr<shader>> aManager::getDefaultComputeShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkCompute.spv";

		return loadShader(path);
#endif // VULKAN

		return error{ "not implemented" };
	}

	static std::pair<glm::vec3, float> calculateBoundingSphere(const std::vector<vertex>& vertices)
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

	withError<std::shared_ptr<texture>> aManager::loadRawTexture(const uint8_t* data, size_t size)
	{
		if (!makeTexture)
			return error{ "makeTexture wasn't set" };

		if (auto found = mLoadedTextures.find(crc32(data, size)); found != mLoadedTextures.end())
			return found->second;

		int width, height, nrChannels;
		uint8_t* decoded = stbi_load_from_memory(data, int(size), &width, &height, &nrChannels, 4);
		if (!decoded)
		{
			return error{ "can't decode texture" };
		}

		auto texture = makeTexture(decoded, width, height, intToChannel(nrChannels));
		if (!texture)
			texture.err();

		stbi_image_free(decoded);

		mLoadedTextures[crc32(data, size)] = texture.value();

		return texture.value();
	}

	static void calculateTangents(
		std::vector<vertex>& v,
		const std::vector<uint32_t>& index
	)
	{
		std::vector<glm::vec3> tan1{};
		tan1.resize(v.size());

		std::vector<glm::vec3> tan2{};
		tan2.resize(v.size());

		for (size_t i = 0; i < index.size(); i += 3)
		{
			uint32_t i1 = index[i];
			uint32_t i2 = index[i + 1];
			uint32_t i3 = index[i + 2];

			const vertex& v1 = v[i1];
			const vertex& v2 = v[i2];
			const vertex& v3 = v[i3];

			float x1 = v2.position.x - v1.position.x;
			float x2 = v3.position.x - v1.position.x;
			float y1 = v2.position.y - v1.position.y;
			float y2 = v3.position.y - v1.position.y;
			float z1 = v2.position.z - v1.position.z;
			float z2 = v3.position.z - v1.position.z;

			float s1 = v2.textureCoords.x - v1.textureCoords.x;
			float s2 = v3.textureCoords.x - v1.textureCoords.x;
			float t1 = v2.textureCoords.y - v1.textureCoords.y;
			float t2 = v3.textureCoords.y - v1.textureCoords.y;

			float r = 1.0F / (s1 * t2 - s2 * t1);

			glm::vec3 sdir{
				(t2 * x1 - t1 * x2) * r,
				(t2 * y1 - t1 * y2) * r,
				(t2 * z1 - t1 * z2) * r
			};

			glm::vec3 tdir{
				(s1 * x2 - s2 * x1) * r,
				(s1 * y2 - s2 * y1) * r,
				(s1 * z2 - s2 * z1) * r
			};

			tan1[i1] += sdir;
			tan1[i2] += sdir;
			tan1[i3] += sdir;

			tan2[i1] += tdir;
			tan2[i2] += tdir;
			tan2[i3] += tdir;
		}

		for (size_t i = 0; i < v.size(); i++)
		{
			const auto& n = v[i].normal;
			const auto& t = tan1[i];

			// Gram-Schmidt orthogonalize.
			v[i].tangent = glm::vec4(glm::normalize(t - n * glm::dot(n, t)), 1.0f);

			// Calculate handedness.
			v[i].tangent.w = (glm::dot(glm::cross(n, t), tan2[i]) < 0.0F) ? -1.0F : 1.0F;
		}
	}

	static std::vector<uint32_t> repackPrimitives(
		const std::vector<uint8_t>& primitives,
		std::vector<meshlet>& meshlets
	)
	{
		std::vector<uint32_t> repacked{};
		repacked.reserve(primitives.size() / 3);

		for (auto& m : meshlets)
		{
			// Save triangle offset for current meshlet
			uint32_t triangleOffset = uint32_t((repacked.size()));

			// Repack to uint32_t
			for (uint32_t k = 0; k < m.triangleCount; ++k)
			{
				uint32_t i0 = 3 * k + 0 + m.triangleBufferOffset;
				uint32_t i1 = 3 * k + 1 + m.triangleBufferOffset;
				uint32_t i2 = 3 * k + 2 + m.triangleBufferOffset;

				uint8_t  vIdx0 = primitives[i0];
				uint8_t  vIdx1 = primitives[i1];
				uint8_t  vIdx2 = primitives[i2];
				uint32_t packed = ((static_cast<uint32_t>(vIdx0) & 0xFF) << 0) |
					((static_cast<uint32_t>(vIdx1) & 0xFF) << 8) |
					((static_cast<uint32_t>(vIdx2) & 0xFF) << 16);

				repacked.push_back(packed);
			}

			// Update triangle offset for current meshlet
			m.triangleBufferOffset = triangleOffset;
		}

		return repacked;
	}

	static error remapMesh(
		const std::vector<vertex>& vertecies,
		const std::vector<uint32_t> indicies,
		std::vector<vertex>& vOut,
		std::vector<uint32_t>& iOut
	)
	{
		vOut.clear();
		iOut.clear();

		std::vector<unsigned int> remap(indicies.size());

		size_t vertex_count = meshopt_generateVertexRemapCustom(
			remap.data(),
			indicies.data(),
			indicies.size(),
			&vertecies.front().position.x,
			vertecies.size(),
			sizeof(vertex),
			[&](unsigned int lhs, unsigned int rhs) -> bool
			{
				const vertex& lv = vertecies[lhs];
				const vertex& rv = vertecies[rhs];

				return fabsf(lv.textureCoords.x - rv.textureCoords.x) < 1e-3f &&
					fabsf(lv.textureCoords.y - rv.textureCoords.y) < 1e-3f &&
					fabsf(lv.normal.x - rv.normal.x) < 1e-3f &&
					fabsf(lv.normal.y - rv.normal.y) < 1e-3f &&
					fabsf(lv.normal.z - rv.normal.z) < 1e-3f &&
					fabsf(lv.tangent.x - rv.tangent.x) < 1e-3f &&
					fabsf(lv.tangent.y - rv.tangent.y) < 1e-3f &&
					fabsf(lv.tangent.z - rv.tangent.z) < 1e-3f;
			}
		);
		if (vertex_count == 0)
			return error{ "vertex count is zero" };

		iOut.resize(indicies.size());
		meshopt_remapIndexBuffer(iOut.data(), indicies.data(), indicies.size(), remap.data());

		vOut.resize(vertex_count);
		meshopt_remapVertexBuffer(vOut.data(), vertecies.data(), vertecies.size(), sizeof(vertex), remap.data());

		return {};
	}

	static error generateMeshlets(
		const std::vector<vertex>& vertecies,
		const std::vector<uint32_t>& indicies,
		std::vector<meshlet>& mOut,
		std::vector<uint8_t>& pOut,
		std::vector<uint32_t>& iOut,
		size_t maxVert, size_t maxTriangles, float coneWieght,
		float errorLevel,
		size_t targetIndexCount
	)
	{
		mOut.clear();
		pOut.clear();
		iOut.clear();

		iOut = indicies;

		std::vector<uint32_t> simplyfiedIndexBuf;
		simplyfiedIndexBuf.resize(indicies.size());

		size_t actualSize = meshopt_simplify(
			simplyfiedIndexBuf.data(),
			indicies.data(),
			indicies.size(),
			&vertecies.front().position.x,
			vertecies.size(),
			sizeof(vertex),
			targetIndexCount,
			errorLevel
		);

		simplyfiedIndexBuf.resize(actualSize);

		const size_t maxMeshlets = meshopt_buildMeshletsBound(iOut.size(), maxVert, maxTriangles);
		if (maxMeshlets == 0)
			return error{ "maxMeshlets is zero" };

		std::vector<meshopt_Meshlet> meshlets;
		meshlets.resize(maxMeshlets);

		pOut.resize(maxMeshlets * maxTriangles * 3);
		iOut.resize(maxMeshlets * maxVert);

		size_t meshletCount = meshopt_buildMeshlets(
			meshlets.data(),											// Output: array of meshopt_Meshlet
			iOut.data(),												// Output: array of uint32_t - meshlet to mesh index mappings
			pOut.data(),												// Output: array of uint8_t - triangle indices
			simplyfiedIndexBuf.data(),									// Input: pointer mesh vertex indices
			simplyfiedIndexBuf.size(),									// Input: number of vertex indices
			&vertecies.front().position.x,								// Input: pointer to vertex positions
			vertecies.size(),											// Input: number of vertex positions	
			sizeof(vertex),												// Input: stride of vertex position elements
			maxVert,													// Input: maximum number of vertices per meshlet
			maxTriangles,												// Input: maximum number of triangles per meshlet
			coneWieght													// Input: cone weight (we'll discuss this eventually...maybe)
		);
		if (meshletCount == 0)
			return error{ "meshletCount is zero" };

		auto& last = meshlets[meshletCount - 1];
		iOut.resize(last.vertex_offset + last.vertex_count);
		pOut.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
		meshlets.resize(meshletCount);

		for (auto& m : meshlets)
		{
			meshopt_optimizeMeshlet(&iOut[m.vertex_offset], &pOut[m.triangle_offset], m.triangle_count, m.vertex_count);
		}

		for (size_t k = 0; k < meshlets.size(); k++)
		{
			meshopt_Meshlet m = meshlets[k];

			meshopt_Bounds bounds = meshopt_computeMeshletBounds(
				&iOut[m.vertex_offset],
				&pOut[m.triangle_offset],
				m.triangle_count,
				&vertecies.front().position[0],
				vertecies.size(),
				sizeof(vertex)
			);

			mOut.push_back(
				meshlet{
					.indexBufferIndex = 0,
					.indexBufferOffset = m.vertex_offset,
					.vertexBufferIndex = 0,
					.vertexBufferOffset = 0,
					.vertexCount = m.vertex_count,
					.triangleBufferIndex = 0,
					.triangleBufferOffset = m.triangle_offset,
					.triangleCount = m.triangle_count,
					.bounds = meshletBounds{
						.center = { bounds.center[0], bounds.center[1], bounds.center[2] },
						.radius = bounds.radius,
						.coneAxis = { bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2] },
						.coneCutoff = bounds.cone_cutoff,
					},
				}
				);
		}

		return {};
	}

	withError<model> aManager::loadModelGLTF(
		const std::string& path,
		size_t maxVert,
		size_t maxTriangles,
		float coneWieght
	)
	{
		cgltf_options options{};
		cgltf_data* data = nullptr;

		if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success)
			return error{ "can't open file {}", path };

		if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success)
		{
			cgltf_free(data);
			return error{ "can't load buffers for gtlf: {}", path };
		}

		model result{
			.id = genUID(),
			.meshData = mesh{
				.vertex = std::make_shared<std::vector<vertex>>(),
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
			},
			.mat = material{},
		};

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
				if (!node->parent)
					return getNodeLocalTransform(node);

				return getNodeWorldTransform(node->parent) * getNodeLocalTransform(node);
			};

		struct primitive
		{
			std::vector<vertex> vertecies;
			std::vector<uint32_t> indicies;
		};

		auto processPrimitive = [](const cgltf_primitive& prim, const glm::mat4& transform) -> primitive
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
			};

		auto processMaterial = [&](const cgltf_material* gltfMaterial) -> withError<materialTextures>
			{
				materialTextures result;

				std::filesystem::path baseDir = std::filesystem::path{ path }.parent_path();

				if (gltfMaterial->has_pbr_metallic_roughness)
				{
					// albedo.
					if (auto albedoTexture = gltfMaterial->pbr_metallic_roughness.base_color_texture.texture; albedoTexture && albedoTexture->image)
					{
						if (albedoTexture->image->uri)
						{
							auto relatiePath = baseDir / albedoTexture->image->uri;

							auto albedo = loadTexture(relatiePath.string());
							if (!albedo)
								return albedo.err();

							result.albedo = albedo.value();
						}
						else if (auto bufferView = albedoTexture->image->buffer_view; bufferView && bufferView->buffer->data && bufferView->size != 0)
						{
							uint8_t* ptr = static_cast<uint8_t*>(bufferView->buffer->data);
							ptr += bufferView->offset;

							auto albedo = loadRawTexture(ptr, bufferView->size);
							if (!albedo)
								return albedo.err();

							result.albedo = albedo.value();
						}
					}

					// metallic-roughness.
					if (auto metallicRoughness = gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture; metallicRoughness && metallicRoughness->image)
					{
						if (metallicRoughness->image->uri)
						{
							auto relatiePath = baseDir / metallicRoughness->image->uri;

							auto metallicRougness = loadTexture(relatiePath.string());
							if (!metallicRougness)
								return metallicRougness.err();

							result.metallicRoughness = metallicRougness.value();
						}
						else if (auto bufferView = metallicRoughness->image->buffer_view; bufferView && bufferView->buffer->data && bufferView->size != 0)
						{
							uint8_t* ptr = static_cast<uint8_t*>(bufferView->buffer->data);
							ptr += bufferView->offset;

							auto metRoughness = loadRawTexture(ptr, bufferView->size);
							if (!metRoughness)
								return metRoughness.err();

							result.metallicRoughness = metRoughness.value();
						}
					}
				}

				// normal.
				if (auto normalTexture = gltfMaterial->normal_texture.texture; normalTexture && normalTexture->image)
				{
					if (normalTexture->image->uri)
					{
						auto relatiePath = baseDir / normalTexture->image->uri;

						auto normal = loadTexture(relatiePath.string());
						if (!normal)
							return normal.err();

						result.normal = normal.value();
					}
					else if (auto bufferView = normalTexture->image->buffer_view; bufferView && bufferView->buffer->data && bufferView->size != 0)
					{
						uint8_t* ptr = static_cast<uint8_t*>(bufferView->buffer->data);
						ptr += bufferView->offset;

						auto normal = loadRawTexture(ptr, bufferView->size);
						if (!normal)
							return normal.err();

						result.normal = normal.value();
					}
				}

				return result;
			};

		for (size_t ni = 0; ni < data->nodes_count; ++ni)
		{
			const cgltf_node* node = &data->nodes[ni];
			if (!node->mesh)
				continue;

			glm::mat4 transform = getNodeWorldTransform(node);
			const cgltf_mesh& mesh = *node->mesh;

			for (size_t pri = 0; pri < mesh.primitives_count; ++pri)
			{
				primitive crntPrimitive = processPrimitive(mesh.primitives[pri], transform);

				calculateTangents(crntPrimitive.vertecies, crntPrimitive.indicies);

				std::vector<vertex> remappedVertex;
				std::vector<uint32_t> remappedIndex;

				error err = remapMesh(crntPrimitive.vertecies, crntPrimitive.indicies, remappedVertex, remappedIndex);
				if (err)
					return err;

				std::vector<meshlet> meshlets;
				std::vector<uint32_t> indices;
				std::vector<uint8_t> primitives;

				err = generateMeshlets(remappedVertex, remappedIndex, meshlets, primitives, indices, maxVert, maxTriangles, coneWieght, 0, 0);
				if (err)
					return err;

				std::vector<uint32_t> repackedPrimitives = repackPrimitives(primitives, meshlets);

				auto material = processMaterial(mesh.primitives[pri].material);
				if (!material)
					return material.err();
			}
		}

		cgltf_free(data);

		return result;
	}

	withError<std::pair<std::shared_ptr<texture>, std::vector<atlasMapping>>> aManager::makeTextureAtlas(const std::vector<image>& images)
	{
		std::vector<uint32_t> crcVals;

		for (auto& i : images)
		{
			crcVals.push_back(crc32(i.data, i.w * i.h * i.channels));
		}

		uint32_t merged = crc32(reinterpret_cast<uint8_t*>(crcVals.data()), crcVals.size() * sizeof(uint32_t));

		if (auto found = mLoadedTextureAtlases.find(merged); found != mLoadedTextureAtlases.end())
			return found->second;

		std::pair<std::shared_ptr<texture>, std::vector<atlasMapping>> result{ nullptr, {} };

		std::vector<stbrp_rect> rects(images.size());

		for (int i = 0; i < images.size(); i++)
		{
			rects[i].id = i;
			rects[i].w = images[i].w + images[i].padding * 2;
			rects[i].h = images[i].h + images[i].padding * 2;
		}

		int maxWidth = 0;
		int maxHeight = 0;
		int totalArea = 0;

		for (int i = 0; i < images.size(); i++)
		{
			maxWidth = std::max(images[i].w + images[i].padding * 2, maxWidth);
			maxHeight = std::max(images[i].h + images[i].padding * 2, maxHeight);
			totalArea += (images[i].w + images[i].padding * 2) * (images[i].h + images[i].padding * 2);
		}

		int size = std::max(size, maxWidth);
		size = std::max(size, maxHeight);

		while (size * size < totalArea) size *= 2;

		while (true)
		{
			stbrp_context ctx;
			std::vector<stbrp_node> nodes(size);

			stbrp_init_target(&ctx, size, size, nodes.data(), size);
			if (stbrp_pack_rects(&ctx, rects.data(), int(rects.size())))
				break;

			size *= 2;
		}

		std::vector<uint8_t> atlas(size * size * 4, 0);

		for (auto& r : rects)
		{
			if (!r.was_packed)
				return error{ "texture wasn't packed" };

			auto img = images[r.id];

			// write main image.
			for (int y = 0; y < img.h; y++)
			{
				unsigned char* src = img.data + y * img.w * 4;
				unsigned char* dst = atlas.data() + ((r.y + y + img.padding) * size + r.x + img.padding) * 4;

				std::memcpy(dst, src, img.w * 4);
			}

			// Write padding.
			auto base = atlas.data();

			// Top.
			for (int i = 0; i < img.w + img.padding * 2; i++)
			{
				int x = r.x + i;
				int y = r.y;

				unsigned char* dst = base + (x + y * size) * 4;

				unsigned char* colorPtr = base + (x + (y + img.padding) * size) * 4;

				unsigned char r = *colorPtr;
				unsigned char g = *(colorPtr + 1);
				unsigned char b = *(colorPtr + 2);
				unsigned char a = *(colorPtr + 3);

				for (int k = 0; k < img.padding; k++)
				{
					dst[0] = r;
					dst[1] = g;
					dst[2] = b;
					dst[3] = a;

					dst += size * 4;
				}
			}

			// Bottom.
			for (int i = 0; i < img.w + img.padding * 2; i++)
			{
				int x = r.x + i;
				int y = r.y + img.padding + img.h;

				unsigned char* dst = base + (x + y * size) * 4;

				unsigned char* colorPtr = base + (x + (y - 1) * size) * 4;

				unsigned char r = *colorPtr;
				unsigned char g = *(colorPtr + 1);
				unsigned char b = *(colorPtr + 2);
				unsigned char a = *(colorPtr + 3);

				for (int k = 0; k < img.padding; k++)
				{
					dst[0] = r;
					dst[1] = g;
					dst[2] = b;
					dst[3] = a;

					dst += size * 4;
				}
			}

			// Right.
			for (int i = 0; i < img.h + img.padding * 2; i++)
			{
				int x = r.x + img.padding + img.w;
				int y = r.y + i;

				unsigned char* dst = base + (x + y * size) * 4;

				unsigned char* colorPtr = dst - 4;

				unsigned char r = *colorPtr;
				unsigned char g = *(colorPtr + 1);
				unsigned char b = *(colorPtr + 2);
				unsigned char a = *(colorPtr + 3);

				for (int k = 0; k < img.padding; k++)
				{
					dst[0] = r;
					dst[1] = g;
					dst[2] = b;
					dst[3] = a;

					dst += 4;
				}
			}

			// Left.
			for (int i = 0; i < img.h + img.padding * 2; i++)
			{
				int x = r.x;
				int y = r.y + i;

				unsigned char* dst = base + (x + y * size) * 4;

				unsigned char* colorPtr = dst + img.padding * 4;

				unsigned char r = *colorPtr;
				unsigned char g = *(colorPtr + 1);
				unsigned char b = *(colorPtr + 2);
				unsigned char a = *(colorPtr + 3);

				for (int k = 0; k < img.padding; k++)
				{
					dst[0] = r;
					dst[1] = g;
					dst[2] = b;
					dst[3] = a;

					dst += 4;
				}
			}
		}

		auto atlasTexture = makeTexture(atlas.data(), size, size, rgba);
		if (!atlasTexture)
			return atlasTexture.err();

		result.first = atlasTexture.value();

		for (auto& r : rects)
		{
			result.second.push_back(
				atlasMapping{
					.index = r.id,
					.x = r.w,
					.y = r.h,
				}
				);
		}

		mLoadedTextureAtlases[merged] = result;

		return result;
	}

	void aManager::testTextureAtlassing()
	{
		std::vector<aManager::image> images;
		std::vector<std::string> names = {
			"../assets/test/1.jpg",
			"../assets/test/2.jpg",
			"../assets/test/3.jpg",
			"../assets/test/4.png",
			"../assets/test/5.png",
		};

		for (auto& n : names)
		{
			aManager::image img;

			img.data = stbi_load(n.c_str(), &img.w, &img.h, &img.channels, 4);
			if (!img.data)
				return;

			img.padding = std::max(img.w, img.h) / 128;

			images.push_back(img);
		}

		auto texture = makeTextureAtlas(images);

		for (auto& i : images)
		{
			stbi_image_free(i.data);
		}
	}
}