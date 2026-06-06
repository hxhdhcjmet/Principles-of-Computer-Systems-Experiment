# This file is part of the openHiTLS project.
#
# openHiTLS is licensed under the Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#
#     http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(demo)

message(status "HITLS_ROOT: ${HITLS_ROOT}")
set(HITLS_ROOT ../..)
set(HITLS_INCLUDE ${HITLS_ROOT}/include/bsl
                  ${HITLS_ROOT}/include/crypto
                  ${HITLS_ROOT}/include/tls
                  ${HITLS_ROOT}/include/pki
                  ${HITLS_ROOT}/include/auth
                  ${HITLS_ROOT}/config/macro_config
                  ${HITLS_ROOT}/platform/Secure_C/include)

if(CUSTOM_CFLAGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CUSTOM_CFLAGS}")
endif()

if(ENABLE_GCOV)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs -ftest-coverage -lgcov")
endif()

if(ENABLE_ASAN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-stack-protector -fno-omit-frame-pointer")
endif()

add_library(DEMO_INTF INTERFACE)
target_compile_options(DEMO_INTF INTERFACE -g)
target_link_directories(DEMO_INTF INTERFACE ${HITLS_ROOT}/build 
                                            ${HITLS_ROOT}/platform/Secure_C/lib/)

target_link_libraries(DEMO_INTF INTERFACE hitls_tls hitls_pki hitls_auth hitls_crypto hitls_bsl boundscheck pthread dl)
target_include_directories(DEMO_INTF INTERFACE ${HITLS_INCLUDE})

# Standalone executables (each links against openHiTLS via DEMO_INTF)
set(TESTS client.c server.c drbg.c ecdh.c pbkdf2.c sm2enc.c sm2sign.c sm4cbc.c hash.c privpass_token.c sm4cbc_origin.c)
foreach(testcase ${TESTS})
    get_filename_component(testname ${testcase} NAME_WLE)
    add_executable(${testname} ${testcase})
    target_link_libraries(${testname} PRIVATE DEMO_INTF)
endforeach()

# === SM4 standalone implementations (no openHiTLS dependency) ===
# These are self-contained SM4 implementations for performance comparison.
# They only depend on sm4_common.h and libm.

# Scheme 1: default loop (baseline)
add_executable(sm4_default_loop sm4_default_loop.c)
target_link_libraries(sm4_default_loop PRIVATE m)

# Scheme 2: loop unrolling
add_executable(sm4_unrolled sm4_unrolled.c)
target_link_libraries(sm4_unrolled PRIVATE m)

# Scheme 3: reduced dependency
add_executable(sm4_reduced_dep sm4_reduced_dep.c)
target_link_libraries(sm4_reduced_dep PRIVATE m)

# Unified benchmark (links all three schemes together)
# -DSM4_AS_LIBRARY suppresses standalone main() in each scheme file
add_executable(sm4_benchmark sm4_benchmark.c sm4_default_loop.c sm4_unrolled.c sm4_reduced_dep.c)
target_compile_definitions(sm4_benchmark PRIVATE SM4_AS_LIBRARY)
target_link_libraries(sm4_benchmark PRIVATE m)
