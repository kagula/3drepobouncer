/**
*  Copyright (C) 2015 3D Repo Ltd
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
* Allows Import/Export functionality into/output Repo world using ASSIMP
*/

#include "repo_model_import_ifc.h"
#include "../../../core/model/bson/repo_bson_factory.h"
#include <boost/filesystem.hpp>

#include <ifcgeom/IfcGeom.h>
#include <ifcgeom/IfcGeomIterator.h>
#include <ifcparse/IfcParse.h>
#include <ifcparse/IfcFile.h>

using namespace repo::manipulator::modelconvertor;

IFCModelImport::IFCModelImport(const ModelImportConfig *settings) :
AbstractModelImport(settings)
{
	if (settings)
	{
		repoWarning << "IFC importer currently ignores all import settings. Any bespoke configurations will be ignored.";
	}
}

IFCModelImport::~IFCModelImport()
{
}

static void convertVertex(double* vertices, const IfcGeom::Transformation<double> &trans)
{
	auto transVector = trans.matrix().data();
	auto temp1 = transVector[0] * vertices[0] + transVector[3] * vertices[1] + transVector[6] * vertices[2] + transVector[9];
	auto temp2 = transVector[1] * vertices[0] + transVector[4] * vertices[1] + transVector[7] * vertices[2] + transVector[10];
	auto temp3 = transVector[2] * vertices[0] + transVector[5] * vertices[1] + transVector[8] * vertices[2] + transVector[11];
	vertices[0] = temp1;
	vertices[1] = temp2;
	vertices[2] = temp3;
}

