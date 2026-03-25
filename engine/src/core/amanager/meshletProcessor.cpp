#include <pch.h>
#include <cgltf.h>
#include <meshoptimizer.h>
#include <glm/gtc/type_ptr.hpp>

#include "meshletProcessor.h"
#include "primitiveProcessor.h"

namespace engine
{
	void calculateTangents(
		std::vector<vertex>& v,
		std::vector<animVertex>& animV,
		const std::vector<uint32_t>& indices
	)
	{
		size_t vertexSize = v.size() == 0 ? animV.size() : v.size();

		std::vector<glm::vec3> tan1{};
		tan1.resize(vertexSize);

		std::vector<glm::vec3> tan2{};
		tan2.resize(vertexSize);

		for (size_t i = 0; i < indices.size(); i += 3)
		{
			uint32_t i1 = indices[i];
			uint32_t i2 = indices[i + 1];
			uint32_t i3 = indices[i + 2];

			const vertex& v1 = v.size() == 0 ? animV[i1].vert : v[i1];
			const vertex& v2 = v.size() == 0 ? animV[i2].vert : v[i2];
			const vertex& v3 = v.size() == 0 ? animV[i3].vert : v[i3];

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

		for (size_t i = 0; i < vertexSize; i++)
		{
			const auto& n = v.size() == 0 ? animV[i].vert.normal : v[i].normal;
			const auto& t = tan1[i];

			// Gram-Schmidt orthogonalize.
			glm::vec3 tangent = glm::normalize(t - n * glm::dot(n, t));

			if (v.size() == 0)
				animV[i].vert.tangent = glm::vec4(tangent, 0.0f);
			else
				v[i].tangent = glm::vec4(tangent, 0.0f);

			// Calculate handedness.
			if (v.size() == 0)
				animV[i].vert.tangent.w = (glm::dot(glm::cross(n, t), tan2[i]) < 0.0F) ? -1.0F : 1.0F;
			else
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

	std::pair<glm::vec3, float> calculateBoundingSphere(const std::vector<vertex>& vertices, const std::vector<animVertex>& animVertices)
	{
		auto findFarthest = [&](glm::vec3 point)-> glm::vec3
			{
				glm::vec3 result{ 0.0f };
				float maxLength = 0.0f;

				if (vertices.size() == 0)
				{
					for (auto& v : animVertices)
					{
						auto length = glm::length(v.vert.position - point);

						if (length > maxLength)
						{
							maxLength = length;
							result = v.vert.position;
						}
					}
				}
				else
				{
					for (auto& v : vertices)
					{
						auto length = glm::length(v.position - point);

						if (length > maxLength)
						{
							maxLength = length;
							result = v.position;
						}
					}
				}

				return result;
			};

		std::pair<glm::vec3, float> result{ glm::vec3(1.0f), 0.0f };

		size_t size = vertices.size() == 0 ? animVertices.size() : vertices.size();

		glm::vec3 first;

		if (vertices.size() == 0)
			first = animVertices[std::rand() % size].vert.position;
		else
			first = vertices[std::rand() % size].position;

		glm::vec3 second = findFarthest(first);
		glm::vec3 third = findFarthest(second);

		glm::vec3 potentialCenter = (second + third) / 2.0f;
		float potentialRadius = glm::length(third - potentialCenter);

		if (vertices.size() == 0)
		{
			for (auto& v : animVertices)
			{
				glm::vec3 toCenter = v.vert.position - potentialCenter;
				float crntRadius = glm::length(toCenter);

				if (crntRadius > potentialRadius)
				{
					float newRadius = potentialRadius + (crntRadius - potentialRadius) / 2.0f;
					potentialCenter += toCenter - (toCenter * (newRadius / crntRadius));
					potentialRadius = newRadius;
				}
			}
		}
		else
		{
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
		}

		result.first = potentialCenter;
		result.second = potentialRadius;

		return result;
	}

	error remapMesh(
		const std::vector<vertex>& vertecies,
		const std::vector<animVertex>& animVertecies,
		const std::vector<uint32_t> indicies,
		std::vector<vertex>& vOut,
		std::vector<animVertex>& animVOut,
		std::vector<uint32_t>& iOut)
	{
		vOut.clear();
		animVOut.clear();
		iOut.clear();

		std::vector<unsigned int> remap(indicies.size());

		size_t size = vertecies.size() == 0 ? animVertecies.size() : vertecies.size();
		float* inputPositionStart = vertecies.size() == 0 ? const_cast<float*>(&animVertecies.front().vert.position.x) : const_cast<float*>(&vertecies.front().position.x);
		size_t sizeOfVertex = vertecies.size() == 0 ? sizeof(animVertex) : sizeof(vertex);

		size_t vertex_count = meshopt_generateVertexRemap(
			remap.data(),
			indicies.data(),
			indicies.size(),
			inputPositionStart,
			size,
			sizeOfVertex
		);
		if (vertex_count == 0)
			return error{ "vertex count is zero" };

		iOut.resize(indicies.size());
		meshopt_remapIndexBuffer(iOut.data(), indicies.data(), indicies.size(), remap.data());

		if (vertecies.size() == 0)
			animVOut.resize(vertex_count);
		else
			vOut.resize(vertex_count);

		const void* inputStart = vertecies.size() == 0 ? static_cast<const void*>(animVertecies.data()) : static_cast<const void*>(vertecies.data());
		void* outputStart = vertecies.size() == 0 ? static_cast<void*>(animVOut.data()) : static_cast<void*>(vOut.data());

		meshopt_remapVertexBuffer(outputStart, inputStart, size, sizeOfVertex, remap.data());

		return {};
	}

	error generateMeshlets(
		const std::vector<vertex>& vertecies,
		const std::vector<animVertex>& animVertecies,
		const std::vector<uint32_t>& indicies,
		std::vector<meshlet>& mOut,
		std::vector<uint8_t>& pOut,
		std::vector<uint32_t>& iOut,
		size_t maxVert, size_t maxTriangles, float coneWieght,
		float errorLevel,
		size_t targetIndexCount,
		uint32_t perMeshOffset
	)
	{
		mOut.clear();
		pOut.clear();
		iOut.clear();

		iOut = indicies;

		size_t size = vertecies.size() == 0 ? animVertecies.size() : vertecies.size();
		float* inputPositionStart = vertecies.size() == 0 ? const_cast<float*>(&animVertecies.front().vert.position.x) : const_cast<float*>(&vertecies.front().position.x);
		size_t sizeOfVertex = vertecies.size() == 0 ? sizeof(animVertex) : sizeof(vertex);

		std::vector<uint32_t> simplyfiedIndexBuf;
		simplyfiedIndexBuf.resize(indicies.size());

		size_t actualSize = meshopt_simplify(
			simplyfiedIndexBuf.data(),
			indicies.data(),
			indicies.size(),
			inputPositionStart,
			size,
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
			inputPositionStart,											// Input: pointer to vertex positions
			size,														// Input: number of vertex positions	
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
				inputPositionStart,
				size,
				sizeOfVertex
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
					.perMeshBufferOffset = perMeshOffset,
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

	error generateLodLevel(const std::vector<vertex>& v, const std::vector<animVertex>& animV, const std::vector<uint32_t> i, mesh& crntMesh, size_t targetIndexCount, size_t maxVert, size_t maxTriangles, float coneWeight, float errorLevel, uint32_t perMeshOffset)
	{
		std::vector<meshlet> meshlets;
		std::vector<uint32_t> indices;
		std::vector<uint8_t> primitives;

		error err = generateMeshlets(
			v,
			animV,
			i,
			meshlets,
			primitives,
			indices,
			maxVert,
			maxTriangles,
			coneWeight,
			errorLevel,
			targetIndexCount,
			perMeshOffset
		);
		if (err)
			return err;

		std::vector<uint32_t> repackedPrimitives = repackPrimitives(primitives, meshlets);

		for (auto& m : meshlets)
		{
			m.indexBufferOffset += uint32_t(crntMesh.indices.data.size());
			m.triangleBufferOffset += uint32_t(crntMesh.primitives.data.size());
		}

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

		return {};
	}

	withError<std::pair<std::vector<mesh>, std::vector<perMeshAttributes>>> proccessMeshes(const cgltf_data* data, size_t maxVert, size_t maxTriangles, float coneWeight, float errorLevel)
	{
		std::pair<std::vector<mesh>, std::vector<perMeshAttributes>> result{};

		LOGDEBUG("mesh count: {}", data->meshes_count);

		uint32_t jointOffset = 0;

		for (size_t ni = 0; ni < data->nodes_count; ++ni)
		{
			const cgltf_node* node = &data->nodes[ni];
			if (!node->mesh)
				continue;

			const cgltf_mesh& gtlfMesh = *node->mesh;

			mesh crntMesh = {};

			perMeshAttributes crntMeshAttrs = {};

			// For tangent calculation and lod calculation.
			std::vector<uint32_t> remappedIndexBuffer;

			for (size_t pri = 0; pri < gtlfMesh.primitives_count; ++pri)
			{
				crntMeshAttrs.meshGlobalTransform = getNodeWorldTransformMat4(node);
				crntMeshAttrs.meshLocalTransform = getNodeLocalTransformMat4(node);
				crntMeshAttrs.isSkinned = uint32_t(node->skin != nullptr);

				primitives crntPrimitive = processPrimitive(gtlfMesh.primitives[pri], crntMeshAttrs.isSkinned, uint32_t(gtlfMesh.primitives[pri].material - data->materials));

				if (crntPrimitive.indicies.size() == 0 || (crntPrimitive.vertecies.size() == 0 && crntPrimitive.animVertecies.size() == 0))
					continue;

				calculateTangents(crntPrimitive.vertecies, crntPrimitive.animVertecies, crntPrimitive.indicies);

				std::vector<vertex> remappedVertex;
				std::vector<animVertex> remappedAnimVertex;
				std::vector<uint32_t> remappedIndex;

				error err = remapMesh(crntPrimitive.vertecies, crntPrimitive.animVertecies, crntPrimitive.indicies, remappedVertex, remappedAnimVertex, remappedIndex);
				if (err)
					return err;

				std::vector<meshlet> meshlets;
				std::vector<uint32_t> indices;
				std::vector<uint8_t> primitives;

				err = generateMeshlets(remappedVertex, remappedAnimVertex, remappedIndex, meshlets, primitives, indices, maxVert, maxTriangles, coneWeight, 0, 0, uint32_t(result.second.size()));
				if (err)
					return err;

				std::vector<uint32_t> repackedPrimitives = repackPrimitives(primitives, meshlets);

				// Write indices for later lod level generation.
				for (auto& i : remappedIndex)
					remappedIndexBuffer.push_back(i + uint32_t(crntMesh.vertices.size()));

				for (auto& m : meshlets)
				{
					m.indexBufferOffset += uint32_t(crntMesh.indices.data.size());
					m.triangleBufferOffset += uint32_t(crntMesh.primitives.data.size());
				}

				for (auto& i : indices)
					i += uint32_t(crntMesh.vertices.size());

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
			}

			if (node->skin)
				jointOffset += uint32_t(node->skin->joints_count);

			auto sphere = calculateBoundingSphere(crntMesh.vertices, crntMesh.animVertices);

			crntMesh.bsCenter = sphere.first;
			crntMesh.bsRadius = sphere.second;

			// Generate lod levels.
			crntMesh.indices.second = uint32_t(crntMesh.indices.data.size());
			crntMesh.primitives.second = uint32_t(crntMesh.primitives.data.size());
			crntMesh.meshlets.second = uint32_t(crntMesh.meshlets.data.size());

			error err = generateLodLevel(crntMesh.vertices, crntMesh.animVertices, remappedIndexBuffer, crntMesh, remappedIndexBuffer.size() / 2, maxVert, maxTriangles, coneWeight, errorLevel, uint32_t(result.second.size()));
			if (err)
				return err;

			crntMesh.indices.third = uint32_t(crntMesh.indices.data.size());
			crntMesh.primitives.third = uint32_t(crntMesh.primitives.data.size());
			crntMesh.meshlets.third = uint32_t(crntMesh.meshlets.data.size());

			err = generateLodLevel(crntMesh.vertices, crntMesh.animVertices, remappedIndexBuffer, crntMesh, remappedIndexBuffer.size() / 3, maxVert, maxTriangles, coneWeight, errorLevel, uint32_t(result.second.size()));
			if (err)
				return err;

			crntMesh.indices.fourth = uint32_t(crntMesh.indices.data.size());
			crntMesh.primitives.fourth = uint32_t(crntMesh.primitives.data.size());
			crntMesh.meshlets.fourth = uint32_t(crntMesh.meshlets.data.size());

			err = generateLodLevel(crntMesh.vertices, crntMesh.animVertices, remappedIndexBuffer, crntMesh, remappedIndexBuffer.size() / 4, maxVert, maxTriangles, coneWeight, errorLevel, uint32_t(result.second.size()));
			if (err)
				return err;

			crntMesh.generateHash();

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

		std::function<skeletonNode(const cgltf_node* root, const cgltf_skin* s, std::map<const cgltf_node*, std::shared_ptr<joint>>& nodeToJoint)> proccessSkinNode;
		proccessSkinNode = [&proccessSkinNode, &getInverseBindForNode](const cgltf_node* root, const cgltf_skin* s, std::map<const cgltf_node*, std::shared_ptr<joint>>& nodeToJoint) -> skeletonNode
			{
				skeletonNode result = {
				};

				auto j = std::make_shared<joint>();

				j->localTransform = getNodeLocalTransform(root);
				j->inverseBind = getInverseBindForNode(s, root);

				result.j = j;

				nodeToJoint[root] = j;

				for (int i = 0; i < root->children_count; i++)
				{
					result.children.push_back(proccessSkinNode(root->children[i], s, nodeToJoint));
				}

				return result;
			};

		std::pair<std::vector<animation>, std::vector<skin>> result{};

		std::map<const cgltf_node*, std::shared_ptr<joint>> nodeToJoint{};

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
					skin s{
						.root = proccessSkinNode(root, sk, nodeToJoint),
					};

					for (int i = 0; i < sk->joints_count; i++)
					{
						s.skinJoints.insert(nodeToJoint[sk->joints[i]]);
					}

					result.second.push_back(s);
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
				};

				ac.j = nodeToJoint[ch.target_node];

				if (!ac.j)
					continue;

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
