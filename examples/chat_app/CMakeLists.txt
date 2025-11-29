cmake_minimum_required(VERSION 3.18)
project(ChatAppExample
    VERSION 1.0.0
    DESCRIPTION "TensorRT-LLM C API Chat Application Example"
    LANGUAGES C
)

# ==============================================================================
# Find or use parent's trtllm_capi target
# ==============================================================================

# Check if this is being built as part of the parent project
if(NOT TARGET trtllm_capi)
    # Standalone build - need to find the installed library
    message(STATUS "Building chat_app as standalone - searching for trtllm_capi")

    # Try to find the parent build directory
    get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
    set(TRTLLM_CAPI_INCLUDE "${PARENT_DIR}/include")
    set(TRTLLM_CAPI_LIB "${PARENT_DIR}/build/libtrtllm_capi.so")

    if(EXISTS ${TRTLLM_CAPI_LIB})
        message(STATUS "Found trtllm_capi at: ${TRTLLM_CAPI_LIB}")
        add_library(trtllm_capi SHARED IMPORTED)
        set_target_properties(trtllm_capi PROPERTIES
            IMPORTED_LOCATION ${TRTLLM_CAPI_LIB}
            INTERFACE_INCLUDE_DIRECTORIES ${TRTLLM_CAPI_INCLUDE}
        )
    else()
        message(FATAL_ERROR
            "Cannot find libtrtllm_capi.so. Please build the main project first:\n"
            "  cd ${PARENT_DIR} && cmake -B build -G Ninja && cmake --build build"
        )
    endif()
else()
    message(STATUS "Building chat_app as part of parent project")
endif()

# ==============================================================================
# Build Chat Application
# ==============================================================================

add_executable(chat_app main.c)

target_link_libraries(chat_app
    PRIVATE
        trtllm_capi
)

# Set RPATH for finding shared libraries at runtime
set_target_properties(chat_app PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib"
    BUILD_WITH_INSTALL_RPATH FALSE
)

# ==============================================================================
# Installation
# ==============================================================================

install(TARGETS chat_app
    RUNTIME DESTINATION bin
)

