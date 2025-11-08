cmake_minimum_required(VERSION 3.27)

# Set project name, version and languages
project( aac-practicas VERSION 0.1.0 LANGUAGES CXX )

# Set C++20 stantard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compile aux library
add_subdirectory( libDomain )

# Compile executable for shared memory
add_subdirectory( computeShared )

#add_subdirectory( computeDistributed )
#add_subdirectory( computeStream )