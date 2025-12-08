cmake_minimum_required(VERSION 3.27)
project(computeDistributed VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(MPI REQUIRED)

add_executable(computeDistributed main.cpp)
target_compile_features(computeDistributed PUBLIC cxx_std_20)

target_link_libraries(computeDistributed PUBLIC 
    libDomain::libDomain 
    MPI::MPI_CXX
)

# Configuración MPI estándar
target_include_directories(computeDistributed PUBLIC ${MPI_CXX_INCLUDE_PATH})
target_compile_options(computeDistributed PUBLIC ${MPI_CXX_COMPILE_FLAGS})
target_link_libraries(computeDistributed PUBLIC ${MPI_CXX_LIBRARIES})
