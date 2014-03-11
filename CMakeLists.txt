cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(oni2mov)

### ---[ Find universal dependencies
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/" ${CMAKE_MODULE_PATH})

# ---[ Release/Debug specific flags
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    add_definitions("-DBOOST_DISABLE_ASSERTS -DEIGEN_NO_DEBUG")
endif()

find_package(OpenCV COMPONENTS core highgui video imgproc contrib REQUIRED)
find_package(Boost 1.40 COMPONENTS program_options filesystem system REQUIRED )
find_package(OpenNI REQUIRED)

if (MSVC)
    add_definitions("-DBOOST_ALL_NO_LIB -D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -DBOOST_ALL_DYN_LINK")
endif()

include_directories(${OpenCV_INCLUDE_DIRS} ${Boost_INCLUDE_DIR} ${OPENNI_INCLUDES} ${OPENNI2_INCLUDES} ${OPENNI_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

add_executable (oni2mov oni2avi.cpp)
target_link_libraries(oni2mov ${OpenCV_LIBS} ${Boost_LIBRARIES} ${OPENNI_LIBRARIES})
