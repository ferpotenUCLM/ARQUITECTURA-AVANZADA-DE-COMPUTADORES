project( computeShared VERSION 0.0.1 )


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

add_executable( computeShared main.cpp)
target_compile_features( computeShared PUBLIC cxx_std_20)
target_link_libraries( computeShared PUBLIC libDomain::libDomain OpenMP::OpenMP_CXX)

