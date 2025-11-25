cmake_minimum_required(VERSION 3.18)
project(TrtLlmCWrapper
    VERSION 1.0.0
    DESCRIPTION "TensorRT-LLM C Binding Example"
    LANGUAGES C CXX CUDA
)

# ==============================================================================
# Path Configuration
# ==============================================================================

get_filename_component(TRTLLM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tensorrt-llm" ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH "${TRTLLM_DIR}/cpp/cmake/modules")

# Allow custom build directory
if(NOT TRTLLM_BUILD_DIR)
    set(TRTLLM_BUILD_DIR "${TRTLLM_DIR}/cpp/build")
endif()

message(STATUS "TensorRT-LLM Directory: ${TRTLLM_DIR}")
message(STATUS "TensorRT-LLM Build Directory: ${TRTLLM_BUILD_DIR}")

# Set library paths
set(TRTLLM_LIB_PATH "${TRTLLM_BUILD_DIR}/tensorrt_llm/libtensorrt_llm.so")
set(TRTLLM_PLUGIN_PATH "${TRTLLM_BUILD_DIR}/tensorrt_llm/plugins/libnvinfer_plugin_tensorrt_llm.so")
set(TRTLLM_INCLUDE_DIR "${TRTLLM_DIR}/cpp/include")

# Validate that TensorRT-LLM has been built
if(NOT EXISTS ${TRTLLM_LIB_PATH})
    message(FATAL_ERROR
        "Cannot find libtensorrt_llm.so at ${TRTLLM_LIB_PATH}\n"
        "Please build TensorRT-LLM C++ libraries first:\n"
        "  cd ${TRTLLM_DIR}/cpp && cmake -B build -G Ninja && cmake --build build"
    )
endif()

# ==============================================================================
# Compiler & ABI Setup
# ==============================================================================

# Set C++ and C standards
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Detect C++11 ABI used by TensorRT-LLM library
# This ensures binary compatibility with the pre-built library
include(CheckCXXSymbolExists)

# Try to detect ABI by testing linking with the library
file(WRITE "${CMAKE_BINARY_DIR}/test_abi.cpp"
    "#include <string>\n"
    "int main() { std::string s; return 0; }\n"
)

try_compile(ABI_TEST_RESULT
    ${CMAKE_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/test_abi.cpp
    LINK_LIBRARIES ${TRTLLM_LIB_PATH}
    CMAKE_FLAGS "-DCMAKE_CXX_STANDARD=17"
    OUTPUT_VARIABLE ABI_TEST_OUTPUT
)

# If default ABI works, use it; otherwise try the other ABI
if(ABI_TEST_RESULT)
    message(STATUS "Using default C++11 ABI")
else()
    message(STATUS "Default ABI failed, trying alternative ABI")
    # Try with opposite ABI flag
    try_compile(ABI_TEST_RESULT_ALT
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/test_abi.cpp
        LINK_LIBRARIES ${TRTLLM_LIB_PATH}
        CMAKE_FLAGS "-DCMAKE_CXX_STANDARD=17" "-DCMAKE_CXX_FLAGS=-D_GLIBCXX_USE_CXX11_ABI=0"
        OUTPUT_VARIABLE ABI_TEST_OUTPUT_ALT
    )
    if(ABI_TEST_RESULT_ALT)
        add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=0)
        message(STATUS "Using old C++11 ABI (_GLIBCXX_USE_CXX11_ABI=0)")
    else()
        message(WARNING "Could not determine correct ABI, using default")
    endif()
endif()

# ==============================================================================
# Dependencies
# ==============================================================================

# Find CUDA Toolkit
find_package(CUDAToolkit REQUIRED COMPONENTS cuda_driver cudart_static nvml)
message(STATUS "CUDA Version: ${CUDAToolkit_VERSION}")
message(STATUS "CUDA Include: ${CUDAToolkit_INCLUDE_DIRS}")

# Find TensorRT
find_package(TensorRT REQUIRED)

if(TensorRT_FOUND)
    message(STATUS "TensorRT Version: ${TensorRT_VERSION_STRING}")
    message(STATUS "TensorRT Include: ${TensorRT_INCLUDE_DIR}")
    message(STATUS "TensorRT Libraries: ${TensorRT_LIBRARIES}")
else()
    message(FATAL_ERROR "TensorRT not found. Please set TensorRT_ROOT or install TensorRT.")
endif()


# Enable precision support based on CUDA version
if(CUDAToolkit_VERSION VERSION_GREATER_EQUAL "11.0")
    add_compile_definitions(ENABLE_BF16)
    message(STATUS "Enabled BF16 support (CUDA >= 11.0)")
endif()

if(CUDAToolkit_VERSION VERSION_GREATER_EQUAL "11.8")
    add_compile_definitions(ENABLE_FP8)
    message(STATUS "Enabled FP8 support (CUDA >= 11.8)")
endif()

# ==============================================================================
# Import TensorRT-LLM Libraries
# ==============================================================================

# Import TensorRT-LLM core library
add_library(tensorrt_llm SHARED IMPORTED)
set_target_properties(tensorrt_llm PROPERTIES
    IMPORTED_LOCATION ${TRTLLM_LIB_PATH}
    INTERFACE_INCLUDE_DIRECTORIES ${TRTLLM_INCLUDE_DIR}
    INTERFACE_LINK_LIBRARIES "CUDA::cuda_driver;CUDA::cudart_static;CUDA::nvml"
)

# Import TensorRT-LLM plugin library
add_library(nvinfer_plugin_tensorrt_llm SHARED IMPORTED)
set_target_properties(nvinfer_plugin_tensorrt_llm PROPERTIES
    IMPORTED_LOCATION ${TRTLLM_PLUGIN_PATH}
    INTERFACE_LINK_LIBRARIES tensorrt_llm
)

# Verify plugin library exists
if(NOT EXISTS ${TRTLLM_PLUGIN_PATH})
    message(FATAL_ERROR
        "Cannot find plugin library at ${TRTLLM_PLUGIN_PATH}\n"
        "Please ensure TensorRT-LLM was built with plugins enabled."
    )
endif()

# ==============================================================================
# Build Targets
# ==============================================================================

# -----------------------------------------------------------------------------
# C Wrapper Library
# -----------------------------------------------------------------------------

# Note: trtllm_capi.c has a .c extension but contains C++ code to wrap
# the TensorRT-LLM C++ API with extern "C" functions. It must be compiled as C++.
# The .c extension is intentional to signal that it provides a C interface.
set_source_files_properties(src/trtllm_capi.c PROPERTIES
    LANGUAGE CXX
    COMPILE_FLAGS "-x c++"
)

add_library(trtllm_capi SHARED src/trtllm_capi.c)

# Include directories
target_include_directories(trtllm_capi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${TRTLLM_INCLUDE_DIR}
        ${TensorRT_INCLUDE_DIR}
)

# Compile options
target_compile_options(trtllm_capi PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wno-deprecated-declarations>
)

# Link libraries
target_link_libraries(trtllm_capi
    PRIVATE
        tensorrt_llm
        nvinfer_plugin_tensorrt_llm
        ${TensorRT_LIBRARIES}
)

# Ensure plugin library is loaded at runtime
# Use LINK_OPTIONS instead of inline linker flags for better CMake compatibility
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(trtllm_capi PRIVATE
        "LINKER:--no-as-needed"
        "$<TARGET_FILE:nvinfer_plugin_tensorrt_llm>"
        "LINKER:--as-needed"
    )
endif()

# -----------------------------------------------------------------------------
# Example C Application
# -----------------------------------------------------------------------------

add_executable(chat_app src/main.c)

target_include_directories(chat_app
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(chat_app
    PRIVATE
        trtllm_capi
)

# -----------------------------------------------------------------------------
# Installation (optional)
# -----------------------------------------------------------------------------

install(TARGETS trtllm_capi chat_app
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES include/trtllm_capi.h
    DESTINATION include
)
