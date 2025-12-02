cmake_minimum_required(VERSION 3.18)

project(TrtLlmCWrapper
        VERSION 1.0.0
        DESCRIPTION "TensorRT-LLM C API Wrapper"
        LANGUAGES C CXX CUDA
)

# ==============================================================================
# 1. Path Configuration & Dependencies
# ==============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(TRTLLM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tensorrt-llm" ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH "${TRTLLM_DIR}/cpp/cmake/modules")

# Define Build/Lib paths
set(TRTLLM_BUILD_DIR "${TRTLLM_DIR}/cpp/build")
set(TRTLLM_LIB_PATH "${TRTLLM_BUILD_DIR}/tensorrt_llm/libtensorrt_llm.so")
set(TRTLLM_PLUGIN_PATH "${TRTLLM_BUILD_DIR}/tensorrt_llm/plugins/libnvinfer_plugin_tensorrt_llm.so")
set(TRTLLM_INCLUDE_DIR "${TRTLLM_DIR}/cpp/include")

# ==============================================================================
# 2. Auto-Build Logic (Trusted)
# ==============================================================================

if(NOT EXISTS ${TRTLLM_LIB_PATH})
    message(STATUS "TensorRT-LLM lib not found. triggering build script...")
    execute_process(
            COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_trtllm.sh
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE BUILD_RESULT
    )
    if(NOT BUILD_RESULT EQUAL 0)
        message(FATAL_ERROR "Build script failed.")
    endif()
endif()

# ==============================================================================
# 3. Dependencies
# ==============================================================================

# Find CUDA and TensorRT
find_package(CUDAToolkit REQUIRED)
find_package(TensorRT REQUIRED)

# ABI Detection
execute_process(
        COMMAND bash -c "nm -f posix -D ${TRTLLM_LIB_PATH} | grep __cxx11"
        RESULT_VARIABLE GLIB_CXX11_FOUND
        OUTPUT_QUIET
)

if(GLIB_CXX11_FOUND EQUAL 0)
    add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=1)
    message(STATUS "Using C++11 ABI: 1")
else()
    add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=0)
    message(STATUS "Using C++11 ABI: 0")
endif()

if(CUDAToolkit_VERSION VERSION_GREATER_EQUAL "11.0")
    add_compile_definitions(ENABLE_BF16)
endif()

# ==============================================================================
# 4. Import & Link
# ==============================================================================

add_library(tensorrt_llm SHARED IMPORTED)
set_target_properties(tensorrt_llm PROPERTIES
        IMPORTED_LOCATION ${TRTLLM_LIB_PATH}
        INTERFACE_INCLUDE_DIRECTORIES ${TRTLLM_INCLUDE_DIR}
        INTERFACE_LINK_LIBRARIES "CUDA::cuda_driver;CUDA::cudart_static;CUDA::nvml"
)

add_library(nvinfer_plugin_tensorrt_llm SHARED IMPORTED)
set_target_properties(nvinfer_plugin_tensorrt_llm PROPERTIES
        IMPORTED_LOCATION ${TRTLLM_PLUGIN_PATH}
        INTERFACE_LINK_LIBRARIES tensorrt_llm
)

# -----------------------------------------------------------------------------
# C Wrapper Library
# -----------------------------------------------------------------------------

set_source_files_properties(src/trtllm_capi.c PROPERTIES
        LANGUAGE CXX
        COMPILE_FLAGS "-x c++"
)

add_library(trtllm_capi SHARED src/trtllm_capi.c)

target_include_directories(trtllm_capi
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${TRTLLM_INCLUDE_DIR}
        ${TensorRT_INCLUDE_DIR}
)

target_compile_options(trtllm_capi PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wno-deprecated-declarations>
)

target_link_libraries(trtllm_capi
        PRIVATE
        tensorrt_llm
        nvinfer_plugin_tensorrt_llm
        ${TensorRT_LIBRARIES}
)

target_link_options(trtllm_capi PRIVATE
        "LINKER:-rpath,\$ORIGIN"
)

# Force load plugins (Linux specific)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(trtllm_capi PRIVATE
            "LINKER:--no-as-needed"
            "$<TARGET_FILE:nvinfer_plugin_tensorrt_llm>"
            "LINKER:--as-needed"
    )
endif()

# -----------------------------------------------------------------------------
# Installation
# -----------------------------------------------------------------------------

install(TARGETS trtllm_capi
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)

install(FILES include/trtllm_capi.h
        DESTINATION include
)