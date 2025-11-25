/*
 * TensorRT-LLM C Wrapper Implementation
 *
 * NOTE: This file has a .c extension but contains C++ code.
 *       It MUST be compiled as C++. The CMakeLists.txt handles this via set_source_files_properties.
 *
 * This wrapper provides a C-compatible interface to TensorRT-LLM's C++ Executor API,
 * enabling integration with C applications and other languages via FFI.
 */

// Standard C++ includes
#include <cstdint>
#include <iostream>
#include <vector>
#include <filesystem>

// TensorRT includes
#include "NvInfer.h"

// TensorRT-LLM includes
#include "tensorrt_llm/executor/executor.h"
#include "tensorrt_llm/plugins/api/tllmPlugin.h"

// Wrapper header
#include "trtllm_capi.h"

using namespace tensorrt_llm::executor;

// ============================================================================
// Logger Implementation
// ============================================================================

/**
 * Minimal logger for TensorRT operations.
 * Only logs errors and warnings to keep output clean.
 */
class WrapperLogger : public nvinfer1::ILogger
{
public:
    void log(Severity severity, char const* msg) noexcept override
    {
        if (severity <= Severity::kWARNING)
        {
            std::cerr << "[TRT-LLM] " << msg << std::endl;
        }
    }
};

static WrapperLogger gLogger;

// ============================================================================
// Internal Wrapper Structures
// ============================================================================

/**
 * Opaque wrapper around TensorRT-LLM Executor.
 * Manages model loading and request execution.
 */
struct TrtLlmExecutor
{
    Executor* impl;
};

/**
 * Opaque wrapper around TensorRT-LLM Request.
 * Contains input tokens and generation configuration.
 */
struct TrtLlmRequest
{
    Request* impl;
};

/**
 * Opaque wrapper around response list.
 * Contains one or more responses from the executor.
 */
struct TrtLlmResponseList
{
    std::vector<Response> responses;
};

// ============================================================================
// C API Implementation
// ============================================================================

