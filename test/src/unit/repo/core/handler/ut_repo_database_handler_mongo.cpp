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

#include <gtest/gtest.h>
#include <repo/core/handler/repo_database_handler_mongo.h>
#include "../../../repo_test_database_info.h"

using namespace repo::core::handler;

MongoDatabaseHandler* getHandler()
{
	std::string errMsg;
	return 	MongoDatabaseHandler::getHandler(errMsg, REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT,
		1,
		REPO_GTEST_AUTH_DATABASE,
		REPO_GTEST_DBUSER, REPO_GTEST_DBPW);
}

TEST(MongoDatabaseHandlerTest, GetHandlerDisconnectHandler)
{

	std::string errMsg;
	MongoDatabaseHandler* handler =
		MongoDatabaseHandler::getHandler(errMsg, REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT,
		1,
		REPO_GTEST_AUTH_DATABASE, 
		REPO_GTEST_DBUSER, REPO_GTEST_DBPW);

	EXPECT_TRUE(handler);
	EXPECT_TRUE(errMsg.empty());

	EXPECT_TRUE(MongoDatabaseHandler::getHandler(REPO_GTEST_DBADDRESS));
	MongoDatabaseHandler::disconnectHandler();

	EXPECT_FALSE(MongoDatabaseHandler::getHandler(REPO_GTEST_DBADDRESS));

	MongoDatabaseHandler *wrongAdd = MongoDatabaseHandler::getHandler(errMsg, "blah", REPO_GTEST_DBPORT,
		1,
		REPO_GTEST_AUTH_DATABASE,
		REPO_GTEST_DBUSER, REPO_GTEST_DBPW);

	EXPECT_FALSE(wrongAdd);
	MongoDatabaseHandler *wrongPort = MongoDatabaseHandler::getHandler(errMsg, REPO_GTEST_DBADDRESS, 0001,
		1,
		REPO_GTEST_AUTH_DATABASE,
		REPO_GTEST_DBUSER, REPO_GTEST_DBPW);
	EXPECT_FALSE(wrongPort);
	

	//Check can connect without authentication
	MongoDatabaseHandler *noauth = MongoDatabaseHandler::getHandler(errMsg, REPO_GTEST_DBADDRESS, REPO_GTEST_DBPORT,
		1);

	EXPECT_TRUE(noauth);
	MongoDatabaseHandler::disconnectHandler();
}

TEST(MongoDatabaseHandlerTest, CreateBSONCredentials)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	EXPECT_TRUE(handler->createBSONCredentials("testdb" , "username", "password"));
	EXPECT_TRUE(handler->createBSONCredentials("testdb" , "username", "password", true));
	EXPECT_FALSE(handler->createBSONCredentials(""      , "username", "password"));
	EXPECT_FALSE(handler->createBSONCredentials("testdb", ""        , "password"));
	EXPECT_FALSE(handler->createBSONCredentials("testdb", "username", ""));
}

TEST(MongoDatabaseHandlerTest, CountItemsInCollection)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);	
	auto goldenData = getCollectionCounts(REPO_GTEST_DBNAME1);
	for (const auto &pair : goldenData)
	{
		std::string message;
		EXPECT_EQ(pair.second, handler->countItemsInCollection(REPO_GTEST_DBNAME1, pair.first, message)); 
		EXPECT_TRUE(message.empty());
	}

	goldenData = getCollectionCounts(REPO_GTEST_DBNAME2);
	for (const auto &pair : goldenData)
	{
		std::string message;
		EXPECT_EQ(pair.second, handler->countItemsInCollection(REPO_GTEST_DBNAME2, pair.first, message));
		EXPECT_TRUE(message.empty());
	}

	std::string message;
	EXPECT_EQ(0, handler->countItemsInCollection("", "", message));
	EXPECT_FALSE(message.empty());

	message.clear();
	EXPECT_EQ(0, handler->countItemsInCollection("", "blah", message));
	EXPECT_FALSE(message.empty());

	message.clear();
	EXPECT_EQ(0, handler->countItemsInCollection("blah", "", message));
	EXPECT_FALSE(message.empty());

	message.clear();
	EXPECT_EQ(0, handler->countItemsInCollection("blah", "blah", message));
	EXPECT_TRUE(message.empty());

}

