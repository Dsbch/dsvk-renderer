#include <pch.h>
#include <cgltf.h>
#include <meshoptimizer.h>
#include <glm/gtc/type_ptr.hpp>

#include "meshletProcessor.h"
#include "primitiveProcessor.h"

namespace engine
{
	template<typename T>
	const T& getAttr(const void* base, size_t idx, size_t stride)
	{
		return *reinterpret_cast<const T*>(
			reinterpret_cast<const char*>(base) + idx * stride
			);
	}

	template<typename T>
	void setAttr(void* base, size_t idx, size_t stride, const T& value)
	{
		*reinterpret_cast<T*>(
			reinterpret_cast<char*>(base) + idx * stride
			) = value;
	}

	void calculateTangents(
		const glm::vec3* positions,
		const glm::vec3* normals,
		const glm::vec2* textCoords,
		size_t verticesLen,
		size_t stride,
		const std::vector<uint32_t>& indices,
		glm::vec4* outTangents
	)
	{
		std::vector<glm::vec3> tan1{};
		tan1.resize(verticesLen);

		std::vector<glm::vec3> tan2{};
		tan2.resize(verticesLen);

		for (size_t i = 0; i < indices.size(); i += 3)
		{
			uint32_t i1 = indices[i];
			uint32_t i2 = indices[i + 1];
			uint32_t i3 = indices[i + 2];

			const glm::vec3& v1 = getAttr<glm::vec3>(positions, i1, stride);
			const glm::vec3& v2 = getAttr<glm::vec3>(positions, i2, stride);
			const glm::vec3& v3 = getAttr<glm::vec3>(positions, i3, stride);
			const glm::vec2& tc1 = getAttr<glm::vec2>(textCoords, i1, stride);
			const glm::vec2& tc2 = getAttr<glm::vec2>(textCoords, i2, stride);
			const glm::vec2& tc3 = getAttr<glm::vec2>(textCoords, i3, stride);

			float x1 = v2.x - v1.x;
			float x2 = v3.x - v1.x;
			float y1 = v2.y - v1.y;
			float y2 = v3.y - v1.y;
			float z1 = v2.z - v1.z;
			float z2 = v3.z - v1.z;

			float s1 = tc2.x - tc1.x;
			float s2 = tc3.x - tc1.x;
			float t1 = tc2.y - tc1.y;
			float t2 = tc3.y - tc1.y;

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

		for (size_t i = 0; i < verticesLen; i++)
		{
			const auto& n = getAttr<glm::vec3>(normals, i, stride);
			const auto& t = tan1[i];

			// Gram-Schmidt orthogonalize.
			glm::vec3 tangent = glm::normalize(t - n * glm::dot(n, t));

			setAttr<glm::vec4>(outTangents, i, stride, glm::vec4(tangent, (glm::dot(glm::cross(n, t), tan2[i]) < 0.0F) ? -1.0F : 1.0F));
		}
	}

	std::vector<uint32_t> repackPrimitives(
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

	std::pair<glm::vec3, float> calculateBoundingSphere(const glm::vec3* positions, size_t verticesLen, size_t stride)
	{
		auto findFarthest = [](glm::vec3 point, const glm::vec3* positions, size_t verticesLen, size_t stride)-> glm::vec3
			{
				glm::vec3 result{ 0.0f };
				float maxLength = 0.0f;

				for (int i = 0; i < verticesLen; i++)
				{
					glm::vec3 pos = getAttr<glm::vec3>(positions, i, stride);

					float length = glm::length(pos - point);

					if (length > maxLength)
					{
						maxLength = length;
						result = pos;
					}
				}

				return result;
			};

		std::pair<glm::vec3, float> result{ glm::vec3(1.0f), 0.0f };

		glm::vec3 first = getAttr<glm::vec3>(positions, std::rand() % verticesLen, stride);

		glm::vec3 second = findFarthest(first, positions, verticesLen, stride);
		glm::vec3 third = findFarthest(second, positions, verticesLen, stride);

		glm::vec3 potentialCenter = (second + third) / 2.0f;
		float potentialRadius = glm::length(third - potentialCenter);

		for (int i = 0; i < verticesLen; i++)
		{
			glm::vec3 pos = getAttr<glm::vec3>(positions, i, stride);

			glm::vec3 toCenter = pos - potentialCenter;
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

	error remapMesh(
		const glm::vec3* positions,
		size_t vertexLen,
		size_t sizeOfVertex,
		const std::vector<uint32_t> indicies,
		const std::function<void* (size_t)>& resizeV,
		const std::function<uint32_t* (size_t)>& resizeI
	)
	{
		std::vector<unsigned int> remap(indicies.size());

		size_t vertex_count = meshopt_generateVertexRemap(
			remap.data(),
			indicies.data(),
			indicies.size(),
			&positions->x,
			vertexLen,
			sizeOfVertex
		);
		if (vertex_count == 0)
			return error{ "vertex count is zero" };

		meshopt_remapIndexBuffer(resizeI(indicies.size()), indicies.data(), indicies.size(), remap.data());

		meshopt_remapVertexBuffer(resizeV(vertex_count), &positions->x, vertexLen, sizeOfVertex, remap.data());

		return {};
	}

	error generateMeshlets(
		const glm::vec3* positions,
		size_t vertexLen,
		size_t sizeOfVertex,
		const std::vector<uint32_t>& indicies,
		std::vector<meshlet>& mOut,
		std::vector<uint8_t>& pOut,
		std::vector<uint32_t>& iOut,
		size_t maxVert, size_t maxTriangles, float coneWieght,
		float errorLevel,
		size_t targetIndexCount,
		uint32_t materialOffset,
		alphaModeType alphaType
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
			&positions->x,
			vertexLen,
			sizeOfVertex,
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
			&positions->x,												// Input: pointer to vertex positions
			vertexLen,													// Input: number of vertex positions	
			sizeOfVertex,												// Input: stride of vertex position elements
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
				&positions->x,
				vertexLen,
				sizeOfVertex
			);

			mOut.push_back(
				meshlet{
					.alphaType = uint32_t(alphaType),
					.localMaterialOffset = materialOffset,
					.indexBufferIndex = 0,
					.indexBufferOffset = m.vertex_offset,
					.vertexBufferIndex = 0,
					.vertexBufferOffset = 0,
					.vertexCount = m.vertex_count,
					.triangleBufferIndex = 0,
					.triangleBufferOffset = m.triangle_offset,
					.triangleCount = m.triangle_count,
					.perMeshBufferOffset = 0,
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

	error generateLodLevel(
		const glm::vec3* positions,
		size_t vertexLen,
		size_t sizeOfVertex,
		const std::vector<uint32_t> i,
		std::vector<meshlet>& meshletsOut,
		std::vector<uint32_t>& indicesOut,
		std::vector<uint32_t>& repackedPrimitivesOut,
		size_t targetIndexCount,
		size_t maxVert,
		size_t maxTriangles,
		float coneWeight,
		float errorLevel,
		uint32_t materialOffset,
		alphaModeType alphaMode
	)
	{
		std::vector<meshlet> meshlets;
		std::vector<uint32_t> indices;
		std::vector<uint8_t> primitives;

		error err = generateMeshlets(
			positions,
			vertexLen,
			sizeOfVertex,
			i,
			meshlets,
			primitives,
			indices,
			maxVert,
			maxTriangles,
			coneWeight,
			errorLevel,
			targetIndexCount,
			materialOffset,
			alphaMode
		);
		if (err)
			return err;

		std::vector<uint32_t> repackedPrimitives = repackPrimitives(primitives, meshlets);

		indicesOut.insert(
			indicesOut.end(),
			std::move_iterator(indices.begin()),
			std::move_iterator(indices.end())
		);

		repackedPrimitivesOut.insert(
			repackedPrimitivesOut.end(),
			std::move_iterator(repackedPrimitives.begin()),
			std::move_iterator(repackedPrimitives.end())
		);

		meshletsOut.insert(
			meshletsOut.end(),
			std::move_iterator(meshlets.begin()),
			std::move_iterator(meshlets.end())
		);

		return {};
	}

	withError<std::pair<std::vector<mesh>, std::vector<perMeshAttributes>>> proccessMeshes(const cgltf_data* data, size_t maxVert, size_t maxTriangles, float coneWeight, float errorLevel)
	{
		auto addLodLevels = [](
			mesh& crntMesh,
			std::vector<meshlet>& meshletLod1,
			std::vector<meshlet>& meshletLod2,
			std::vector<meshlet>& meshletLod3,
			std::vector<uint32_t>& indicesLod1,
			std::vector<uint32_t>& indicesLod2,
			std::vector<uint32_t>& indicesLod3,
			std::vector<uint32_t>& primitivesLod1,
			std::vector<uint32_t>& primitivesLod2,
			std::vector<uint32_t>& primitivesLod3
			)
			{
				for (auto& m : meshletLod1)
				{
					m.indexBufferOffset += uint32_t(crntMesh.indices.data.size());
					m.triangleBufferOffset += uint32_t(crntMesh.primitives.data.size());
				}

				crntMesh.indices.second = uint32_t(crntMesh.indices.data.size());
				crntMesh.primitives.second = uint32_t(crntMesh.primitives.data.size());
				crntMesh.meshlets.second = uint32_t(crntMesh.meshlets.data.size());

				crntMesh.indices.data.insert(
					crntMesh.indices.data.end(),
					std::move_iterator(indicesLod1.begin()),
					std::move_iterator(indicesLod1.end())
				);

				crntMesh.primitives.data.insert(
					crntMesh.primitives.data.end(),
					std::move_iterator(primitivesLod1.begin()),
					std::move_iterator(primitivesLod1.end())
				);

				crntMesh.meshlets.data.insert(
					crntMesh.meshlets.data.end(),
					std::move_iterator(meshletLod1.begin()),
					std::move_iterator(meshletLod1.end())
				);

				for (auto& m : meshletLod2)
				{
					m.indexBufferOffset += uint32_t(crntMesh.indices.data.size());
					m.triangleBufferOffset += uint32_t(crntMesh.primitives.data.size());
				}

				crntMesh.indices.third = uint32_t(crntMesh.indices.data.size());
				crntMesh.primitives.third = uint32_t(crntMesh.primitives.data.size());
				crntMesh.meshlets.third = uint32_t(crntMesh.meshlets.data.size());

				crntMesh.indices.data.insert(
					crntMesh.indices.data.end(),
					std::move_iterator(indicesLod2.begin()),
					std::move_iterator(indicesLod2.end())
				);

				crntMesh.primitives.data.insert(
					crntMesh.primitives.data.end(),
					std::move_iterator(primitivesLod2.begin()),
					std::move_iterator(primitivesLod2.end())
				);

				crntMesh.meshlets.data.insert(
					crntMesh.meshlets.data.end(),
					std::move_iterator(meshletLod2.begin()),
					std::move_iterator(meshletLod2.end())
				);

				for (auto& m : meshletLod3)
				{
					m.indexBufferOffset += uint32_t(crntMesh.indices.data.size());
					m.triangleBufferOffset += uint32_t(crntMesh.primitives.data.size());
				}

				crntMesh.indices.fourth = uint32_t(crntMesh.indices.data.size());
				crntMesh.primitives.fourth = uint32_t(crntMesh.primitives.data.size());
				crntMesh.meshlets.fourth = uint32_t(crntMesh.meshlets.data.size());

				crntMesh.indices.data.insert(
					crntMesh.indices.data.end(),
					std::move_iterator(indicesLod3.begin()),
					std::move_iterator(indicesLod3.end())
				);

				crntMesh.primitives.data.insert(
					crntMesh.primitives.data.end(),
					std::move_iterator(primitivesLod3.begin()),
					std::move_iterator(primitivesLod3.end())
				);

				crntMesh.meshlets.data.insert(
					crntMesh.meshlets.data.end(),
					std::move_iterator(meshletLod3.begin()),
					std::move_iterator(meshletLod3.end())
				);
			};

		std::pair<std::vector<mesh>, std::vector<perMeshAttributes>> result{};

		for (size_t ni = 0; ni < data->nodes_count; ++ni)
		{
			const cgltf_node* node = &data->nodes[ni];
			if (!node->mesh)
				continue;

			const cgltf_mesh& gtlfMesh = *node->mesh;

			mesh crntMesh = {};

			perMeshAttributes crntMeshAttrs = {};
			crntMeshAttrs.meshGlobalTransform = getNodeWorldTransformMat4(node);
			crntMeshAttrs.meshLocalTransform = getNodeLocalTransformMat4(node);
			crntMeshAttrs.meshGlobalNormal = glm::transpose(glm::inverse(glm::mat3(crntMeshAttrs.meshGlobalTransform)));
			crntMeshAttrs.meshLocalNormal = glm::transpose(glm::inverse(glm::mat3(crntMeshAttrs.meshLocalTransform)));
			crntMeshAttrs.isSkinned = uint32_t(node->skin != nullptr);

			// For lod levels.
			std::vector<meshlet> meshletLod1{};
			std::vector<uint32_t> indicesLod1{};
			std::vector<uint32_t> primitivesLod1{};

			std::vector<meshlet> meshletLod2{};
			std::vector<uint32_t> indicesLod2{};
			std::vector<uint32_t> primitivesLod2{};

			std::vector<meshlet> meshletLod3{};
			std::vector<uint32_t> indicesLod3{};
			std::vector<uint32_t> primitivesLod3{};

			for (size_t pri = 0; pri < gtlfMesh.primitives_count; ++pri)
			{
				primitive crntPrimitive = processPrimitive(
					gtlfMesh.primitives[pri],
					crntMeshAttrs.isSkinned
				);

				if (crntPrimitive.indicies.size() == 0 || (crntPrimitive.vertecies.size() == 0 && crntPrimitive.animVertecies.size() == 0))
					continue;

				const glm::vec3* positions = crntMeshAttrs.isSkinned ? &crntPrimitive.animVertecies.front().vert.position : &crntPrimitive.vertecies.front().position;
				const glm::vec2* textCoords = crntMeshAttrs.isSkinned ? &crntPrimitive.animVertecies.front().vert.textureCoords : &crntPrimitive.vertecies.front().textureCoords;
				const glm::vec3* normals = crntMeshAttrs.isSkinned ? &crntPrimitive.animVertecies.front().vert.normal : &crntPrimitive.vertecies.front().normal;
				glm::vec4* tangents = crntMeshAttrs.isSkinned ? &crntPrimitive.animVertecies.front().vert.tangent : &crntPrimitive.vertecies.front().tangent;
				size_t sizeOfVertex = crntMeshAttrs.isSkinned ? sizeof(animVertex) : sizeof(vertex);
				size_t vertexLen = crntMeshAttrs.isSkinned ? crntPrimitive.animVertecies.size() : crntPrimitive.vertecies.size();

				calculateTangents(positions, normals, textCoords, vertexLen, sizeOfVertex, crntPrimitive.indicies, tangents);

				std::vector<vertex> remappedVertex;
				std::vector<animVertex> remappedAnimVertex;
				std::vector<uint32_t> remappedIndex;

				void* vOut = crntMeshAttrs.isSkinned ? static_cast<void*>(remappedAnimVertex.data()) : static_cast<void*>(remappedVertex.data());
				auto resizeV = [&remappedVertex, &remappedAnimVertex, crntMeshAttrs](size_t size) -> void*
					{
						if (crntMeshAttrs.isSkinned)
						{
							remappedAnimVertex.resize(size);

							return static_cast<void*>(remappedAnimVertex.data());
						}
						else
						{
							remappedVertex.resize(size);

							return static_cast<void*>(remappedVertex.data());
						}
					};
				auto resizeI = [&remappedIndex](size_t size) -> uint32_t*
					{
						remappedIndex.resize(size);

						return remappedIndex.data();
					};

				error err = remapMesh(
					positions,
					vertexLen,
					sizeOfVertex,
					crntPrimitive.indicies,
					resizeV,
					resizeI
				);
				if (err)
					return err;

				alphaModeType alphaMode = alphaModeType::opaque;

				switch (gtlfMesh.primitives[pri].material->alpha_mode)
				{
				case cgltf_alpha_mode_opaque:
					alphaMode = alphaModeType::opaque;
					break;
				case cgltf_alpha_mode_blend:
					alphaMode = alphaModeType::blend;
					break;
				case cgltf_alpha_mode_mask:
					alphaMode = alphaModeType::mask;
					break;
				}

				std::vector<meshlet> meshlets;
				std::vector<uint32_t> indices;
				std::vector<uint8_t> primitives;

				positions = crntMeshAttrs.isSkinned ? &remappedAnimVertex.front().vert.position : &remappedVertex.front().position;
				sizeOfVertex = crntMeshAttrs.isSkinned ? sizeof(animVertex) : sizeof(vertex);
				vertexLen = crntMeshAttrs.isSkinned ? remappedAnimVertex.size() : remappedVertex.size();

				err = generateMeshlets(
					positions,
					vertexLen,
					sizeOfVertex,
					remappedIndex,
					meshlets,
					primitives,
					indices,
					maxVert,
					maxTriangles,
					coneWeight,
					0,
					0,
					uint32_t(gtlfMesh.primitives[pri].material - data->materials),
					alphaMode
				);
				if (err)
					return err;

				std::vector<uint32_t> repackedPrimitives = repackPrimitives(primitives, meshlets);

				for (auto& m : meshlets)
				{
					m.indexBufferOffset += uint32_t(crntMesh.indices.data.size());
					m.triangleBufferOffset += uint32_t(crntMesh.primitives.data.size());
				}

				size_t vertexBase = crntMeshAttrs.isSkinned ?
					crntMesh.animVertices.size() : crntMesh.vertices.size();

				for (auto& i : indices)
					i += uint32_t(vertexBase);

				crntMesh.indices.data.insert(
					crntMesh.indices.data.end(),
					std::move_iterator(indices.begin()),
					std::move_iterator(indices.end())
				);

				crntMesh.primitives.data.insert(
					crntMesh.primitives.data.end(),
					std::move_iterator(repackedPrimitives.begin()),
					std::move_iterator(repackedPrimitives.end())
				);

				crntMesh.meshlets.data.insert(
					crntMesh.meshlets.data.end(),
					std::move_iterator(meshlets.begin()),
					std::move_iterator(meshlets.end())
				);

				crntMesh.vertices.insert(
					crntMesh.vertices.end(),
					std::move_iterator(remappedVertex.begin()),
					std::move_iterator(remappedVertex.end())
				);

				crntMesh.animVertices.insert(
					crntMesh.animVertices.end(),
					std::move_iterator(remappedAnimVertex.begin()),
					std::move_iterator(remappedAnimVertex.end())
				);

				std::vector<uint32_t> crntLodIndices{};

				// LOD1
				size_t meshletLod1Before = meshletLod1.size();
				size_t indexLod1Before = indicesLod1.size();
				size_t primLod1Before = primitivesLod1.size();

				err = generateLodLevel(positions, vertexLen, sizeOfVertex, remappedIndex, meshletLod1, crntLodIndices, primitivesLod1, remappedIndex.size() / 2, maxVert, maxTriangles, coneWeight, errorLevel, uint32_t(gtlfMesh.primitives[pri].material - data->materials), alphaMode);
				if (err) return err;

				for (size_t m = meshletLod1Before; m < meshletLod1.size(); m++) 
				{
					meshletLod1[m].indexBufferOffset += uint32_t(indexLod1Before);
					meshletLod1[m].triangleBufferOffset += uint32_t(primLod1Before);
				}
				for (auto& i : crntLodIndices) i += uint32_t(vertexBase);
				indicesLod1.insert(indicesLod1.end(), std::move_iterator(crntLodIndices.begin()), std::move_iterator(crntLodIndices.end()));
				crntLodIndices.clear();

				// LOD2
				size_t meshletLod2Before = meshletLod2.size();
				size_t indexLod2Before = indicesLod2.size();
				size_t primLod2Before = primitivesLod2.size();

				err = generateLodLevel(positions, vertexLen, sizeOfVertex, remappedIndex, meshletLod2, crntLodIndices, primitivesLod2, remappedIndex.size() / 3, maxVert, maxTriangles, coneWeight, errorLevel, uint32_t(gtlfMesh.primitives[pri].material - data->materials), alphaMode);
				if (err) return err;

				for (size_t m = meshletLod2Before; m < meshletLod2.size(); m++) 
				{
					meshletLod2[m].indexBufferOffset += uint32_t(indexLod2Before);
					meshletLod2[m].triangleBufferOffset += uint32_t(primLod2Before);
				}
				for (auto& i : crntLodIndices) i += uint32_t(vertexBase);
				indicesLod2.insert(indicesLod2.end(), std::move_iterator(crntLodIndices.begin()), std::move_iterator(crntLodIndices.end()));
				crntLodIndices.clear();

				// LOD3
				size_t meshletLod3Before = meshletLod3.size();
				size_t indexLod3Before = indicesLod3.size();
				size_t primLod3Before = primitivesLod3.size();

				err = generateLodLevel(positions, vertexLen, sizeOfVertex, remappedIndex, meshletLod3, crntLodIndices, primitivesLod3, remappedIndex.size() / 4, maxVert, maxTriangles, coneWeight, errorLevel, uint32_t(gtlfMesh.primitives[pri].material - data->materials), alphaMode);
				if (err) return err;

				for (size_t m = meshletLod3Before; m < meshletLod3.size(); m++) 
				{
					meshletLod3[m].indexBufferOffset += uint32_t(indexLod3Before);
					meshletLod3[m].triangleBufferOffset += uint32_t(primLod3Before);
				}
				for (auto& i : crntLodIndices) i += uint32_t(vertexBase);
				indicesLod3.insert(indicesLod3.end(), std::move_iterator(crntLodIndices.begin()), std::move_iterator(crntLodIndices.end()));
				crntLodIndices.clear();
			}

			const glm::vec3* positions = crntMeshAttrs.isSkinned ? &crntMesh.animVertices.front().vert.position : &crntMesh.vertices.front().position;
			size_t sizeOfVertex = crntMeshAttrs.isSkinned ? sizeof(animVertex) : sizeof(vertex);
			size_t vertexLen = crntMeshAttrs.isSkinned ? crntMesh.animVertices.size() : crntMesh.vertices.size();

			auto sphere = calculateBoundingSphere(positions, vertexLen, sizeOfVertex);

			crntMeshAttrs.bsCenter = sphere.first;
			crntMeshAttrs.bsRadius = sphere.second;

			addLodLevels(crntMesh, meshletLod1, meshletLod2, meshletLod3, indicesLod1, indicesLod2, indicesLod3, primitivesLod1, primitivesLod2, primitivesLod3);

			crntMesh.generateHashes();

			result.first.push_back(std::move(crntMesh));
			result.second.push_back(std::move(crntMeshAttrs));
		}

		return result;
	}

	std::pair<std::vector<animation>, std::vector<skin>> proccessAnimations(const cgltf_data* data)
	{
		auto animType = [](cgltf_animation_path_type type) -> animationType
			{
				switch (type)
				{
				case cgltf_animation_path_type_translation:
					return tr;
				case cgltf_animation_path_type_rotation:
					return rt;
				case cgltf_animation_path_type_scale:
					return sc;
				default:
					return sc;
				}
			};

		auto interType = [](cgltf_interpolation_type type) -> interpolationType
			{
				switch (type)
				{
				case cgltf_interpolation_type_linear:
					return linear;
				case cgltf_interpolation_type_cubic_spline:
					return cubicspline;
				case cgltf_interpolation_type_step:
					return step;
				default:
					return linear;
				}
			};

		auto getInverseBindForNode = [](const cgltf_skin* skin, const cgltf_node* node) -> glm::mat4
			{
				glm::mat4 inverseMat{ 1.0f };

				for (int i = 0; i < skin->joints_count; i++)
				{
					if (skin->joints[i] == node)
					{
						cgltf_accessor_read_float(skin->inverse_bind_matrices, i, glm::value_ptr(inverseMat), 16);
						break;
					}
				}

				return inverseMat;
			};

		std::function<void(const cgltf_node* root, const cgltf_skin* s, const cgltf_data* data, skin& result, int parentIdx, std::map<const cgltf_node*, std::pair<size_t, size_t>>& nodeToJoint)> processSkinNode;
		processSkinNode = [&](const cgltf_node* root, const cgltf_skin* s, const cgltf_data* data, skin& result, int parentIdx, std::map<const cgltf_node*, std::pair<size_t, size_t>>& nodeToJoint)
			{
				joint j{};

				j.localTransform = getNodeLocalTransform(root);
				j.inverseBind = getInverseBindForNode(s, root);
				j.isSkinJoint = std::any_of(s->joints, s->joints + s->joints_count,
					[root](const cgltf_node* n) { return n == root; });

				size_t jointIdx = result.skinJoints.size();
				nodeToJoint[root] = { s - data->skins, jointIdx };
				result.skinJoints.push_back(j);

				if (parentIdx >= 0)
					result.skinJoints[jointIdx].parentIdx = parentIdx;
				else
					result.skinJoints[jointIdx].parentIdx = -1;

				for (size_t i = 0; i < root->children_count; i++)
					processSkinNode(root->children[i], s, data, result, int(jointIdx), nodeToJoint);
			};

		std::pair<std::vector<animation>, std::vector<skin>> result{};

		std::map<const cgltf_node*, std::pair<size_t, size_t>> nodeToJoint{};

		for (int i = 0; i < data->nodes_count; i++)
		{
			auto sk = data->nodes[i].skin;

			if (sk)
			{
				std::map<const cgltf_node*, const cgltf_node*> childToParent{};
				for (int i = 0; i < sk->joints_count; i++)
				{
					for (int c = 0; c < sk->joints[i]->children_count; c++)
					{
						childToParent[sk->joints[i]->children[c]] = sk->joints[i];
					}
				}

				cgltf_node* root = nullptr;

				for (auto& [k, v] : childToParent)
				{
					if (childToParent.find(v) == childToParent.end())
					{
						root = const_cast<cgltf_node*>(v);
						break;
					}
				}

				if (root)
				{
					skin crntSkin{};

					processSkinNode(root, sk, data, crntSkin, -1, nodeToJoint);

					result.second.push_back(crntSkin);
				}
			}
		}

		for (int i = 0; i < data->animations_count; i++)
		{
			animation crntAnim{
				.name = std::string(data->animations[i].name),
			};

			for (int c = 0; c < data->animations[i].channels_count; c++)
			{
				cgltf_animation_sampler* sampler = data->animations[i].channels[c].sampler;
				cgltf_animation_channel ch = data->animations[i].channels[c];

				if (!sampler)
					continue;

				channel ac{
					.aType = animType(ch.target_path),
					.iType = interpolationType(sampler->interpolation),
					.currentTimeStamp = 0.0f,
				};

				ac.skinIndex = nodeToJoint[ch.target_node].first;
				ac.jointIndex = nodeToJoint[ch.target_node].second;

				std::vector<float> ts{};
				std::vector<transform> trs{};

				size_t frameCount = sampler->input->count;
				ts.resize(frameCount);

				for (size_t k = 0; k < frameCount; k++)
					cgltf_accessor_read_float(sampler->input, k, &ts[k], 1);

				trs.resize(frameCount);

				for (size_t k = 0; k < frameCount; k++)
				{
					transform keyframe{
						.scale = glm::vec3{1.0f},
					};

					float val[4];
					if (ac.aType == rt)
					{
						cgltf_accessor_read_float(sampler->output, k, val, 4);
						keyframe.rotation = glm::quat(val[3], val[0], val[1], val[2]);
					}
					else if (ac.aType == tr)
					{
						cgltf_accessor_read_float(sampler->output, k, val, 3);
						keyframe.translation = glm::make_vec3(val);
					}
					else if (ac.aType == sc)
					{
						cgltf_accessor_read_float(sampler->output, k, val, 3);
						keyframe.scale = glm::make_vec3(val);
					}

					trs[k] = std::move(keyframe);
				}

				ac.timestamps = std::make_shared<std::vector<float>>(std::move(ts));
				ac.keyframes = std::make_shared<std::vector<transform>>(std::move(trs));

				crntAnim.channels.push_back(std::move(ac));
			}

			result.first.push_back(std::move(crntAnim));
		}

		return result;
	}
}
