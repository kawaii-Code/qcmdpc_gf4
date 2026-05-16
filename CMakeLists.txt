cmake_minimum_required(VERSION 3.16)
project(mdpc_decoder CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BUILD_GFPX "Build GFPX implementation" ON)
option(BUILD_GF4X "Build GF4X implementation" ON)
option(BUILD_BIKE "Build BIKE library and benchmark" ON)

set(COMMON_CXX_FLAGS -Wall -Wextra -Wpedantic -Wfloat-conversion)
set(COMMON_C_FLAGS -Wall -Wextra -Wpedantic)

find_library(GMP_LIBRARY gmp)
find_library(GMPXX_LIBRARY gmpxx)
if(NOT GMP_LIBRARY)
    message(FATAL_ERROR "GMP library not found.")
endif()

set(NTL_LIB ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ntl-11.6.0/ntl.a)
set(NTL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ntl-11.6.0/include)
if(NOT EXISTS ${NTL_LIB})
    message(FATAL_ERROR "NTL library not found at ${NTL_LIB}.")
endif()

add_library(ntl_imported STATIC IMPORTED)
set_target_properties(ntl_imported PROPERTIES
    IMPORTED_LOCATION ${NTL_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${NTL_INCLUDE_DIR}
)

if(BUILD_GFPX)
    file(GLOB GFPX_SOURCES src/gfpx/*.cpp)
    add_executable(gfpx ${GFPX_SOURCES})
    target_compile_options(gfpx PRIVATE ${COMMON_CXX_FLAGS})
    target_include_directories(gfpx PRIVATE
        src/gfpx
        src
        ${NTL_INCLUDE_DIR}
    )
    target_link_libraries(gfpx PRIVATE ntl_imported ${GMP_LIBRARY})
    target_precompile_headers(gfpx PRIVATE <bits/stdc++.h>)
endif()

if(BUILD_BIKE)
    set(BIKE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/bike-kem)
    set(BIKE_INCLUDE_DIR ${BIKE_SOURCE_DIR}/include)
    set(BIKE_COMPILE_DEFS -DX86_64 -DUSE_SHA3_AND_SHAKE=1 -DSTANDALONE_IMPL=1)

    set(BIKE_LIB_SOURCES
        ${BIKE_SOURCE_DIR}/src/kem.c
        ${BIKE_SOURCE_DIR}/src/decode/decode.c
        ${BIKE_SOURCE_DIR}/src/decode/decode_avx2.c
        ${BIKE_SOURCE_DIR}/src/decode/decode_avx512.c
        ${BIKE_SOURCE_DIR}/src/decode/decode_portable.c
        ${BIKE_SOURCE_DIR}/src/gf2x/gf2x_mul.c
        ${BIKE_SOURCE_DIR}/src/gf2x/gf2x_mul_base_pclmul.c
        ${BIKE_SOURCE_DIR}/src/gf2x/gf2x_mul_base_portable.c
        ${BIKE_SOURCE_DIR}/src/gf2x/gf2x_mul_base_vpclmul.c
        ${BIKE_SOURCE_DIR}/src/gf2x/gf2x_mul_portable.c
        ${BIKE_SOURCE_DIR}/src/gf2x/gf2x_mul_avx2.c
        ${BIKE_SOURCE_DIR}/src/gf2x/gf2x_mul_avx512.c
        ${BIKE_SOURCE_DIR}/src/gf2x/gf2x_ksqr_portable.c
        ${BIKE_SOURCE_DIR}/src/gf2x/gf2x_ksqr_avx2.c
        ${BIKE_SOURCE_DIR}/src/gf2x/gf2x_ksqr_avx512.c
        ${BIKE_SOURCE_DIR}/src/gf2x/gf2x_inv.c
        ${BIKE_SOURCE_DIR}/src/common/cpu_features.c
        ${BIKE_SOURCE_DIR}/src/common/utilities.c
        ${BIKE_SOURCE_DIR}/src/common/error.c
        ${BIKE_SOURCE_DIR}/src/random/sampling.c
        ${BIKE_SOURCE_DIR}/src/random/sampling_portable.c
        ${BIKE_SOURCE_DIR}/src/random/sampling_avx2.c
        ${BIKE_SOURCE_DIR}/src/random/sampling_avx512.c
        ${BIKE_SOURCE_DIR}/src/random/shake_prf.c
        ${BIKE_SOURCE_DIR}/src/third_party_src/fips202.c
    )

    foreach(BIKE_LEVEL 1;3;5)
        add_library(bike_ref_l${BIKE_LEVEL} STATIC ${BIKE_LIB_SOURCES})
        target_compile_options(bike_ref_l${BIKE_LEVEL} PRIVATE -mavx2 -mpclmul ${BIKE_COMPILE_DEFS})
        target_compile_definitions(bike_ref_l${BIKE_LEVEL} PRIVATE LEVEL=${BIKE_LEVEL})
        target_include_directories(bike_ref_l${BIKE_LEVEL} PRIVATE
            ${BIKE_INCLUDE_DIR}
            ${BIKE_INCLUDE_DIR}/internal
            ${BIKE_SOURCE_DIR}/src/third_party_src
        )
        target_compile_features(bike_ref_l${BIKE_LEVEL} PRIVATE c_std_11)

        set_source_files_properties(
            ${BIKE_SOURCE_DIR}/src/gf2x/gf2x_mul_base_vpclmul.c
            ${BIKE_SOURCE_DIR}/src/gf2x/gf2x_mul_avx512.c
            ${BIKE_SOURCE_DIR}/src/gf2x/gf2x_ksqr_avx512.c
            ${BIKE_SOURCE_DIR}/src/decode/decode_avx512.c
            ${BIKE_SOURCE_DIR}/src/random/sampling_avx512.c
            PROPERTIES COMPILE_OPTIONS "-mavx512f;-mavx512bw;-mavx512dq;-mvpclmulqdq"
        )

        add_executable(bike_l${BIKE_LEVEL} src/bike_benchmark/main.cpp)
        target_compile_options(bike_l${BIKE_LEVEL} PRIVATE ${COMMON_CXX_FLAGS} -mavx2 -mpclmul)
        target_include_directories(bike_l${BIKE_LEVEL} PRIVATE ${BIKE_INCLUDE_DIR} ${BIKE_INCLUDE_DIR}/internal)
        target_link_libraries(bike_l${BIKE_LEVEL} PRIVATE bike_ref_l${BIKE_LEVEL} ${GMP_LIBRARY})
        target_compile_definitions(bike_l${BIKE_LEVEL} PRIVATE LEVEL=${BIKE_LEVEL})
    endforeach()
endif()

if(BUILD_GF4X)
    set(GF4X_LIB_SOURCES
        src/gf4x/mdpc.c
        src/gf4x/gf4x.c
        src/gf4x/gf2x.c
        src/gf4x/basic.c
        src/gf4x/gf2x_portable.c
    )

    add_library(gf4x STATIC ${GF4X_LIB_SOURCES})
    target_compile_options(gf4x PRIVATE ${COMMON_C_FLAGS} -mavx2 -mpclmul -O3 -fno-strict-aliasing)
    target_include_directories(gf4x PRIVATE src/gf4x)
    target_compile_features(gf4x PRIVATE c_std_11)

    add_executable(gf4x_benchmark src/gf4x_benchmark/main.c)
    target_compile_options(gf4x_benchmark PRIVATE ${COMMON_C_FLAGS} -mavx2 -mpclmul -O3 -fno-strict-aliasing)
    target_include_directories(gf4x_benchmark PRIVATE src/gf4x)
    target_link_libraries(gf4x_benchmark PRIVATE gf4x)
endif()
