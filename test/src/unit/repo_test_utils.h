/**
*  Copyright (C) 2016 3D Repo Ltd
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

#pragma once
#include <repo/core/model/repo_node_utils.h>

//Test Database address
const static std::string REPO_GTEST_DBADDRESS     = "localhost";
const static uint32_t    REPO_GTEST_DBPORT        = 27017;
const static std::string REPO_GTEST_AUTH_DATABASE = "admin";
const static std::string REPO_GTEST_DBUSER        = "testUser";
const static std::string REPO_GTEST_DBPW          = "3drepotest";

static bool compareVectors(const repo_vector2d_t &v1, const repo_vector2d_t &v2)
{
	return v1.x == v2.x && v1.y == v2.y;
}


static bool compareVectors(const repo_vector_t &v1, const repo_vector_t &v2)
{
	return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
}

static bool compareVectors(const repo_color4d_t &v1, const repo_color4d_t &v2)
{
	return v1.r == v2.r && v1.g == v2.g && v1.b == v2.b && v1.a == v2.a;
}

template <typename T>
static bool compareVectors(const std::vector<T> &v1, const  std::vector<T> &v2)
{
	if (v1.size() != v2.size())
	{
		return false;
	}

	for (size_t i = 0; i < v1.size(); ++i)
	{
		if (!compareVectors(v1[i], v2[i]))
		{
			return false;
		}
	}

	return true;
}

template <typename T>
static bool compareStdVectors(const std::vector<T> &v1, const std::vector<T> &v2)
{
	bool identical;
	if (identical = v1.size() == v2.size())
	{
		for (int i = 0; i < v1.size(); ++i)
		{
			identical &= v1[i] == v2[i];
		}
	}
	return identical;
}

static bool compareMaterialStructs(const repo_material_t &m1, const repo_material_t &m2)
{
	return compareStdVectors(m1.ambient, m2.ambient)
		&& compareStdVectors(m1.diffuse, m2.diffuse)
		&& compareStdVectors(m1.specular, m2.specular)
		&& compareStdVectors(m1.emissive, m2.emissive)
		&& m1.opacity == m2.opacity
		&& m1.shininess == m2.shininess
		&& m1.shininessStrength == m2.shininessStrength
		&& m1.isWireframe == m2.isWireframe
		&& m1.isTwoSided == m2.isTwoSided;	
}

