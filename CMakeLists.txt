#  Copyright (C) 2015 3D Repo Ltd
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Affero General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Affero General Public License for more details.
#
#  You should have received a copy of the GNU Affero General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Code to automatically generate CMakeList.txt in sub directories
# This project compiles a library with source files span across multiple directories
# So this generates a CMakeList.txt for each sub directory within the src folder
# update the SOURCES and HEADERS variable, and include their child directory

project("3drepobouncer")

cmake_minimum_required(VERSION 3.2)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)

add_compile_options(-std=c++11)
add_compile_options(-fpermissive)

#================BOOST SETTINGS=======================
add_definitions(-DBOOST_ALL_NO_LIB)
add_definitions(-DBOOST_LOG_DYN_LINK)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

unset(Boost_INCLUDE_DIR CACHE)
unset(Boost_LIBRARY_DIRS CACHE)

find_package(Boost REQUIRED COMPONENTS system thread chrono log log_setup filesystem)


#================MONGO SETTINGS=======================
find_package(Mongo REQUIRED)


#================ASSIMP SETTINGS=======================
find_package(ASSIMP REQUIRED)

#================Source and header files===============
add_subdirectory(src)

if(${Boost_FOUND} AND ${MONGO_FOUND} AND ${ASSIMP_FOUND})
	include_directories(${Boost_INCLUDE_DIRS} ${MONGO_INCLUDE_DIR} ${ASSIMP_INCLUDE_DIR} src)
	add_definitions(${Boost_LIB_DIAGNOSTIC_DEFINITIONS})
	add_definitions(-DREPO_API_LIBRARY)	

	add_library(3drepobouncer SHARED ${SOURCES})

	target_link_libraries(3drepobouncer ${Boost_LIBRARIES} ${MONGO_LIBRARIES} ${ASSIMP_LIBRARIES})

	install(DIRECTORY src/ DESTINATION include FILES_MATCHING PATTERN "*.h")
	install(TARGETS 3drepobouncer DESTINATION lib)

endif()

#=============== Compile Test ===================
option (REPO_BUILD_TESTS "If the test suite is built in addition to the library" OFF)
if (REPO_BUILD_TESTS)


	add_subdirectory(submodules/googletest)
	enable_testing()
	add_subdirectory(test)
	
	include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} ${TEST_HEADERS} src)
	add_executable(3drepobouncerTest ${TEST_SOURCES})   
	target_link_libraries(3drepobouncerTest gtest 3drepobouncer ${Boost_LIBRARIES} ${Mongo_LIBRARIES} ${Assimp_LIBRARIES})
	add_test(3drepobouncerTest 3drepobouncerTest)
endif()