TEST(MongoDatabaseHandlerTest, GetAllFromCollectionTailable)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);
	auto goldenData = getGoldenForGetAllFromCollectionTailable();

	std::vector<repo::core::model::RepoBSON> bsons = handler->getAllFromCollectionTailable(
		goldenData.first.first, goldenData.first.second);

	ASSERT_EQ(bsons.size(), goldenData.second.size());
	/*auto goldenDataDisposable = goldenData.second;
	for (int i = 0; i < bsons.size(); ++i)
	{
		bool foundMatch = false;
		for (int j = 0; j< goldenDataDisposable.size(); ++j)
		{
			if (foundMatch = bsons[i].toString() == goldenDataDisposable[j])
			{
				goldenDataDisposable.erase(goldenDataDisposable.begin() + j);
				break;
			}
				
		}
		EXPECT_TRUE(foundMatch);
		
	}*/

	//Test limit and skip
	std::vector<repo::core::model::RepoBSON> bsonsLimitSkip = handler->getAllFromCollectionTailable(
		goldenData.first.first, goldenData.first.second, 1, 1);

	ASSERT_EQ(bsonsLimitSkip.size(), 1);

	EXPECT_EQ(bsonsLimitSkip[0].toString(), bsons[1].toString());

	//test projection
	auto bsonsProjected = handler->getAllFromCollectionTailable(
		goldenData.first.first, goldenData.first.second, 0, 0, { "_id", "shared_id" });

	std::vector<repoUUID> ids;

	ASSERT_EQ(bsonsProjected.size(), bsons.size());
	for (int i = 0; i < bsons.size(); ++i)
	{
		ids.push_back(bsons[i].getUUIDField("_id"));
		EXPECT_EQ(bsons[i].getUUIDField("_id"), bsonsProjected[i].getUUIDField("_id"));
		EXPECT_EQ(bsons[i].getUUIDField("shared_id"), bsonsProjected[i].getUUIDField("shared_id"));
	}

	//test sort
	auto bsonsSorted = handler->getAllFromCollectionTailable(
		goldenData.first.first, goldenData.first.second, 0, 0, {}, "_id", -1);

	std::sort(ids.begin(), ids.end());

	ASSERT_EQ(bsonsSorted.size(), ids.size());
	for (int i = 0; i < bsons.size(); ++i)
	{
		EXPECT_EQ(bsonsSorted[i].getUUIDField("_id"), ids[bsons.size() - i - 1]);
	}

	bsonsSorted = handler->getAllFromCollectionTailable(
		goldenData.first.first, goldenData.first.second, 0, 0, {}, "_id", 1);

	ASSERT_EQ(bsonsSorted.size(), ids.size());
	for (int i = 0; i < bsons.size(); ++i)
	{
		EXPECT_EQ(bsonsSorted[i].getUUIDField("_id"), ids[i]);
	}

	//check error handling - make sure it doesn't crash
	EXPECT_EQ(0, handler->getAllFromCollectionTailable("", "").size());
	EXPECT_EQ(0, handler->getAllFromCollectionTailable("", "blah").size());
	EXPECT_EQ(0, handler->getAllFromCollectionTailable("blah", "").size());
	EXPECT_EQ(0, handler->getAllFromCollectionTailable("blah", "blah").size());
}

TEST(MongoDatabaseHandlerTest, GetCollections)
{
	auto handler = getHandler();
	ASSERT_TRUE(handler);

	auto goldenData = getCollectionList(REPO_GTEST_DBNAME1);

	auto collections = handler->getCollections(REPO_GTEST_DBNAME1);
	
	ASSERT_EQ(goldenData.size(), collections.size());

	std::sort(goldenData.begin(), goldenData.end());
	collections.sort();

	auto colIt = collections.begin();
	auto gdIt = goldenData.begin();
	for (;colIt != collections.end(); ++colIt, ++gdIt)
	{
		EXPECT_EQ(*colIt, *gdIt);
	}


	goldenData = getCollectionList(REPO_GTEST_DBNAME2);
	collections = handler->getCollections(REPO_GTEST_DBNAME2);

	ASSERT_EQ(goldenData.size(), collections.size());
	
	std::sort(goldenData.begin(), goldenData.end());	
	collections.sort();
	
	colIt = collections.begin();
	gdIt = goldenData.begin();
	for (; colIt != collections.end(); ++colIt, ++gdIt)
	{
		EXPECT_EQ(*colIt, *gdIt);
	}

	//check error handling - make sure it doesn't crash
	EXPECT_EQ(0, handler->getCollections("").size());
	EXPECT_EQ(0, handler->getCollections("blahblah").size());
}