bool IFCModelImport::generateGeometry(std::string filePath, std::string &errMsg)
{
	repoInfo << "Initialising Geometry....." << std::endl;

	IfcGeom::IteratorSettings settings;
	settings.set(IfcGeom::IteratorSettings::APPLY_DEFAULT_MATERIALS, true);
	settings.set(IfcGeom::IteratorSettings::DISABLE_OPENING_SUBTRACTIONS, false);
	settings.set(IfcGeom::IteratorSettings::NO_NORMALS, false);
	settings.set(IfcGeom::IteratorSettings::WELD_VERTICES, false);
	settings.set(IfcGeom::IteratorSettings::GENERATE_UVS, true);

	IfcGeom::Iterator<double> context_iterator(settings, filePath);

	std::set<std::string> exclude_entities;
	//Do not generate geometry for openings and spaces
	exclude_entities.insert("IfcOpeningElement");
	exclude_entities.insert("IfcSpace");
	context_iterator.excludeEntities(exclude_entities);

	repoTrace << "Initialising Geom iterator";
	if (context_iterator.initialize())
	{
		repoTrace << "Geom Iterator initialized";
	}
	else
	{
		errMsg = "Failed to initialised Geom Iterator";
		return false;
	}

	int meshCount = 0;
	std::vector<std::vector<repo_face_t>> allFaces;
	std::vector<std::vector<double>> allVertices;
	std::vector<std::vector<double>> allNormals;
	std::vector<std::vector<double>> allUVs;
	std::vector<std::string> allIds, allNames, allMaterials;

	//FIXME: need to do materials

	repoTrace << "Iterating through Geom Iterator.";
	do
	{
		IfcGeom::Element<double> *ob = context_iterator.get();
		auto ob_geo = static_cast<const IfcGeom::TriangulationElement<double>*>(ob);
		if (ob_geo)
		{
			auto faces = ob_geo->geometry().faces();
			auto vertices = ob_geo->geometry().verts();
			auto normals = ob_geo->geometry().normals();
			auto uvs = ob_geo->geometry().uvs();
			auto trans = ob_geo->transformation();
			std::unordered_map<int, std::unordered_map<int, int>>  indexMapping;
			std::unordered_map<int, int> vertexCount;

			std::unordered_map<int, std::vector<double>> post_vertices, post_normals, post_uvs;
			std::unordered_map<int, std::vector<repo_face_t>> post_faces;

			auto matIndIt = ob_geo->geometry().material_ids().begin();

			for (int i = 0; i < vertices.size(); i += 3)
			{
				convertVertex(&vertices[i], trans);

				for (int j = 0; j < 3; ++j)
				{
					int index = j + i;
					if (offset.size() < j + 1)
					{
						offset.push_back(vertices[index]);
					}
					else
					{
						offset[j] = offset[j] > vertices[index] ? vertices[index] : offset[j];
					}
				}
			}

			for (int iface = 0; iface < faces.size(); iface += 3)
			{
				auto matInd = *matIndIt;
				if (indexMapping.find(matInd) == indexMapping.end())
				{
					//new material
					indexMapping[matInd] = std::unordered_map<int, int>();
					vertexCount[matInd] = 0;

					std::unordered_map<int, std::vector<double>> post_vertices, post_normals, post_uvs;
					std::unordered_map<int, std::vector<repo_face_t>> post_faces;

					post_vertices[matInd] = std::vector<double>();
					post_normals[matInd] = std::vector<double>();
					post_uvs[matInd] = std::vector<double>();
					post_faces[matInd] = std::vector<repo_face_t>();

					auto material = ob_geo->geometry().materials()[matInd];
					std::string matName = settings.get(IfcGeom::IteratorSettings::USE_MATERIAL_NAMES) ? material.original_name() : material.name();
					allMaterials.push_back(matName);
					if (materials.find(matName) == materials.end())
					{
						//new material, add it to the vector
						repo_material_t matProp;
						if (material.hasDiffuse())
						{
							auto diffuse = material.diffuse();
							matProp.diffuse = { (float)diffuse[0], (float)diffuse[1], (float)diffuse[2] };
						}

						if (material.hasSpecular())
						{
							auto specular = material.specular();
							matProp.specular = { (float)specular[0], (float)specular[1], (float)specular[2] };
						}

						if (material.hasSpecularity())
						{
							matProp.shininess = material.specularity();
						}
						else
						{
							matProp.shininess = NAN;
						}

						matProp.shininessStrength = NAN;

						if (material.hasTransparency())
						{
							matProp.opacity = 1. - material.transparency();
						}
						else
						{
							matProp.opacity = 1.;
						}

						materials[matName] = new repo::core::model::MaterialNode(repo::core::model::RepoBSONFactory::makeMaterialNode(matProp, matName));
					}
				}

				repo_face_t face;
				for (int j = 0; j < 3; ++j)
				{
					auto vIndex = faces[iface + j];
					if (indexMapping[matInd].find(vIndex) == indexMapping[matInd].end())
					{
						//new index. create a mapping
						indexMapping[matInd][vIndex] = vertexCount[matInd]++;
						for (int ivert = 0; ivert < 3; ++ivert)
						{
							auto bufferInd = ivert + vIndex * 3;
							post_vertices[matInd].push_back(vertices[bufferInd]);

							if (normals.size())
								post_normals[matInd].push_back(normals[bufferInd]);

							if (uvs.size())
							{
								auto uvbufferInd = ivert + vIndex * 2;
								post_uvs[matInd].push_back(uvs[uvbufferInd]);
							}
						}
					}
					face.push_back(indexMapping[matInd][vIndex]);
				}

				post_faces[matInd].push_back(face);

				++matIndIt;
			}

			auto guid = ob_geo->guid();
			auto name = ob_geo->name();
			for (const auto& pair : post_faces)
			{
				auto index = pair.first;
				allVertices.push_back(post_vertices[index]);
				allNormals.push_back(post_normals[index]);
				allFaces.push_back(pair.second);
				allUVs.push_back(post_uvs[index]);

				allIds.push_back(guid);
				allNames.push_back(name);
			}
		}
	} while (context_iterator.next());

	//now we have found all meshes, take the minimum bounding box of the scene as offset
	//and create repo meshes
	repoTrace << "Finished iterating. number of meshes found: " << allVertices.size();
	repoTrace << "Finished iterating. number of materials found: " << materials.size();
	for (int i = 0; i < allVertices.size(); ++i)
	{
		std::vector<repo_vector_t> vertices, normals;
		std::vector<repo_vector2d_t> uvs;
		std::vector<repo_face_t> faces;
		std::vector<std::vector<float>> boundingBox;
		for (int j = 0; j < allVertices[i].size(); j += 3)
		{
			vertices.push_back({ allVertices[i][j] - offset[0], allVertices[i][j + 1] - offset[1], allVertices[i][j + 2] - offset[2] });
			if (allNormals[i].size())
				normals.push_back({ allNormals[i][j], allNormals[i][j + 1], allNormals[i][j + 2] });

			auto vertex = vertices.back();
			if (j == 0)
			{
				boundingBox.push_back({ vertex.x, vertex.y, vertex.z });
				boundingBox.push_back({ vertex.x, vertex.y, vertex.z });
			}
			else
			{
				boundingBox[0][0] = boundingBox[0][0] > vertex.x ? vertex.x : boundingBox[0][0];
				boundingBox[0][1] = boundingBox[0][1] > vertex.y ? vertex.y : boundingBox[0][1];
				boundingBox[0][2] = boundingBox[0][2] > vertex.z ? vertex.z : boundingBox[0][2];

				boundingBox[1][0] = boundingBox[1][0] < vertex.x ? vertex.x : boundingBox[1][0];
				boundingBox[1][1] = boundingBox[1][1] < vertex.y ? vertex.y : boundingBox[1][1];
				boundingBox[1][2] = boundingBox[1][2] < vertex.z ? vertex.z : boundingBox[1][2];
			}
		}

		for (int j = 0; j < allUVs[i].size(); j += 2)
		{
			uvs.push_back({ allUVs[i][j], allUVs[i][j + 1] });
		}

		std::vector < std::vector<repo_vector2d_t>> uvChannels;
		if (uvs.size())
			uvChannels.push_back(uvs);

		auto mesh = repo::core::model::RepoBSONFactory::makeMeshNode(vertices, allFaces[i], normals, boundingBox, uvChannels,
			std::vector<repo_color4d_t>(), std::vector<std::vector<float>>(), allNames[i]);

		if (meshes.find(allIds[i]) == meshes.end())
		{
			meshes[allIds[i]] = std::vector<repo::core::model::MeshNode*>();
		}
		meshes[allIds[i]].push_back(new repo::core::model::MeshNode(mesh));

		if (allMaterials[i] != "")
		{
			//FIXME: probably slow. should do in bulk
			auto matIt = materials.find(allMaterials[i]);
			if (matIt != materials.end())
			{
				*(matIt->second) = matIt->second->cloneAndAddParent(mesh.getSharedID());
			}
		}
	}

	return true;
}

