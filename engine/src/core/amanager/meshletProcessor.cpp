#include <pch.h>
#include <cgltf.h>
#include <meshoptimizer.h>
#include <glm/gtc/type_ptr.hpp>

#include "meshletProcessor.h"

namespace engine
{
	std::vector<glm::vec4> calculateTangents(
		const std::vector<glm::vec4>& positions,
		const std::vector<glm::vec4>& normals,
		const std::vector<uint32_t>& indices
	)
	{
		std::vector<glm::vec3> tan1{};
		tan1.resize(positions.size());

		std::vector<glm::vec3> tan2{};
		tan2.resize(positions.size());

		for (size_t i = 0; i < indices.size(); i += 3)
		{
			uint32_t i1 = indices[i];
			uint32_t i2 = indices[i + 1];
			uint32_t i3 = indices[i + 2];

			const glm::vec3& v1 = glm::vec3{ positions[i1] };
			const glm::vec3& v2 = glm::vec3{ positions[i2] };
			const glm::vec3& v3 = glm::vec3{ positions[i3] };
			const glm::vec2& tc1 = glm::vec2{ positions[i1].w, normals[i1].w };
			const glm::vec2& tc2 = glm::vec2{ positions[i2].w, normals[i2].w };
			const glm::vec2& tc3 = glm::vec2{ positions[i3].w, normals[i3].w };

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

		std::vector<glm::vec4> result{};

		for (size_t i = 0; i < positions.size(); i++)
		{
			const auto& n = glm::vec3{ normals[i] };
			const auto& t = tan1[i];

			// Gram-Schmidt orthogonalize.
			glm::vec3 tangent = glm::normalize(t - n * glm::dot(n, t));

			result.push_back(glm::vec4(tangent, (glm::dot(glm::cross(n, t), tan2[i]) < 0.0F) ? -1.0F : 1.0F));
		}

		return result;
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

	std::pair<glm::vec3, float> calculateBoundingSphere(const std::vector<glm::vec3>& positions)
	{
		auto findFarthest = [](glm::vec3 point, const std::vector<glm::vec3>& positions)-> glm::vec3
			{
				glm::vec3 result{ 0.0f };
				float maxLength = 0.0f;

				for (int i = 0; i < positions.size(); i++)
				{
					glm::vec3 pos = glm::vec3{ positions[i] };

					float length = glm::length(pos - point);

					if (length > maxLength)
					{
						maxLength = length;
						result = pos;
					}
				}

				return result;
			};

		if (positions.size() == 0)
			return { glm::vec3{0.0f}, 0.0f };

		glm::vec3 first = glm::vec3{ positions[std::rand() % positions.size()] };

		glm::vec3 second = findFarthest(first, positions);
		glm::vec3 third = findFarthest(second, positions);

		glm::vec3 potentialCenter = (second + third) / 2.0f;
		float potentialRadius = glm::length(third - potentialCenter);

		for (int i = 0; i < positions.size(); i++)
		{
			glm::vec3 pos = glm::vec3{ positions[i] };

			glm::vec3 toCenter = pos - potentialCenter;
			float crntRadius = glm::length(toCenter);

			if (crntRadius > potentialRadius)
			{
				float newRadius = potentialRadius + (crntRadius - potentialRadius) / 2.0f;
				potentialCenter += toCenter - (toCenter * (newRadius / crntRadius));
				potentialRadius = newRadius;
			}
		}

		return { potentialCenter, potentialRadius };
	}

	std::pair<glm::vec3, float> calculateBoundingSphere(const std::vector<glm::vec4>& positions)
	{
		auto findFarthest = [](glm::vec3 point, const std::vector<glm::vec4>& positions)-> glm::vec3
			{
				glm::vec3 result{ 0.0f };
				float maxLength = 0.0f;

				for (int i = 0; i < positions.size(); i++)
				{
					glm::vec3 pos = glm::vec3{ positions[i] };

					float length = glm::length(pos - point);

					if (length > maxLength)
					{
						maxLength = length;
						result = pos;
					}
				}

				return result;
			};

		glm::vec3 first = glm::vec3{ positions[std::rand() % positions.size()] };

		glm::vec3 second = findFarthest(first, positions);
		glm::vec3 third = findFarthest(second, positions);

		glm::vec3 potentialCenter = (second + third) / 2.0f;
		float potentialRadius = glm::length(third - potentialCenter);

		for (int i = 0; i < positions.size(); i++)
		{
			glm::vec3 pos = glm::vec3{ positions[i] };

			glm::vec3 toCenter = pos - potentialCenter;
			float crntRadius = glm::length(toCenter);

			if (crntRadius > potentialRadius)
			{
				float newRadius = potentialRadius + (crntRadius - potentialRadius) / 2.0f;
				potentialCenter += toCenter - (toCenter * (newRadius / crntRadius));
				potentialRadius = newRadius;
			}
		}

		return { potentialCenter, potentialRadius };
	}

	error remapMesh(primitive& prim, bool isSkinned)
	{
		std::vector<meshopt_Stream> streams;

		streams.push_back({ prim.positions.data(), sizeof(glm::vec4), sizeof(glm::vec4) });
		streams.push_back({ prim.normal.data(),    sizeof(glm::vec4), sizeof(glm::vec4) });

		if (!prim.jointIndices.empty())
			streams.push_back({ prim.jointIndices.data(), sizeof(glm::uvec4), sizeof(glm::uvec4) });

		if (!prim.weights.empty())
			streams.push_back({ prim.weights.data(), sizeof(glm::vec4), sizeof(glm::vec4) });

		std::vector<uint32_t> remap(prim.indicies.size());

		size_t vertexCount = meshopt_generateVertexRemapMulti(
			remap.data(),
			prim.indicies.data(),
			prim.indicies.size(),
			prim.positions.size(),
			streams.data(),
			streams.size()
		);

		if (vertexCount == 0)
			return error{ "vertex count is zero" };

		std::vector<uint32_t>    remappedIndices(prim.indicies.size());
		std::vector<glm::vec4>   remappedPositions(vertexCount);
		std::vector<glm::vec4>   remappedNormals(vertexCount);
		std::vector<glm::uvec4>  remappedJoints{};
		std::vector<glm::vec4>   remappedWeights{};

		meshopt_remapIndexBuffer(remappedIndices.data(), prim.indicies.data(), prim.indicies.size(), remap.data());
		meshopt_remapVertexBuffer(remappedPositions.data(), prim.positions.data(), prim.positions.size(), sizeof(glm::vec4), remap.data());
		meshopt_remapVertexBuffer(remappedNormals.data(), prim.normal.data(), prim.normal.size(), sizeof(glm::vec4), remap.data());

		if (isSkinned && !prim.jointIndices.empty())
		{
			remappedJoints.resize(vertexCount);
			meshopt_remapVertexBuffer(remappedJoints.data(), prim.jointIndices.data(), prim.jointIndices.size(), sizeof(glm::uvec4), remap.data());
		}

		if (isSkinned && !prim.weights.empty())
		{
			remappedWeights.resize(vertexCount);
			meshopt_remapVertexBuffer(remappedWeights.data(), prim.weights.data(), prim.weights.size(), sizeof(glm::vec4), remap.data());
		}

		prim.indicies = std::move(remappedIndices);
		prim.positions = std::move(remappedPositions);
		prim.normal = std::move(remappedNormals);
		prim.jointIndices = std::move(remappedJoints);
		prim.weights = std::move(remappedWeights);

		return {};
	}

	error generateMeshlets(
		const std::vector<glm::vec4>& positions,
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
			&positions[0].x,
			positions.size(),
			sizeof(glm::vec4),
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
			&positions[0].x,											// Input: pointer to vertex positions
			positions.size(),											// Input: number of vertex positions	
			sizeof(glm::vec4),											// Input: stride of vertex position elements
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
				&positions[0].x,
				positions.size(),
				sizeof(glm::vec4)
			);

			mOut.push_back(
				meshlet{
					.alphaType = uint32_t(alphaType),
					.localMaterialOffset = materialOffset,
					.indexBufferIndex = 0,
					.indexBufferOffset = m.vertex_offset,
					.weightBufferOffset = 0,
					.weightBufferIndex = 0,
					.vertexBufferIndex = 0,
					.vertexBufferOffset = 0,
					.vertexCount = m.vertex_count,
					.triangleBufferIndex = 0,
					.triangleBufferOffset = m.triangle_offset,
					.triangleCount = m.triangle_count,
					.perMeshBufferIndex = 0,
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
		const std::vector<glm::vec4>& positions,
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
		std::map<cgltf_skin*, std::pair<uint32_t, uint32_t>> skinOffsets{};

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

			uint32_t jointOffset = 0;
			if (crntMeshAttrs.isSkinned && skinOffsets.size() != 0)
			{
				if (auto found = skinOffsets.find(node->skin); found != skinOffsets.end())
				{
					jointOffset = found->second.first;
				}
				else
				{
					for (auto [_, v] : skinOffsets)
						jointOffset += v.second;
				}
			}

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
				auto crntPrimitive = processPrimitive(
					gtlfMesh.primitives[pri],
					crntMeshAttrs.isSkinned,
					jointOffset
				);
				if (!crntPrimitive)
					return crntPrimitive.err();

				uint32_t primitiveMaterialOffset = uint32_t(gtlfMesh.primitives[pri].material - data->materials);

				error err = remapMesh(
					crntPrimitive.value(),
					crntMeshAttrs.isSkinned
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

				err = generateMeshlets(
					crntPrimitive.value().positions,
					crntPrimitive.value().indicies,
					meshlets,
					primitives,
					indices,
					maxVert,
					maxTriangles,
					coneWeight,
					0,
					0,
					primitiveMaterialOffset,
					alphaMode
				);
				if (err)
					return err;

				std::vector<glm::vec4> tangents = calculateTangents(crntPrimitive.value().positions, crntPrimitive.value().normal, crntPrimitive.value().indicies);
				std::vector<uint32_t> repackedPrimitives = repackPrimitives(primitives, meshlets);

				for (auto& m : meshlets)
				{
					m.indexBufferOffset += uint32_t(crntMesh.indices.data.size());
					m.triangleBufferOffset += uint32_t(crntMesh.primitives.data.size());
				}

				size_t vertexBase = crntMesh.positions.size();

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

				crntMesh.positions.insert(
					crntMesh.positions.end(),
					std::move_iterator(crntPrimitive.value().positions.begin()),
					std::move_iterator(crntPrimitive.value().positions.end())
				);

				crntMesh.tangent.insert(
					crntMesh.tangent.end(),
					std::move_iterator(tangents.begin()),
					std::move_iterator(tangents.end())
				);

				crntMesh.normal.insert(
					crntMesh.normal.end(),
					std::move_iterator(crntPrimitive.value().normal.begin()),
					std::move_iterator(crntPrimitive.value().normal.end())
				);

				if (crntMeshAttrs.isSkinned)
				{
					crntMesh.weights.insert(
						crntMesh.weights.end(),
						std::move_iterator(crntPrimitive.value().weights.begin()),
						std::move_iterator(crntPrimitive.value().weights.end())
					);

					crntMesh.jointIndices.insert(
						crntMesh.jointIndices.end(),
						std::move_iterator(crntPrimitive.value().jointIndices.begin()),
						std::move_iterator(crntPrimitive.value().jointIndices.end())
					);
				}

				std::vector<uint32_t> crntLodIndices{};

				// LOD1
				size_t meshletLod1Before = meshletLod1.size();
				size_t indexLod1Before = indicesLod1.size();
				size_t primLod1Before = primitivesLod1.size();

				err = generateLodLevel(crntPrimitive.value().positions, crntPrimitive.value().indicies, meshletLod1, crntLodIndices, primitivesLod1, crntPrimitive.value().indicies.size() / 2, maxVert, maxTriangles, coneWeight, errorLevel, primitiveMaterialOffset, alphaMode);
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

				err = generateLodLevel(crntPrimitive.value().positions, crntPrimitive.value().indicies, meshletLod2, crntLodIndices, primitivesLod2, crntPrimitive.value().indicies.size() / 3, maxVert, maxTriangles, coneWeight, errorLevel, primitiveMaterialOffset, alphaMode);
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

				err = generateLodLevel(crntPrimitive.value().positions, crntPrimitive.value().indicies, meshletLod3, crntLodIndices, primitivesLod3, crntPrimitive.value().indicies.size() / 4, maxVert, maxTriangles, coneWeight, errorLevel, primitiveMaterialOffset, alphaMode);
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

			auto [center, radius] = calculateBoundingSphere(crntMesh.positions);

			crntMeshAttrs.bsCenter = center;
			crntMeshAttrs.bsRadius = radius;

			addLodLevels(crntMesh, meshletLod1, meshletLod2, meshletLod3, indicesLod1, indicesLod2, indicesLod3, primitivesLod1, primitivesLod2, primitivesLod3);

			crntMesh.generateHashes();

			result.first.push_back(std::move(crntMesh));
			result.second.push_back(std::move(crntMeshAttrs));

			if (crntMeshAttrs.isSkinned)
				skinOffsets[node->skin] = { jointOffset, uint32_t(node->skin->joints_count) };
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

		if (data->skins_count == 0)
			return result;

		std::map<const cgltf_node*, std::pair<size_t, size_t>> nodeToJoint{};

		for (int i = 0; i < data->skins_count; i++)
		{
			const cgltf_skin* sk = &data->skins[i];

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

		for (int i = 0; i < data->animations_count; i++)
		{
			animation crntAnim{
				.name = std::string(data->animations[i].name),
			};

			for (int c = 0; c < data->animations[i].channels_count; c++)
			{
				cgltf_animation_sampler* sampler = data->animations[i].channels[c].sampler;
				cgltf_animation_channel ch = data->animations[i].channels[c];

				auto it = nodeToJoint.find(ch.target_node);
				if (it == nodeToJoint.end())
					continue;

				channel ac{
					.aType = animType(ch.target_path),
					.iType = interpolationType(sampler->interpolation),
					.currentTimeStamp = 0.0f,
					.skinIndex = it->second.first,
					.jointIndex = it->second.second,
				};

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

	void recalculateMeshletBounds(mesh& m, perMeshAttributes& attrs, std::vector<animation> anims, std::vector<skin> skins)
	{
		auto skinMesh =
			[](
				const mesh& m,
				const std::vector<glm::mat4>& jointMatrices,
				std::vector<std::pair<glm::vec3, glm::vec3>>& sPosition
				)
			{
				const size_t numVertices = m.positions.size();

				for (size_t v = 0; v < numVertices; ++v)
				{
					const glm::vec4& weights = m.weights[v];
					const glm::uvec4& joints = m.jointIndices[v];

					const glm::mat4& m0 = jointMatrices[joints.x];
					const glm::mat4& m1 = jointMatrices[joints.y];
					const glm::mat4& m2 = jointMatrices[joints.z];
					const glm::mat4& m3 = jointMatrices[joints.w];

					glm::vec4 bindPos = glm::vec4{ glm::vec3{m.positions[v]} , 1.0f };
					glm::vec4 skinnedPos = (m0 * bindPos) * weights.x +
						(m1 * bindPos) * weights.y +
						(m2 * bindPos) * weights.z +
						(m3 * bindPos) * weights.w;

					sPosition[v].first = glm::min(sPosition[v].first, glm::vec3{ skinnedPos });
					sPosition[v].second = glm::max(sPosition[v].second, glm::vec3{ skinnedPos });
				}
			};

		auto calculateJointMatrices = [](const std::vector<skin>& skins) -> std::vector<glm::mat4>
			{
				std::vector<glm::mat4> result{};

				for (auto& s : skins)
				{
					auto mat = s.getJointMatrices();

					result.insert(result.end(), std::move_iterator(mat.begin()), std::move_iterator(mat.end()));
				}

				return result;
			};

		auto mergeSpheres = [](const std::vector<glm::vec4>& spheres) -> glm::vec4
			{
				glm::vec3 center{};
				for (auto& s : spheres)
					center += glm::vec3{ s };

				center /= float(spheres.size());

				float maxDistance = std::numeric_limits<float>::lowest();
				for (auto& s : spheres)
				{
					glm::vec3 toCenter = center - glm::vec3{ s };
					float length = glm::length(toCenter) + s.w;

					if (length > maxDistance)
						maxDistance = length;
				}

				return glm::vec4{ center, maxDistance };
			};

		auto calculateMeshletCone = [](
			const std::vector<std::pair<glm::vec3, glm::vec3>>& skinnedPositions,
			const std::vector<uint32_t>& localIndices
			) -> std::pair<glm::vec3, float>
			{
				std::vector<glm::vec3> triangleNormals;
				size_t triangleCount = localIndices.size() / 3;

				for (size_t t = 0; t < triangleCount; ++t)
				{
					unsigned int a = localIndices[t * 3 + 0];
					unsigned int b = localIndices[t * 3 + 1];
					unsigned int c = localIndices[t * 3 + 2];

					glm::vec3 p0 = (skinnedPositions[a].first + skinnedPositions[a].second) * 0.5f;
					glm::vec3 p1 = (skinnedPositions[b].first + skinnedPositions[b].second) * 0.5f;
					glm::vec3 p2 = (skinnedPositions[c].first + skinnedPositions[c].second) * 0.5f;

					glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
					float area = glm::length(normal);

					if (area > 1e-6f)
						triangleNormals.push_back(normal / area);
				}

				if (triangleNormals.empty())
					return { glm::vec3(0.0f, 1.0f, 0.0f), 1.0f };

				glm::vec3 axis(0.0f);
				for (const auto& n : triangleNormals) axis += n;

				if (glm::length(axis) < 1e-4f)
					return { glm::vec3(0.0f, 1.0f, 0.0f), 1.0f };

				axis = glm::normalize(axis);

				float mindp = 1.0f;
				for (const auto& n : triangleNormals)
					mindp = std::min(mindp, glm::dot(n, axis));

				if (mindp <= 0.0f) return { axis, 1.0f };

				float coneCutoff = std::sqrt(std::max(0.0f, 1.0f - mindp * mindp));
				return { axis, coneCutoff };
			};

		std::vector<glm::mat4> jointMatrices = calculateJointMatrices(skins);
		std::vector<std::pair<glm::vec3, glm::vec3>> skinnedPositions{ m.positions.size() };

		for (auto& sp : skinnedPositions)
		{
			sp.first = glm::vec3{ std::numeric_limits<float>::max() };
			sp.second = glm::vec3{ std::numeric_limits<float>::lowest() };
		}

		float longestAnim = 0.0f;
		float deltaTime = 0.033f;

		for (auto& a : anims)
			for (auto& c : a.channels)
				longestAnim = std::max(c.timestamps->back(), longestAnim);

		for (auto& a : anims)
		{
			for (float currentFrame = 0.0f; currentFrame < longestAnim; currentFrame += deltaTime)
			{
				a.update(deltaTime, skins);
				jointMatrices = calculateJointMatrices(skins);
				skinMesh(m, jointMatrices, skinnedPositions);
			}
		}

		std::vector<glm::vec4> skinnedVertexBS{};
		skinnedVertexBS.reserve(m.positions.size());

		for (auto& sp : skinnedPositions)
		{
			glm::vec3 center = (sp.first + sp.second) / 2.0f;

			skinnedVertexBS.push_back(glm::vec4{ center, glm::length(center - sp.first)});
		}

		for (uint32_t i = 0; i < m.meshlets.data.size(); i++)
		{
			auto& crntMeshlet = m.meshlets.data[i];

			std::vector<glm::vec4> meshletBS{};
			meshletBS.reserve(crntMeshlet.vertexCount);

			std::vector<uint32_t> meshletIndices;

			for (uint32_t v = 0; v < crntMeshlet.vertexCount; v++)
			{
				uint32_t idx = m.indices.data[crntMeshlet.indexBufferOffset + v];
				meshletBS.push_back(skinnedVertexBS[idx]);
			}

			for (uint32_t t = 0; t < crntMeshlet.triangleCount; t++)
			{
				uint32_t packed = m.primitives.data[crntMeshlet.triangleBufferOffset + t];
				uint8_t v0 = (packed >> 0) & 0xFF;
				uint8_t v1 = (packed >> 8) & 0xFF;
				uint8_t v2 = (packed >> 16) & 0xFF;

				meshletIndices.push_back(v0);
				meshletIndices.push_back(v1);
				meshletIndices.push_back(v2);
			}

			glm::vec4 sphere = mergeSpheres(meshletBS); 
			crntMeshlet.bounds.center = glm::vec3{ sphere };
			crntMeshlet.bounds.radius = sphere.w;

			auto [coneAxis, coneCutoff] = calculateMeshletCone(skinnedPositions, meshletIndices);
			crntMeshlet.bounds.coneAxis = coneAxis;
			crntMeshlet.bounds.coneCutoff = coneCutoff;
		}

		// Calculate BS for mesh.
		glm::vec4 meshSphere = mergeSpheres(skinnedVertexBS);

		attrs.bsCenter = glm::vec3{ meshSphere };
		attrs.bsRadius = meshSphere.w;
	}
}
