# (C) Copyright Vadim Frolov 2014.
#
# Distributed under the FreeBSD Software License (See accompanying file license.txt)

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(oni2avi)

### ---[ Find universal dependencies
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/" ${CMAKE_MODULE_PATH})

# ---[ Release/Debug specific flags
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_definitions("-DBOOST_DISABLE_ASSERTS -DEIGEN_NO_DEBUG")
endif()

find_package(OpenCV 3 COMPONENTS core highgui video imgproc videoio imgcodecs REQUIRED)
find_package(Boost 1.40 COMPONENTS program_options filesystem system REQUIRED )

#not configures as it should, at least for homebrew, so do manually
#find_package(OpenNI 2 REQUIRED)
set(OPENNI2_INCLUDE "/usr/local/include/ni2")
set(OPENNI2_REDIST "/usr/local/lib/ni2/libOpenNI2.dylib")
include_directories(${OPENNI2_INCLUDE})

if (MSVC)
    add_definitions("-DBOOST_ALL_NO_LIB -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -DBOOST_ALL_DYN_LINK")
endif()

include_directories(${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIR} ${OPENNI_INCLUDES} ${OPENNI2_INCLUDES} ${OPENNI_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

add_executable (oni2avi oni2avi.cpp)
target_link_libraries(oni2avi ${OpenCV_LIBS} ${Boost_LIBRARIES} ${OPENNI_LIBRARIES} ${OPENNI2_REDIST})
