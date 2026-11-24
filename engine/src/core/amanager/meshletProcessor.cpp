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
		std::vector<meshletData>& meshlets
	)
	{
		std::vector<uint32_t> repacked{};
		repacked.reserve(primitives.size() / 3);

		for (auto& m : meshlets)
		{
			// Save triangle offset for current meshlet
			uint32_t triangleOffset = uint32_t((repacked.size()));

			// Repack to uint32_t
			for (uint32_t k = 0; k < m.m.triangleCount; ++k)
			{
				uint32_t i0 = 3 * k + 0 + m.m.triangleBufferOffset;
				uint32_t i1 = 3 * k + 1 + m.m.triangleBufferOffset;
				uint32_t i2 = 3 * k + 2 + m.m.triangleBufferOffset;

				uint8_t  vIdx0 = primitives[i0];
				uint8_t  vIdx1 = primitives[i1];
				uint8_t  vIdx2 = primitives[i2];
				uint32_t packed = ((static_cast<uint32_t>(vIdx0) & 0xFF) << 0) |
					((static_cast<uint32_t>(vIdx1) & 0xFF) << 8) |
					((static_cast<uint32_t>(vIdx2) & 0xFF) << 16);

				repacked.push_back(packed);
			}

			// Update triangle offset for current meshlet
			m.m.triangleBufferOffset = triangleOffset;
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
		std::span<const glm::vec4> positions,
		std::span<const uint32_t> indicies,
		std::vector<meshletData>& mOut,
		std::vector<uint8_t>& pOut,
		std::vector<uint32_t>& iOut,
		size_t maxVert, size_t maxTriangles, float coneWieght,
		float errorLevel,
		size_t targetIndexCount,
		uint32_t materialOffset,
		alphaModeType alphaType,
		float* resultError
	)
	{
		mOut.clear();
		pOut.clear();
		iOut.clear();

		iOut = { indicies.begin(), indicies.end() };

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
			errorLevel,
			0,
			resultError
		);

		simplyfiedIndexBuf.resize(actualSize);

		if (simplyfiedIndexBuf.empty())
			return error{ errCodeEmptyLod, "lod level is empty" };

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
				meshletData{
					.m = meshlet{
						.localMaterialOffset = materialOffset,
						.indexBufferOffset = m.vertex_offset,
						.vertexCount = m.vertex_count,
						.triangleBufferOffset = m.triangle_offset,
						.triangleCount = m.triangle_count,
					},
					.attributes = meshletAttributes{
						.alphaType = uint32_t(alphaType),
						.bounds = meshletBounds{
							.center = { bounds.center[0], bounds.center[1], bounds.center[2] },
							.radius = bounds.radius,
							.coneAxis = { bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2] },
							.coneCutoff = bounds.cone_cutoff,
						},
					}
				}
			);
		}

		return {};
	}

	error generateLodLevel(
		std::span<const glm::vec4> positions,
		std::span<const uint32_t> i,
		std::vector<meshletData>& meshletsOut,
		std::vector<uint32_t>& indicesOut,
		std::vector<uint32_t>& repackedPrimitivesOut,
		size_t targetIndexCount,
		size_t maxVert,
		size_t maxTriangles,
		float coneWeight,
		float errorLevel,
		uint32_t materialOffset,
		alphaModeType alphaMode,
		float* resultError
	)
	{
		std::vector<meshletData> meshlets;
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
			alphaMode,
			resultError
		);
		if (err && err.is(errCodeEmptyLod))
			return {};

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

	withError<std::pair<std::vector<mesh>, std::vector<perMeshAttributes>>> proccessMeshes(const cgltf_data* data, size_t maxTriangles, size_t maxVert, float coneWeight, float errorLevel)
	{
		struct lodData
		{
			std::vector<uint32_t> indices;
			std::vector<uint32_t> primitives;
			std::vector<meshletData> meshlets;
			float errorLevel = 0.0f;
		};

		std::pair<std::vector<mesh>, std::vector<perMeshAttributes>> result{};
		std::vector<uint32_t> skinOffsets{};
		skinOffsets.resize(data->skins_count);

		uint32_t sum = 0;
		for (size_t s = 0; s < data->skins_count; s++)
		{
			skinOffsets[s] = sum;
			sum += uint32_t(data->skins[s].joints_count);
		}

		for (size_t ni = 0; ni < data->nodes_count; ++ni)
		{
			const cgltf_node* node = &data->nodes[ni];
			if (!node->mesh)
				continue;

			const cgltf_mesh& gtlfMesh = *node->mesh;

			mesh crntMesh = {};

			perMeshAttributes crntMeshAttrs = {};
			bool isSkinned = uint32_t(node->skin != nullptr);

			uint32_t jointOffset = isSkinned ? skinOffsets[node->skin - data->skins] : 0;

			std::array<lodData, MAX_LOD_LEVELS > lods{};

			for (size_t pri = 0; pri < gtlfMesh.primitives_count; ++pri)
			{
				auto crntPrimitive = processPrimitive(
					gtlfMesh.primitives[pri],
					isSkinned,
					jointOffset,
					isSkinned ? glm::mat4{ 1.0f } : getNodeWorldTransformMat4(node)
				);
				if (!crntPrimitive)
					return crntPrimitive.err();

				uint32_t primitiveMaterialOffset = uint32_t(gtlfMesh.primitives[pri].material - data->materials);

				error err = remapMesh(
					crntPrimitive.value(),
					isSkinned
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

				std::vector<meshletData> meshlets;
				std::vector<meshletAttributes> meshletAttributes;
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
					m.m.indexBufferOffset += uint32_t(lods[0].indices.size());
					m.m.triangleBufferOffset += uint32_t(lods[0].primitives.size());
				}

				size_t vertexBase = crntMesh.positions.size();

				for (auto& i : indices)
					i += uint32_t(vertexBase);

				// First lod level.
				lods[0].indices.insert(
					lods[0].indices.end(),
					std::move_iterator(indices.begin()),
					std::move_iterator(indices.end())
				);

				lods[0].primitives.insert(
					lods[0].primitives.end(),
					std::move_iterator(repackedPrimitives.begin()),
					std::move_iterator(repackedPrimitives.end())
				);

				lods[0].meshlets.insert(
					lods[0].meshlets.end(),
					std::move_iterator(meshlets.begin()),
					std::move_iterator(meshlets.end())
				);

				float scale = meshopt_simplifyScale(&crntPrimitive.value().positions[0].x, crntPrimitive.value().positions.size(), sizeof(glm::vec4));

				constexpr std::array<uint32_t, MAX_LOD_LEVELS> lodLevelTriangleCount{ 1, 2, 8, 16, 32, 128, 256, 1024 };
				for (uint32_t i = 1; i < MAX_LOD_LEVELS; i++)
				{
					std::vector<meshletData> meshlets{};
					std::vector<uint32_t> indices{};
					std::vector<uint32_t> primitives{};

					float resultError = 0.0f;

					err = generateLodLevel(
						crntPrimitive.value().positions,
						crntPrimitive.value().indicies,
						meshlets,
						indices,
						primitives,
						crntPrimitive.value().indicies.size() / size_t(lodLevelTriangleCount[i]),
						maxVert,
						maxTriangles,
						coneWeight,
						errorLevel,
						primitiveMaterialOffset,
						alphaMode,
						&resultError
					);
					if (err)
						return err;

					lods[i].errorLevel = std::max(lods[i].errorLevel, resultError * scale);

					for (auto& m : meshlets)
					{
						m.m.indexBufferOffset += uint32_t(lods[i].indices.size());
						m.m.triangleBufferOffset += uint32_t(lods[i].primitives.size());
					}

					for (auto& idx : indices)
						idx += uint32_t(vertexBase);

					lods[i].indices.insert(
						lods[i].indices.end(),
						std::move_iterator(indices.begin()),
						std::move_iterator(indices.end())
					);

					lods[i].primitives.insert(
						lods[i].primitives.end(),
						std::move_iterator(primitives.begin()),
						std::move_iterator(primitives.end())
					);

					lods[i].meshlets.insert(
						lods[i].meshlets.end(),
						std::move_iterator(meshlets.begin()),
						std::move_iterator(meshlets.end())
					);
				}

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

				if (isSkinned)
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
			}

			auto [center, radius] = calculateBoundingSphere(crntMesh.positions);

			crntMeshAttrs.bsCenter = center;
			crntMeshAttrs.bsRadius = radius;

			bool lodIsSkipped{};
			for (uint32_t i = 0; i < lods.size(); i++)
			{
				lodData& lod = lods[i];

				if ((i != 0 && float(lods[i].meshlets.size()) / float(lods[i - 1].meshlets.size()) >= 0.85f) || lodIsSkipped)
				{
					crntMesh.indices.lodOffsets[i] = std::numeric_limits<uint32_t>::max();
					crntMesh.primitives.lodOffsets[i] = std::numeric_limits<uint32_t>::max();
					crntMesh.meshlets.lodOffsets[i] = std::numeric_limits<uint32_t>::max();

					lodIsSkipped = true;

					continue;
				}

				crntMesh.indices.lodOffsets[i] = uint32_t(crntMesh.indices.data.size());
				crntMesh.primitives.lodOffsets[i] = uint32_t(crntMesh.primitives.data.size());
				crntMesh.meshlets.lodOffsets[i] = uint32_t(crntMesh.meshlets.data.size());
				crntMesh.meshlets.lodErrors[i] = lod.errorLevel;

				for (auto& m : lod.meshlets)
				{
					m.m.indexBufferOffset += uint32_t(crntMesh.indices.data.size());
					m.m.triangleBufferOffset += uint32_t(crntMesh.primitives.data.size());
				}

				crntMesh.indices.data.insert(
					crntMesh.indices.data.end(),
					std::move_iterator(lod.indices.begin()),
					std::move_iterator(lod.indices.end())
				);

				crntMesh.primitives.data.insert(
					crntMesh.primitives.data.end(),
					std::move_iterator(lod.primitives.begin()),
					std::move_iterator(lod.primitives.end())
				);

				crntMesh.meshlets.data.insert(
					crntMesh.meshlets.data.end(),
					std::move_iterator(lod.meshlets.begin()),
					std::move_iterator(lod.meshlets.end())
				);
			}

			// Need to bake node world transform into bounds for mesh and meshlets for skinned meshes.
			if (isSkinned)
			{
				glm::mat4 M = getNodeWorldTransformMat4(node);
				glm::mat3 N = glm::transpose(glm::inverse(glm::mat3{ M }));
				float s = std::max(glm::length(glm::vec3{ M[0] }),
					std::max(glm::length(glm::vec3{ M[1] }), glm::length(glm::vec3{ M[2] })));

				for (auto& md : crntMesh.meshlets.data)
				{
					meshletBounds& b = md.attributes.bounds;
					b.center = glm::vec3{ M * glm::vec4{b.center, 1.0f} };
					b.radius *= s;
					if (b.coneAxis != glm::vec3{ 0.0f })
						b.coneAxis = glm::normalize(N * b.coneAxis);
				}

				crntMeshAttrs.bsCenter = glm::vec3{ M * glm::vec4{crntMeshAttrs.bsCenter, 1.0f} };
				crntMeshAttrs.bsRadius *= s;

				for (auto& e : crntMesh.meshlets.lodErrors)
					e *= s;
			}

			crntMesh.generateHashes();

			result.first.push_back(std::move(crntMesh));
			result.second.push_back(std::move(crntMeshAttrs));
		}

		return result;
	}

	proccessedAnimation proccessAnimations(const cgltf_data* data)
	{
		auto animType = [](cgltf_animation_path_type type) -> animationType
			{
				switch (type)
				{
				case cgltf_animation_path_type_translation:
					return animationType::tr;
				case cgltf_animation_path_type_rotation:
					return animationType::rt;
				case cgltf_animation_path_type_scale:
					return animationType::sc;
				default:
					return animationType::sc;
				}
			};

		auto interType = [](cgltf_interpolation_type type) -> interpolationType
			{
				switch (type)
				{
				case cgltf_interpolation_type_linear:
					return interpolationType::linear;
				case cgltf_interpolation_type_cubic_spline:
					return interpolationType::cubicspline;
				case cgltf_interpolation_type_step:
					return interpolationType::step;
				default:
					return interpolationType::linear;
				}
			};

		std::function<void(const cgltf_skin* s, const cgltf_data* data, skin& result)> processSkinNode;
		processSkinNode = [&](const cgltf_skin* s, const cgltf_data* data, skin& result)
			{
				result.skinJoints.resize(s->joints_count);
				result.inverseBinds.resize(s->joints_count, glm::mat4{ 1.0f });

				for (uint32_t i = 0; i < s->joints_count; i++)
				{
					result.skinJoints[i] = uint32_t(s->joints[i] - data->nodes);

					if (s->inverse_bind_matrices)
						cgltf_accessor_read_float(s->inverse_bind_matrices, i, glm::value_ptr(result.inverseBinds[i]), 16);
				}
			};

		proccessedAnimation result{
			.skinData = std::make_shared<std::vector<skin>>(),
			.jointData = std::make_shared<std::vector<joint>>(),
			.jointOrder = std::make_shared<std::vector<uint32_t>>(),
			.animationData = std::make_shared<std::vector<animation>>(),
		};

		if (data->skins_count == 0)
			return result;

		// Process skins.
		result.skinData->resize(data->skins_count);
		for (int i = 0; i < data->skins_count; i++)
		{
			if (data->skins[i].joints_count != 0)
				processSkinNode(&data->skins[i], data, result.skinData->operator[](i));
		}

		// Process animations.
		result.animationData->reserve(data->animations_count);
		for (int i = 0; i < data->animations_count; i++)
		{
			animation crntAnim{
				.name = data->animations[i].name ? std::string(data->animations[i].name) : std::string{},
			};

			for (int c = 0; c < data->animations[i].channels_count; c++)
			{
				cgltf_animation_sampler* sampler = data->animations[i].channels[c].sampler;
				cgltf_animation_channel ch = data->animations[i].channels[c];

				if (!ch.target_node)
					continue;

				const bool supported =
					ch.target_path == cgltf_animation_path_type_translation ||
					ch.target_path == cgltf_animation_path_type_rotation ||
					ch.target_path == cgltf_animation_path_type_scale;

				if (!supported)
					continue;

				channel ac{
					.aType = animType(ch.target_path),
					.iType = interType(sampler->interpolation),
					.jointIndex = uint32_t(ch.target_node - data->nodes),
				};

				std::vector<float> ts{};
				std::vector<transform> trs{};

				size_t frameCount = sampler->input->count;
				ts.resize(frameCount);

				for (size_t k = 0; k < frameCount; k++)
					cgltf_accessor_read_float(sampler->input, k, &ts[k], 1);

				if (!ts.empty())
					crntAnim.duration = std::max(crntAnim.duration, ts.back());

				trs.resize(frameCount);

				for (size_t k = 0; k < frameCount; k++)
				{
					transform keyframe{
						.scale = glm::vec3{1.0f},
					};

					float val[4];
					if (ac.aType == animationType::rt)
					{
						cgltf_accessor_read_float(sampler->output, k, val, 4);
						keyframe.rotation = glm::quat(val[3], val[0], val[1], val[2]);
					}
					else if (ac.aType == animationType::tr)
					{
						cgltf_accessor_read_float(sampler->output, k, val, 3);
						keyframe.translation = glm::make_vec3(val);
					}
					else if (ac.aType == animationType::sc)
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

			result.animationData->push_back(std::move(crntAnim));
		}

		// Process joints.
		result.jointData->reserve(data->nodes_count);
		result.jointOrder->reserve(data->nodes_count);

		for (int i = 0; i < data->nodes_count; i++)
		{
			if (!data->nodes[i].parent)
				result.jointOrder->push_back(uint32_t(i));

			result.jointData->push_back(
				joint{
					.restPose = getNodeLocalTransform(&data->nodes[i]),
					.parentIdx = int(data->nodes[i].parent ? data->nodes[i].parent - data->nodes : -1),
				}
			);
		}

		for (size_t k = 0; k < result.jointOrder->size(); k++)
		{
			const cgltf_node& node = data->nodes[result.jointOrder->operator[](k)];

			for (cgltf_size c = 0; c < node.children_count; c++)
				result.jointOrder->push_back(uint32_t(node.children[c] - data->nodes));
		}

		return result;
	}

	void recalculateMeshletBounds(
		mesh& m,
		perMeshAttributes& attrs,
		animationAsset animAsset
	)
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

		// Create animation states that will play all animations at once.
		std::vector<animationState> states{};
	
		std::vector<glm::mat4> jointMatrices{};
		std::vector<std::pair<glm::vec3, glm::vec3>> skinnedPositions(
			m.positions.size(),
			{ glm::vec3{ std::numeric_limits<float>::max() }, glm::vec3{ std::numeric_limits<float>::lowest() } }
		);

		// Play rest position.
		playActiveAnimations(0.0f, animAsset, states, jointMatrices);
		skinMesh(m, jointMatrices, skinnedPositions);

		// Play all animations at full weight.
		const float step = 1.0f / 30.0f;
		for (uint32_t i = 0; i < animAsset.animationData->size(); i++)
		{
			states = { animationState{.animationIndex = i, .time = 0.0f, .weight = 1.0f } };

			for (float t = 0.0f; t <= animAsset.animationData->operator[](i).duration; t += step)
			{
				playActiveAnimations(t, animAsset, states, jointMatrices);
				skinMesh(m, jointMatrices, skinnedPositions);
			}
		}

		std::vector<glm::vec4> skinnedVertexBS{};
		skinnedVertexBS.reserve(m.positions.size());

		for (auto& sp : skinnedPositions)
		{
			glm::vec3 center = (sp.first + sp.second) / 2.0f;

			skinnedVertexBS.push_back(glm::vec4{ center, glm::length(center - sp.first) });
		}

		for (uint32_t i = 0; i < m.meshlets.data.size(); i++)
		{
			auto& crntMeshlet = m.meshlets.data[i];

			std::vector<glm::vec4> meshletBS{};
			meshletBS.reserve(crntMeshlet.m.vertexCount);

			std::vector<uint32_t> meshletIndices;

			for (uint32_t v = 0; v < crntMeshlet.m.vertexCount; v++)
			{
				uint32_t idx = m.indices.data[crntMeshlet.m.indexBufferOffset + v];
				meshletBS.push_back(skinnedVertexBS[idx]);
			}

			for (uint32_t t = 0; t < crntMeshlet.m.triangleCount; t++)
			{
				uint32_t packed = m.primitives.data[crntMeshlet.m.triangleBufferOffset + t];
				uint8_t v0 = (packed >> 0) & 0xFF;
				uint8_t v1 = (packed >> 8) & 0xFF;
				uint8_t v2 = (packed >> 16) & 0xFF;

				meshletIndices.push_back(v0);
				meshletIndices.push_back(v1);
				meshletIndices.push_back(v2);
			}

			glm::vec4 sphere = mergeSpheres(meshletBS);
			crntMeshlet.attributes.bounds.center = glm::vec3{ sphere };
			crntMeshlet.attributes.bounds.radius = sphere.w;

			auto [coneAxis, coneCutoff] = calculateMeshletCone(skinnedPositions, meshletIndices);
			crntMeshlet.attributes.bounds.coneAxis = coneAxis;
			crntMeshlet.attributes.bounds.coneCutoff = coneCutoff;
		}

		// Calculate BS for mesh.
		glm::vec4 meshSphere = mergeSpheres(skinnedVertexBS);

		attrs.bsCenter = glm::vec3{ meshSphere };
		attrs.bsRadius = meshSphere.w;
	}
}
