cmake_minimum_required(VERSION 2.6)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -lrt -Wall -pthread")

AUX_SOURCE_DIRECTORY(. main)

add_library(stat ${main} ${http_service})

LINK_DIRECTORIES(
${CMAKE_CURRENT_BINARY_DIR}/../../comm
${CMAKE_CURRENT_BINARY_DIR}/../../3rdlib/jsoncpp/lib
${CMAKE_CURRENT_BINARY_DIR}/../../3rdlib/attr_api
)

#ADD_EXECUTABLE(stattool ${main} ${http_service})

target_include_directories(stat  PUBLIC
../../comm
../../3rdlib/attr_api
../../3rdlib/jsoncpp/include
)
#[[
target_include_directories(stattool  PUBLIC
../../comm
../../3rdlib/attr_api
../../3rdlib/jsoncpp/include
)

target_link_libraries(stattool libattr_api_64.a libcommon.a jsoncpp dl rt)
]]