extern "C"
{

    TrtLlmExecutor* trt_create_executor(char const* model_path, TrtModelType model_type, int max_beam_width)
    {
        try
        {
            // Initialize plugins once per process
            static bool plugins_initialized = false;
            if (!plugins_initialized)
            {
                std::cout << "[C-Wrapper] Initializing TRT-LLM plugins..." << std::endl;

                // Register all TensorRT-LLM plugins using default logger and namespace
                // This must be called before loading any TensorRT engines
                bool success = initTrtLlmPlugins();

                if (!success)
                {
                    std::cerr << "[C-Wrapper] ERROR: Failed to register TRT-LLM plugins!" << std::endl;
                    return nullptr;
                }

                std::cout << "[C-Wrapper] TRT-LLM plugins registered successfully." << std::endl;
                plugins_initialized = true;
            }

            // Configure KV cache for efficient multi-turn conversations
            KvCacheConfig kvConfig;
            kvConfig.setEnableBlockReuse(true);

            // Create executor configuration
            ExecutorConfig config(max_beam_width);
            config.setKvCacheConfig(kvConfig);

            // Create the executor
            auto type = static_cast<ModelType>(model_type);
            auto* exec = new Executor(std::filesystem::path(model_path), type, config);

            // Wrap in C-compatible structure
            auto* wrapper = new TrtLlmExecutor{exec};
            return wrapper;
        }
        catch (std::exception const& e)
        {
            std::cerr << "[C-Wrapper] ERROR: Failed to create executor: " << e.what() << std::endl;
            return nullptr;
        }
    }

    void trt_destroy_executor(TrtLlmExecutor* executor)
    {
        if (executor)
        {
            delete executor->impl;
            delete executor;
        }
    }

    TrtLlmRequest* trt_create_request(int32_t const* input_ids, int input_len, int max_new_tokens)
    {
        try
        {
            std::vector<int32_t> input_vec(input_ids, input_ids + input_len);
            SamplingConfig samplingConfig(1);

            auto* req = new Request(input_vec, max_new_tokens, false, samplingConfig);
            auto* wrapper = new TrtLlmRequest{req};
            return wrapper;
        }
        catch (std::exception const& e)
        {
            std::cerr << "[C-Wrapper] ERROR: Failed to create request: " << e.what() << std::endl;
            return nullptr;
        }
    }

    void trt_request_set_temperature(TrtLlmRequest* req, float temperature)
    {
        if (req && req->impl)
        {
            SamplingConfig config = req->impl->getSamplingConfig();
            config.setTemperature(temperature);
            req->impl->setSamplingConfig(config);
        }
    }

    void trt_request_set_top_k(TrtLlmRequest* req, int k)
    {
        if (req && req->impl)
        {
            SamplingConfig config = req->impl->getSamplingConfig();
            config.setTopK(k);
            req->impl->setSamplingConfig(config);
        }
    }

    void trt_request_set_top_p(TrtLlmRequest* req, float p)
    {
        if (req && req->impl)
        {
            SamplingConfig config = req->impl->getSamplingConfig();
            config.setTopP(p);
            req->impl->setSamplingConfig(config);
        }
    }

    void trt_request_set_streaming(TrtLlmRequest* req, bool streaming)
    {
        if (req && req->impl)
        {
            req->impl->setStreaming(streaming);
        }
    }

    void trt_request_set_end_id(TrtLlmRequest* req, int32_t end_id)
    {
        if (req && req->impl)
        {
            req->impl->setEndId(end_id);
        }
    }

    void trt_request_set_client_id(TrtLlmRequest* req, uint64_t client_id)
    {
        if (req && req->impl)
        {
            req->impl->setClientId(client_id);
        }
    }

    void trt_destroy_request(TrtLlmRequest* req)
    {
        if (req)
        {
            delete req->impl;
            delete req;
        }
    }

    uint64_t trt_executor_enqueue(TrtLlmExecutor* executor, TrtLlmRequest* request)
    {
        if (!executor || !request)
        {
            return 0;
        }

        try
        {
            return executor->impl->enqueueRequest(*request->impl);
        }
        catch (std::exception const& e)
        {
            std::cerr << "[C-Wrapper] ERROR: Enqueue failed: " << e.what() << std::endl;
            return 0;
        }
    }

    void trt_executor_cancel_request(TrtLlmExecutor* executor, uint64_t request_id)
    {
        if (!executor)
        {
            return;
        }

        try
        {
            executor->impl->cancelRequest(request_id);
        }
        catch (std::exception const& e)
        {
            std::cerr << "[C-Wrapper] ERROR: Cancel failed: " << e.what() << std::endl;
        }
    }

    TrtLlmResponseList* trt_executor_await_responses(TrtLlmExecutor* executor, uint64_t request_id)
    {
        if (!executor)
        {
            return nullptr;
        }

        try
        {
            auto responses = executor->impl->awaitResponses(request_id);
            auto* list = new TrtLlmResponseList{std::move(responses)};
            return list;
        }
        catch (std::exception const& e)
        {
            std::cerr << "[C-Wrapper] ERROR: Await failed: " << e.what() << std::endl;
            return nullptr;
        }
    }

    int trt_response_list_size(TrtLlmResponseList* list)
    {
        return list ? static_cast<int>(list->responses.size()) : 0;
    }

    bool trt_response_has_error(TrtLlmResponseList* list, int index)
    {
        if (!list || index < 0 || index >= static_cast<int>(list->responses.size()))
        {
            return true;
        }
        return list->responses[index].hasError();
    }

    char const* trt_response_get_error(TrtLlmResponseList* list, int index)
    {
        if (!list || index < 0 || index >= static_cast<int>(list->responses.size()))
        {
            return "Invalid index";
        }

        if (list->responses[index].hasError())
        {
            return list->responses[index].getErrorMsg().c_str();
        }

        return nullptr;
    }

    bool trt_response_is_final(TrtLlmResponseList* list, int index)
    {
        if (!list || index < 0 || index >= static_cast<int>(list->responses.size()))
        {
            return true;
        }

        if (list->responses[index].hasError())
        {
            return true;
        }

        return list->responses[index].getResult().isFinal;
    }

    int trt_response_get_tokens(TrtLlmResponseList* list, int index, int32_t* out_buffer, int max_size)
    {
        if (!list || index < 0 || index >= static_cast<int>(list->responses.size()) || !out_buffer || max_size <= 0)
        {
            return 0;
        }

        auto const& response = list->responses[index];
        if (response.hasError())
        {
            return 0;
        }

        auto const& result = response.getResult();
        auto const& tokens = result.outputTokenIds;

        if (tokens.empty())
        {
            return 0;
        }

        // Get first beam (beam 0)
        auto const& beam0 = tokens[0];
        int count = 0;

        for (size_t i = 0; i < beam0.size() && count < max_size; ++i)
        {
            out_buffer[count++] = beam0[i];
        }

        return count;
    }

    void trt_destroy_response_list(TrtLlmResponseList* list)
    {
        delete list;
    }
}