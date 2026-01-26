cmake_minimum_required(VERSION 3.27)
project(computeGPU VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glm REQUIRED)

# Opción para seleccionar CPU o GPU
option(USE_GPU "Compilar para GPU usando OpenACC" OFF)

if(USE_GPU)
    set(CMAKE_CXX_COMPILER "nvc++")
    add_compile_options(-acc -Minfo=accel -gpu=managed)
    add_definitions(-DUSE_GPU)
    message(STATUS "Compilando para GPU con OpenACC")
else()
    set(CMAKE_CXX_COMPILER "g++")
    add_compile_options(-O3 -march=native)
    add_definitions(-DUSE_CPU)
    message(STATUS "Compilando para CPU")
endif()

add_executable(computeGPU
    main.cpp
    libDomain/src/Compute.cpp
)

# 🔧 CORRECIÓN: Ahora apunta a libDomain/include
target_include_directories(computeGPU PRIVATE ${CMAKE_SOURCE_DIR}/libDomain/include)
target_link_libraries(computeGPU PRIVATE glm::glm)

# Verificar soporte de OpenACC
include(CheckCXXSourceCompiles)
if(USE_GPU)
    check_cxx_source_compiles("
        #include <openacc.h>
        int main() { return 0; }
    " HAVE_OPENACC)
    
    if(NOT HAVE_OPENACC)
        message(FATAL_ERROR "El compilador no soporta OpenACC")
    endif()
endif()
