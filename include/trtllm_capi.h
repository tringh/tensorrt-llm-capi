/*
 * TensorRT-LLM C API Wrapper
 *
 * This header provides a pure C interface to TensorRT-LLM's C++ Executor API.
 * It enables integration with C applications and foreign function interfaces (FFI)
 * from other programming languages.
 *
 * Features:
 * - Executor management (model loading, configuration)
 * - Request creation with sampling parameters
 * - Async request execution
 * - Streaming and batch inference support
 * - KV cache reuse for multi-turn conversations
 *
 * Usage:
 * 1. Create an executor with trt_create_executor()
 * 2. Create a request with trt_create_request()
 * 3. Configure request parameters (optional)
 * 4. Enqueue request with trt_executor_enqueue()
 * 5. Poll for responses with trt_executor_await_responses()
 * 6. Process tokens from responses
 * 7. Clean up with destroy functions
 */

#ifndef TRTLLM_CAPI_H
#define TRTLLM_CAPI_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // ========================================================================
    // Opaque Handles
    // ========================================================================

    /** Opaque handle to a TensorRT-LLM executor instance */
    typedef struct TrtLlmExecutor TrtLlmExecutor;

    /** Opaque handle to a generation request */
    typedef struct TrtLlmRequest TrtLlmRequest;

    /** Opaque handle to a list of responses */
    typedef struct TrtLlmResponseList TrtLlmResponseList;

    // ========================================================================
    // Enumerations
    // ========================================================================

    /** Model architecture types supported by TensorRT-LLM */
    typedef enum
    {
        TRT_MODEL_TYPE_DECODER_ONLY = 0,     /**< GPT-style decoder-only models */
        TRT_MODEL_TYPE_ENCODER_ONLY = 1,     /**< BERT-style encoder-only models */
        TRT_MODEL_TYPE_ENCODER_DECODER = 2   /**< T5-style encoder-decoder models */
    } TrtModelType;

    // ========================================================================
    // Executor Management
    // ========================================================================

    /**
     * Create a new TensorRT-LLM executor.
     *
     * @param model_path Path to the TensorRT engine directory
     * @param model_type Type of model architecture
     * @param max_beam_width Maximum beam width for beam search
     * @return Executor handle, or NULL on failure
     */
    TrtLlmExecutor* trt_create_executor(char const* model_path, TrtModelType model_type, int max_beam_width);

    /**
     * Destroy an executor and free its resources.
     *
     * @param executor Executor handle to destroy
     */
    void trt_destroy_executor(TrtLlmExecutor* executor);

    // ========================================================================
    // Request Management
    // ========================================================================

    /**
     * Create a new generation request.
     *
     * @param input_ids Array of input token IDs
     * @param input_len Length of input_ids array
     * @param max_new_tokens Maximum number of new tokens to generate
     * @return Request handle, or NULL on failure
     */
    TrtLlmRequest* trt_create_request(int32_t const* input_ids, int input_len, int max_new_tokens);

    /** Set temperature for sampling (higher = more random) */
    void trt_request_set_temperature(TrtLlmRequest* req, float temperature);

    /** Set top-k sampling parameter */
    void trt_request_set_top_k(TrtLlmRequest* req, int k);

    /** Set top-p (nucleus) sampling parameter */
    void trt_request_set_top_p(TrtLlmRequest* req, float p);

    /** Enable/disable streaming mode (partial results) */
    void trt_request_set_streaming(TrtLlmRequest* req, bool streaming);

    /** Set end-of-sequence token ID */
    void trt_request_set_end_id(TrtLlmRequest* req, int32_t end_id);

    /** Set client ID for KV cache reuse across requests */
    void trt_request_set_client_id(TrtLlmRequest* req, uint64_t client_id);

    /**
     * Destroy a request and free its resources.
     *
     * @param req Request handle to destroy
     */
    void trt_destroy_request(TrtLlmRequest* req);

    // ========================================================================
    // Execution Control
    // ========================================================================

    /**
     * Enqueue a request for execution.
     *
     * @param executor Executor handle
     * @param request Request to execute
     * @return Request ID (non-zero on success, 0 on failure)
     */
    uint64_t trt_executor_enqueue(TrtLlmExecutor* executor, TrtLlmRequest* request);

    /**
     * Cancel a pending or running request.
     *
     * @param executor Executor handle
     * @param request_id Request ID to cancel
     */
    void trt_executor_cancel_request(TrtLlmExecutor* executor, uint64_t request_id);

    /**
     * Wait for and retrieve responses for a request.
     * Blocks until at least one response is available.
     *
     * @param executor Executor handle
     * @param request_id Request ID to wait for
     * @return Response list, or NULL on failure
     */
    TrtLlmResponseList* trt_executor_await_responses(TrtLlmExecutor* executor, uint64_t request_id);

    // ========================================================================
    // Response Processing
    // ========================================================================

    /** Get the number of responses in a response list */
    int trt_response_list_size(TrtLlmResponseList* list);

    /** Check if a response contains an error */
    bool trt_response_has_error(TrtLlmResponseList* list, int index);

    /** Get error message from a response (if any) */
    char const* trt_response_get_error(TrtLlmResponseList* list, int index);

    /** Check if a response is the final one for the request */
    bool trt_response_is_final(TrtLlmResponseList* list, int index);

    /**
     * Extract generated tokens from a response.
     *
     * @param list Response list
     * @param index Response index within the list
     * @param out_buffer Buffer to write tokens to
     * @param max_size Maximum number of tokens to write
     * @return Number of tokens actually written
     */
    int trt_response_get_tokens(TrtLlmResponseList* list, int index, int32_t* out_buffer, int max_size);

    /**
     * Destroy a response list and free its resources.
     *
     * @param list Response list to destroy
     */
    void trt_destroy_response_list(TrtLlmResponseList* list);

#ifdef __cplusplus
}
#endif

#endif // TRTLLM_CAPI_H
