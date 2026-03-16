#include <pch.h>
#include <cgltf.h>
#include <meshoptimizer.h>

#include "meshletProcessor.h"
#include "primitiveProcessor.h"

namespace engine
{
	void calculateTangents(
		std::vector<vertex>& v,
		const std::vector<uint32_t>& indices
	)
	{
		std::vector<glm::vec3> tan1{};
		tan1.resize(v.size());

		std::vector<glm::vec3> tan2{};
		tan2.resize(v.size());

		for (size_t i = 0; i < indices.size(); i += 3)
		{
			uint32_t i1 = indices[i];
			uint32_t i2 = indices[i + 1];
			uint32_t i3 = indices[i + 2];

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

	error remapMesh(
		const std::vector<vertex>& vertecies,
		const std::vector<uint32_t> indicies,
		std::vector<vertex>& vOut,
		std::vector<uint32_t>& iOut
	)
	{
		vOut.clear();
		iOut.clear();

		std::vector<unsigned int> remap(indicies.size());

		size_t vertex_count = meshopt_generateVertexRemap(
			remap.data(),
			indicies.data(),
			indicies.size(),
			&vertecies.front().position.x,
			vertecies.size(),
			sizeof(vertex)
		);
		if (vertex_count == 0)
			return error{ "vertex count is zero" };

		iOut.resize(indicies.size());
		meshopt_remapIndexBuffer(iOut.data(), indicies.data(), indicies.size(), remap.data());

		vOut.resize(vertex_count);
		meshopt_remapVertexBuffer(vOut.data(), vertecies.data(), vertecies.size(), sizeof(vertex), remap.data());

		return {};
	}

	error generateMeshlets(
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

	withError<mesh> processMesh(const cgltf_data* data, size_t maxVert, size_t maxTriangles, float coneWeight, float errorLevel)
	{
		mesh result{
			.vertices = std::make_shared<std::vector<vertex>>(),
			.indices = dataWithLodLevels<uint32_t>{
				.data = std::make_shared<std::vector<uint32_t>>()
			},
			.primitives = dataWithLodLevels<uint32_t>{
				.data = std::make_shared<std::vector<uint32_t>>()
			},
			.meshlets = dataWithLodLevels<meshlet>{
				.data = std::make_shared<std::vector<meshlet>>()
			},
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
					coneWeight,
					0.01f,
					targetIndexCount
				);
				if (err)
					return err;

				std::vector<uint32_t> repackedPrimitives = repackPrimitives(primitives, meshlets);

				for (auto& m : meshlets)
				{
					m.indexBufferOffset += uint32_t(result.indices.data->size());
					m.triangleBufferOffset += uint32_t(result.primitives.data->size());
				}

				result.indices.data->insert(
					result.indices.data->end(),
					std::move_iterator(indices.begin()),
					std::move_iterator(indices.end())
				);

				result.primitives.data->insert(
					result.primitives.data->end(),
					std::move_iterator(repackedPrimitives.begin()),
					std::move_iterator(repackedPrimitives.end())
				);

				result.meshlets.data->insert(
					result.meshlets.data->end(),
					std::move_iterator(meshlets.begin()),
					std::move_iterator(meshlets.end())
				);

				return {};
			};

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
				primitives crntPrimitive = processPrimitive(mesh.primitives[pri], transform, data->materials);

				if (crntPrimitive.indicies.size() == 0 || crntPrimitive.vertecies.size() == 0)
					continue;

				calculateTangents(crntPrimitive.vertecies, crntPrimitive.indicies);

				std::vector<vertex> remappedVertex;
				std::vector<uint32_t> remappedIndex;

				error err = remapMesh(crntPrimitive.vertecies, crntPrimitive.indicies, remappedVertex, remappedIndex);
				if (err)
					return err;

				std::vector<meshlet> meshlets;
				std::vector<uint32_t> indices;
				std::vector<uint8_t> primitives;

				err = generateMeshlets(remappedVertex, remappedIndex, meshlets, primitives, indices, maxVert, maxTriangles, coneWeight, 0, 0);
				if (err)
					return err;

				std::vector<uint32_t> repackedPrimitives = repackPrimitives(primitives, meshlets);

				// Write indices for later lod level generation.
				for (auto& i : remappedIndex)
					remappedIndexBuffer.push_back(i + uint32_t(result.vertices->size()));

				for (auto& m : meshlets)
				{
					m.indexBufferOffset += uint32_t(result.indices.data->size());
					m.triangleBufferOffset += uint32_t(result.primitives.data->size());
				}

				for (auto& i : indices)
					i += uint32_t(result.vertices->size());

				result.indices.data->insert(
					result.indices.data->end(),
					std::move_iterator(indices.begin()),
					std::move_iterator(indices.end())
				);

				result.primitives.data->insert(
					result.primitives.data->end(),
					std::move_iterator(repackedPrimitives.begin()),
					std::move_iterator(repackedPrimitives.end())
				);

				result.meshlets.data->insert(
					result.meshlets.data->end(),
					std::move_iterator(meshlets.begin()),
					std::move_iterator(meshlets.end())
				);

				result.vertices->insert(
					result.vertices->end(),
					std::move_iterator(remappedVertex.begin()),
					std::move_iterator(remappedVertex.end())
				);
			}
		}

		auto sphere = calculateBoundingSphere(*result.vertices.get());

		result.bsCenter = sphere.first;
		result.bsRadius = sphere.second;

		// Generate lod levels.
		result.indices.second = uint32_t(result.indices.data->size());
		result.primitives.second = uint32_t(result.primitives.data->size());
		result.meshlets.second = uint32_t(result.meshlets.data->size());

		error err = generateLodLevel(*result.vertices.get(), remappedIndexBuffer, remappedIndexBuffer.size() / 2);
		if (err)
			return err;

		result.indices.third = uint32_t(result.indices.data->size());
		result.primitives.third = uint32_t(result.primitives.data->size());
		result.meshlets.third = uint32_t(result.meshlets.data->size());

		err = generateLodLevel(*result.vertices.get(), remappedIndexBuffer, remappedIndexBuffer.size() / 3);
		if (err)
			return err;

		result.indices.fourth = uint32_t(result.indices.data->size());
		result.primitives.fourth = uint32_t(result.primitives.data->size());
		result.meshlets.fourth = uint32_t(result.meshlets.data->size());

		err = generateLodLevel(*result.vertices.get(), remappedIndexBuffer, remappedIndexBuffer.size() / 4);
		if (err)
			return err;

		result.generateHash();

		return result;
	}
}