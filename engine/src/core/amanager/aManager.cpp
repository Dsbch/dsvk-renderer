#include <pch.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define CGLTF_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include <stb_image.h>
#include <stb_image_write.h>
#include <stb_rect_pack.h>
#include <stb_image_resize2.h>
#include "aManager.h"
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

	static float toRGB(float color)
	{
		return pow(color, 2.2f);
	}

	static float toSRGB(float color)
	{
		return pow(color, 1.0f / 2.2f);
	}

	withError<std::shared_ptr<shader>> aManager::loadShader(const std::string& path)
	{
		if (!makeShader)
			return error{ "makeShader wasn't set" };

		{
			std::lock_guard l{ mShaderMu };
			auto loadRes = mLoadedShaders.get(key(path));
			if (loadRes)
				return loadRes.value();
		}

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

		{
			std::lock_guard l{ mShaderMu };
			mLoadedShaders.put(key(path), shader.value());
		}

		return shader.value();
	}

	void aManager::clearCache()
	{
		{
			std::lock_guard l1{ mShaderMu };
			mLoadedShaders.clear();
		}

		{
			std::lock_guard l2{ mModelMu };
			mLoadedModels.clear();
		}

		{
			std::lock_guard l3{ mTexturesMu };
			mLoadedTextureAtlases.clear();
		}
	}

	aManager::aManager()
		: mLoadedModels(50), mLoadedShaders(100), mLoadedTextureAtlases(50)
	{
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

	withError<std::shared_ptr<shader>> aManager::getDefaultLineVertexShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkLineVs.spv";

		return loadShader(path);
#endif // VULKAN

		return error{ "not implemented" };
	}

	withError<std::shared_ptr<shader>> aManager::getDefaultLinePixelShader()
	{
#ifdef VULKAN
		const std::string path = "../assets/shaders/vkCompiled/vkLinePs.spv";

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
			glm::vec3 tangent = glm::normalize(t - n * glm::dot(n, t));

			v[i].tangent = glm::vec4(tangent, 0.0f);

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
					fabsf(lv.normal.z - rv.normal.z) < 1e-3f;
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
		{
			std::lock_guard l{ mModelMu };

			auto found = mLoadedModels.get(key(path));
			if (found)
				return found.value();
		}

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
			};

		struct rawTextures
		{
			std::vector<aManager::image> albedo;
			std::vector<aManager::image> normal;
			std::vector<aManager::image> metalicRoughnes;
		};

		struct imageInfo
		{
			int w, h, channels;
		};

		auto getImageInfo = [](const cgltf_texture* texture, const std::filesystem::path& baseDir)->withError<imageInfo>
			{
				imageInfo result{};

				if (texture->image->uri)
				{
					auto relativePath = baseDir / texture->image->uri;

					if (!stbi_info(relativePath.string().c_str(), &result.w, &result.h, &result.channels))
						return error{ "can't get image info: {}", relativePath.string() };
				}
				else if (auto bufferView = texture->image->buffer_view; bufferView && bufferView->buffer->data && bufferView->size != 0)
				{
					uint8_t* ptr = static_cast<uint8_t*>(bufferView->buffer->data);
					ptr += bufferView->offset;

					if (!stbi_info_from_memory(ptr, int(bufferView->size), &result.w, &result.h, &result.channels))
						return error{ "can't get image info" };
				}

				return result;
			};

		auto processTexture = [](const cgltf_texture* texture, const std::filesystem::path& baseDir) -> withError<aManager::image>
			{
				aManager::image img{};

				uint8_t* data = nullptr;

				if (texture->image->uri)
				{
					auto relativePath = baseDir / texture->image->uri;

					int factChannels = 0;

					data = stbi_load(relativePath.string().c_str(), &img.w, &img.h, &factChannels, 4);
					if (!data)
						return error{ "can't load texture with path: {}", relativePath.string() };

					img.channels = 4;
				}
				else if (auto bufferView = texture->image->buffer_view; bufferView && bufferView->buffer->data && bufferView->size != 0)
				{
					uint8_t* ptr = static_cast<uint8_t*>(bufferView->buffer->data);
					ptr += bufferView->offset;

					int factChannels = 0;

					data = stbi_load_from_memory(ptr, int(bufferView->size), &img.w, &img.h, &factChannels, 4);
					if (!data)
						return error{ "can't load texture" };

					img.channels = 4;
				}

				img.padding = std::max(img.w, img.h) / 128;

				img.data.resize(img.w * img.h * img.channels);

				std::memcpy(img.data.data(), data, img.w * img.h * img.channels);

				stbi_image_free(data);

				return img;
			};

		auto applyBaseFactor = [](aManager::image& img, float factor[4])
			{
				if (img.channels != 4)
					return error{ "applyBaseFactor: not RGBA" };

				if (factor[0] == 1.0f && factor[1] == 1.0f && factor[2] == 1.0f && factor[3] == 1.0f)
					return error{};

				auto ptr = img.data.data();
				for (int i = 0; i < img.h * img.w * img.channels; i += img.channels)
				{
					ptr[0] = uint8_t(toSRGB(toRGB(ptr[0] / 255.0f) * factor[0]) * 255.0f);
					ptr[1] = uint8_t(toSRGB(toRGB(ptr[1] / 255.0f) * factor[1]) * 255.0f);
					ptr[2] = uint8_t(toSRGB(toRGB(ptr[2] / 255.0f) * factor[2]) * 255.0f);
					ptr[3] = uint8_t(ptr[3] / 255.0f * factor[3] * 255.0f);

					ptr += img.channels;
				}

				return error{};
			};

		auto applyMetallicRoughnessFactor = [](aManager::image& img, float metallic, float roughness)
			{
				if (metallic == 1.0f && roughness == 1.0f)
					return;

				auto ptr = img.data.data();
				for (int i = 0; i < img.h * img.w * img.channels; i += img.channels)
				{
					ptr[1] = uint8_t(toSRGB(toRGB(ptr[1] / 255.0f) * roughness) * 255.0f);
					ptr[2] = uint8_t(toSRGB(toRGB(ptr[2] / 255.0f) * metallic) * 255.0f);

					ptr += img.channels;
				}
			};

		auto processMaterials = [&](const cgltf_material* materialsPtr, int materialCount) -> withError<rawTextures>
			{
				rawTextures result;

				std::filesystem::path baseDir = std::filesystem::path{ path }.parent_path();

				for (int i = 0; i < materialCount; i++)
				{
					const cgltf_material* material = materialsPtr + i;

					if (material->has_pbr_metallic_roughness)
					{
						// In case if all materials doesn't have textures.
						int w = 64, h = 64, padding = 0;
						if (auto metalicRoughnesTexture = material->pbr_metallic_roughness.metallic_roughness_texture.texture; metalicRoughnesTexture && metalicRoughnesTexture->image)
						{
							auto info = getImageInfo(metalicRoughnesTexture, baseDir);
							if (!info)
								return info.err();

							w = info.value().w, h = info.value().h;
							padding = std::max(w, h) / 128;
						}
						else if (auto albedoTexture = material->pbr_metallic_roughness.base_color_texture.texture; albedoTexture && albedoTexture->image)
						{
							auto info = getImageInfo(albedoTexture, baseDir);
							if (!info)
								return info.err();

							w = info.value().w, h = info.value().h;
							padding = std::max(w, h) / 128;
						}
						else if (auto normalTexture = material->normal_texture.texture; normalTexture && normalTexture->image)
						{
							auto info = getImageInfo(normalTexture, baseDir);
							if (!info)
								return info.err();

							w = info.value().w, h = info.value().h;
							padding = std::max(w, h) / 128;
						}

						float albedoFactor[4] = {
							material->pbr_metallic_roughness.base_color_factor[0],
							material->pbr_metallic_roughness.base_color_factor[1],
							material->pbr_metallic_roughness.base_color_factor[2],
							material->pbr_metallic_roughness.base_color_factor[3],
						};

						// albedo.
						if (auto albedoTexture = material->pbr_metallic_roughness.base_color_texture.texture; albedoTexture && albedoTexture->image)
						{
							auto rawTexture = processTexture(albedoTexture, baseDir);
							if (!rawTexture)
								return rawTexture.err();


							error err = applyBaseFactor(rawTexture.value(), albedoFactor);
							if (err)
								return err;

							result.albedo.push_back(rawTexture.value());
						}
						else
						{
							aManager::image img{
								.data = {},
								.w = w,
								.h = h,
								.padding = padding,
								.channels = 4,
							};
							img.data.resize(img.w * img.h * img.channels);

							auto ptr = img.data.begin();
							for (int y = 0; y < img.h; y++)
							{
								for (int w = 0; w < img.w; w++)
								{
									ptr[0] = uint8_t(toSRGB(albedoFactor[0]) * 255.0f);
									ptr[1] = uint8_t(toSRGB(albedoFactor[1]) * 255.0f);
									ptr[2] = uint8_t(toSRGB(albedoFactor[2]) * 255.0f);
									ptr[3] = uint8_t(albedoFactor[3] * 255.0f);

									ptr += 4;
								}
							}

							result.albedo.push_back(img);
						}

						// metallic-roughness.
						if (auto metalicRoughnesTexture = material->pbr_metallic_roughness.metallic_roughness_texture.texture; metalicRoughnesTexture && metalicRoughnesTexture->image)
						{
							auto rawTexture = processTexture(metalicRoughnesTexture, baseDir);
							if (!rawTexture)
								return rawTexture.err();

							applyMetallicRoughnessFactor(rawTexture.value(), material->pbr_metallic_roughness.metallic_factor, material->pbr_metallic_roughness.roughness_factor);

							result.metalicRoughnes.push_back(rawTexture.value());
						}
						else
						{
							aManager::image img{
								.data = {},
								.w = w,
								.h = h,
								.padding = padding,
								.channels = 4,
							};
							img.data.resize(img.w * img.h * img.channels);

							auto ptr = img.data.begin();
							for (int y = 0; y < img.h; y++)
							{
								for (int w = 0; w < img.w; w++)
								{
									ptr[0] = 0;
									ptr[1] = uint8_t(toSRGB(material->pbr_metallic_roughness.roughness_factor) * 255.0f);
									ptr[2] = uint8_t(toSRGB(material->pbr_metallic_roughness.metallic_factor) * 255.0f);
									ptr[3] = 0;

									ptr += 4;
								}
							}

							result.metalicRoughnes.push_back(img);
						}

						// normal.
						if (auto normalTexture = material->normal_texture.texture; normalTexture && normalTexture->image)
						{
							auto rawTexture = processTexture(normalTexture, baseDir);
							if (!rawTexture)
								return rawTexture.err();

							result.normal.push_back(rawTexture.value());
						}
						else
						{
							aManager::image img{
								.data = {},
								.w = w,
								.h = h,
								.padding = padding,
								.channels = 4,
							};
							img.data.resize(img.w * img.h * img.channels);

							auto ptr = img.data.begin();
							for (int y = 0; y < img.h; y++)
							{
								for (int w = 0; w < img.w; w++)
								{
									ptr[0] = 0;
									ptr[1] = 0;
									ptr[2] = 255;
									ptr[3] = 0;

									ptr += 4;
								}
							}

							result.normal.push_back(img);
						}
					}
					else
						return error{ "metalicRoughnes texture isn't defined for model: {}", path };
				}

				return result;
			};

		auto generateLodLevel = [&](const std::vector<vertex>& v, const std::vector<uint32_t> i, size_t targetIndexCount) -> error
			{
				std::vector<meshlet> meshlets;
				std::vector<uint32_t> indices;
				std::vector<uint8_t> primitives;

				error err = generateMeshlets(
					v,
					i,
					meshlets,
					primitives,
					indices,
					maxVert,
					maxTriangles,
					coneWieght,
					0.01f,
					targetIndexCount
				);
				if (err)
					return err;

				std::vector<uint32_t> repackedPrimitives = repackPrimitives(primitives, meshlets);

				for (auto& m : meshlets)
				{
					m.indexBufferOffset += uint32_t(result.meshData.index.data->size());
					m.triangleBufferOffset += uint32_t(result.meshData.primitive.data->size());
				}

				result.meshData.index.data->insert(
					result.meshData.index.data->end(),
					std::move_iterator(indices.begin()),
					std::move_iterator(indices.end())
				);

				result.meshData.primitive.data->insert(
					result.meshData.primitive.data->end(),
					std::move_iterator(repackedPrimitives.begin()),
					std::move_iterator(repackedPrimitives.end())
				);

				result.meshData.mesh.data->insert(
					result.meshData.mesh.data->end(),
					std::move_iterator(meshlets.begin()),
					std::move_iterator(meshlets.end())
				);

				return {};
			};

		// For UV recalculation.
		std::vector<uint32_t> vertexToTextureMapping;
		// For tangent calculation and lod calculation.
		std::vector<uint32_t> remappedIndexBuffer;

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

				if (crntPrimitive.indicies.size() == 0 || crntPrimitive.vertecies.size() == 0)
					continue;

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

				// Write indices for later lod level generation.
				for (auto& i : remappedIndex)
					remappedIndexBuffer.push_back(i + uint32_t(result.meshData.vertex->size()));

				// Write material index for later UV remap.
				for (auto _ : remappedVertex)
					vertexToTextureMapping.push_back(uint32_t(mesh.primitives[pri].material - data->materials));

				for (auto& m : meshlets)
				{
					m.indexBufferOffset += uint32_t(result.meshData.index.data->size());
					m.triangleBufferOffset += uint32_t(result.meshData.primitive.data->size());
				}

				for (auto& i : indices)
					i += uint32_t(result.meshData.vertex->size());

				result.meshData.index.data->insert(
					result.meshData.index.data->end(),
					std::move_iterator(indices.begin()),
					std::move_iterator(indices.end())
				);

				result.meshData.primitive.data->insert(
					result.meshData.primitive.data->end(),
					std::move_iterator(repackedPrimitives.begin()),
					std::move_iterator(repackedPrimitives.end())
				);

				result.meshData.mesh.data->insert(
					result.meshData.mesh.data->end(),
					std::move_iterator(meshlets.begin()),
					std::move_iterator(meshlets.end())
				);

				result.meshData.vertex->insert(
					result.meshData.vertex->end(),
					std::move_iterator(remappedVertex.begin()),
					std::move_iterator(remappedVertex.end())
				);
			}
		}

		auto materials = processMaterials(data->materials, int(data->materials_count));
		if (!materials)
			return materials.err();

		auto metalicRoughnesAtlas = makeTextureAtlas(materials.value().metalicRoughnes);
		if (!metalicRoughnesAtlas)
			return metalicRoughnesAtlas.err();

		auto normalAtlas = makeTextureAtlas(materials.value().normal);
		if (!normalAtlas)
			return normalAtlas.err();

		auto albedoAtlas = makeTextureAtlas(materials.value().albedo);
		if (!albedoAtlas)
			return albedoAtlas.err();

		result.mat.textures.albedoAtlas = albedoAtlas.value().first;
		result.mat.textures.normalAtlas = normalAtlas.value().first;
		result.mat.textures.metalicRoughnesAtlas = metalicRoughnesAtlas.value().first;

		for (int i = 0; i < result.meshData.vertex->size(); i++)
		{
			vertex& v = result.meshData.vertex->at(i);
			uint32_t textureIndex = vertexToTextureMapping[i];
			const atlasEntry& e = albedoAtlas.value().second[textureIndex];
			const aManager::image& img = materials.value().albedo[textureIndex];

			float scaledW = img.w * e.downSampleScale;
			float scaledH = img.h * e.downSampleScale;
			float scaledPadding = img.padding * e.downSampleScale;

			v.textureCoords.x = (e.x + scaledPadding + v.textureCoords.x * scaledW) / e.size;
			v.textureCoords.y = (e.y + scaledPadding + v.textureCoords.y * scaledH) / e.size;
		}

		calculateTangents(*result.meshData.vertex.get(), remappedIndexBuffer);
		auto sphere = calculateBoundingSphere(*result.meshData.vertex.get());

		result.meshData.bsCenter = sphere.first;
		result.meshData.bsRadius = sphere.second;

		// Generate lod levels.
		result.meshData.index.second = uint32_t(result.meshData.index.data->size());
		result.meshData.primitive.second = uint32_t(result.meshData.primitive.data->size());
		result.meshData.mesh.second = uint32_t(result.meshData.mesh.data->size());

		error err = generateLodLevel(*result.meshData.vertex.get(), remappedIndexBuffer, remappedIndexBuffer.size() / 2);
		if (err)
			return err;

		result.meshData.index.third = uint32_t(result.meshData.index.data->size());
		result.meshData.primitive.third = uint32_t(result.meshData.primitive.data->size());
		result.meshData.mesh.third = uint32_t(result.meshData.mesh.data->size());

		err = generateLodLevel(*result.meshData.vertex.get(), remappedIndexBuffer, remappedIndexBuffer.size() / 3);
		if (err)
			return err;

		result.meshData.index.fourth = uint32_t(result.meshData.index.data->size());
		result.meshData.primitive.fourth = uint32_t(result.meshData.primitive.data->size());
		result.meshData.mesh.fourth = uint32_t(result.meshData.mesh.data->size());

		err = generateLodLevel(*result.meshData.vertex.get(), remappedIndexBuffer, remappedIndexBuffer.size() / 4);
		if (err)
			return err;

		cgltf_free(data);

		result.meshData.generateHash();

		{
			std::lock_guard l{ mModelMu };

			mLoadedModels.put(key(path), result);
		}

		return result;
	}

	withError<std::pair<std::shared_ptr<texture>, std::map<uint32_t, atlasEntry>>> aManager::makeTextureAtlas(const std::vector<aManager::image>& images)
	{
		if (images.empty())
			return error{ "empty images" };

		std::vector<uint32_t> crcVals;

		for (auto& i : images)
			crcVals.push_back(crc32(i.data.data(), i.w * i.h * i.channels));

		uint32_t mergedCrc = crc32(reinterpret_cast<uint8_t*>(crcVals.data()), crcVals.size() * sizeof(uint32_t));

		{
			std::lock_guard l{ mTexturesMu };

			auto found = mLoadedTextureAtlases.get(mergedCrc);
			if (found)
				return found.value();
		}

		std::pair<std::shared_ptr<texture>, std::map<uint32_t, atlasEntry>> result{ nullptr, {} };

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

		std::vector<uint8_t> atlas(size * size * images.front().channels, 0);

		for (auto& r : rects)
		{
			if (!r.was_packed)
				return error{ "texture wasn't packed" };

			auto img = images[r.id];

			// write main image.
			for (int y = 0; y < img.h; y++)
			{
				uint8_t* src = img.data.data() + y * img.w * img.channels;
				uint8_t* dst = atlas.data() + ((r.y + y + img.padding) * size + r.x + img.padding) * img.channels;

				std::memcpy(dst, src, img.w * img.channels);
			}

			// Write padding.
			auto base = atlas.data();

			// Top.
			for (int i = 0; i < img.w + img.padding * 2; i++)
			{
				int x = r.x + i;
				int y = r.y;

				uint8_t* dst = base + (x + y * size) * img.channels;

				uint8_t* colorPtr = base + (x + (y + img.padding) * size) * img.channels;

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

					dst += size * img.channels;
				}
			}

			// Bottom.
			for (int i = 0; i < img.w + img.padding * 2; i++)
			{
				int x = r.x + i;
				int y = r.y + img.padding + img.h;

				uint8_t* dst = base + (x + y * size) * img.channels;

				uint8_t* colorPtr = base + (x + (y - 1) * size) * img.channels;

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

					dst += size * img.channels;
				}
			}

			// Right.
			for (int i = 0; i < img.h + img.padding * 2; i++)
			{
				int x = r.x + img.padding + img.w;
				int y = r.y + i;

				uint8_t* dst = base + (x + y * size) * img.channels;

				uint8_t* colorPtr = dst - img.channels;

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

					dst += img.channels;
				}
			}

			// Left.
			for (int i = 0; i < img.h + img.padding * 2; i++)
			{
				int x = r.x;
				int y = r.y + i;

				uint8_t* dst = base + (x + y * size) * img.channels;

				uint8_t* colorPtr = dst + img.padding * img.channels;

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

					dst += img.channels;
				}
			}
		}

		const int trashHold = 2048;
		float scale = trashHold / float(size);

		if (scale > 1.0f)
			scale = 1.0f;

		if (scale < 1.0f)
		{
			int newSize = int(scale * size);
			std::vector<uint8_t> downSampled{};
			downSampled.resize(newSize * newSize * 4);

			void* downSampledPtr = stbir_resize_uint8_srgb(
				atlas.data(),
				size,
				size,
				0,
				downSampled.data(),
				newSize,
				newSize,
				0,
				(stbir_pixel_layout)images.front().channels
			);
			if (!downSampledPtr)
				return error{ "can't downsample img" };

			size = newSize;

			atlas = std::move(downSampled);
		}

		for (auto& r : rects)
		{
			result.second[r.id] = atlasEntry{
					.x = int(r.x * scale),
					.y = int(r.y * scale),
					.size = size,
					.downSampleScale = scale,
			};
		}

		auto atlasTexture = makeTexture(atlas.data(), size, size, imageChannel(images.front().channels));
		if (!atlasTexture)
			return atlasTexture.err();

		result.first = atlasTexture.value();

		{
			std::lock_guard l{ mTexturesMu };

			mLoadedTextureAtlases.put(mergedCrc, result);
		}

		return result;
	}
}