repo::core::model::RepoScene* IFCModelImport::generateRepoScene()
{
	repo::core::model::RepoNodeSet transNodes, meshNodes, matNodes, dummy;
	auto rootNode = new repo::core::model::TransformationNode(repo::core::model::RepoBSONFactory::makeTransformationNode());
	transNodes.insert(rootNode);
	auto rootSharedID = rootNode->getSharedID();

	for (const auto &id : meshes)
	{
		for (auto &mesh : id.second)
		{
			*mesh = mesh->cloneAndAddParent(rootSharedID);
			meshNodes.insert(mesh);
		}
	}

	for (auto &mat : materials)
		matNodes.insert(mat.second);

	auto scene = new repo::core::model::RepoScene({ ifcFile }, dummy, meshNodes, matNodes, dummy, dummy, transNodes);
	scene->setWorldOffset(offset);
	return scene;
}

bool IFCModelImport::importModel(std::string filePath, std::string &errMsg)
{
	ifcFile = filePath;
	std::string fileName = getFileName(filePath);

	repoInfo << "IMPORT [" << fileName << "]";
	repoInfo << "=== IMPORTING MODEL WITH IFC OPEN SHELL ===";
	bool success = false;

	if (success = generateGeometry(filePath, errMsg))
	{
		//generate tree;
		repoInfo << "Geometry generated successfully";
	}
	else
	{
		repoError << "Failed to generate geometry: " << errMsg;
	}

	return success;
}