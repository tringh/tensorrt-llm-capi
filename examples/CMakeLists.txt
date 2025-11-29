cmake_minimum_required(VERSION 3.18)
project(TrtLlmCAPI_Examples
    VERSION 1.0.0
    DESCRIPTION "TensorRT-LLM C API Examples"
)

# ==============================================================================
# Examples
# ==============================================================================

message(STATUS "Building TensorRT-LLM C API Examples")

# Add all example subdirectories
add_subdirectory(chat_app)

# Add more examples here as they are created
# add_subdirectory(streaming_example)
# add_subdirectory(batch_example)

