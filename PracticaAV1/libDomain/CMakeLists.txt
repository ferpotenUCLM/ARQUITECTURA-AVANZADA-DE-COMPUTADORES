project(libDomain VERSION 0.0.1 )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glm REQUIRED)

set( LIB_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/include/libDomain/Compute.h
	${CMAKE_CURRENT_SOURCE_DIR}/include/libDomain/CSVLoader.h
)

set( LIB_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/Compute.cpp
)




add_library( libDomain ${LIB_SOURCES} ${LIB_HEADERS})
target_include_directories(libDomain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(libDomain::libDomain ALIAS libDomain)
target_compile_features( libDomain PUBLIC cxx_std_20)
target_link_libraries( libDomain )

source_group(
  TREE "${PROJECT_SOURCE_DIR}/include"
  PREFIX "Header Files"
  FILES ${HEADER_LIST})
