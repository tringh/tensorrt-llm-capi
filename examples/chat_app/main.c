#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "trtllm_capi.h"

int main(int argc, char* argv[]) {
    printf("--- TensorRT-LLM C Interface Chatbot ---\n");

    // 1. Configuration
    const char* model_path = "/code/tensorrt_llm_capi/engines/gpt2";

    if (argc > 1) {
        model_path = argv[1];
    }

    printf("Loading model from: %s\n", model_path);

    // 2. Initialize Executor
    // max_beam_width = 1 means "Greedy Search" (fastest).
    TrtLlmExecutor* engine = trt_create_executor(model_path, TRT_MODEL_TYPE_DECODER_ONLY, 1);

    if (!engine) {
        fprintf(stderr, "Error: Failed to initialize Executor. Check model path.\n");
        return 1;
    }
    printf("Model loaded successfully.\n");

    // 3. Prepare Input Data
    // Example: "Hello world" tokens.
    // These IDs are hypothetical examples.
    int32_t input_ids[] = {15496, 995, 13}; // "Hello" " world" "."
    int input_len = 3;
    int max_new_tokens = 20;

    // 4. Create Request
    TrtLlmRequest* req = trt_create_request(input_ids, input_len, max_new_tokens);

    // Optional: Set decoding parameters
    trt_request_set_temperature(req, 0.95f);
    trt_request_set_top_k(req, 1);
    trt_request_set_streaming(req, true); // Enable streaming to see tokens as they arrive

    // Optional: Set a Client ID for session tracking (useful for the KV cache reuse)
    trt_request_set_client_id(req, 1001);

    printf("Enqueuing request...\n");

    // 5. Enqueue
    uint64_t req_id = trt_executor_enqueue(engine, req);

    if (req_id == 0) {
        fprintf(stderr, "Error: Failed to enqueue request.\n");
        trt_destroy_request(req);
        trt_destroy_executor(engine);
        return 1;
    }

    printf("Request ID: %lu. Waiting for tokens...\n", req_id);

    // 6. Event Loop (Streaming)
    bool is_finished = false;

    while (!is_finished) {
        // Wait for any new response for this request
        TrtLlmResponseList* responses = trt_executor_await_responses(engine, req_id);

        if (!responses) {
            fprintf(stderr, "Error: Internal failure waiting for responses.\n");
            break;
        }

        int num_responses = trt_response_list_size(responses);

        for (int i = 0; i < num_responses; ++i) {
            // Check for errors
            if (trt_response_has_error(responses, i)) {
                const char* err = trt_response_get_error(responses, i);
                fprintf(stderr, "Response Error: %s\n", err);
                is_finished = true;
                continue;
            }

            // Check if this is the end of the stream
            if (trt_response_is_final(responses, i)) {
                is_finished = true;
            }

            // Retrieve generated tokens using the new two-step API
            int token_count = trt_response_get_token_count(responses, i);

            if (token_count > 0) {
                // Allocate exact buffer size needed
                int32_t* token_buffer = (int32_t*)malloc(token_count * sizeof(int32_t));
                if (!token_buffer) {
                    fprintf(stderr, "Error: Failed to allocate token buffer.\n");
                    continue;
                }

                int num_tokens = trt_response_get_tokens(responses, i, token_buffer, token_count);

                if (num_tokens > 0) {
                    printf("Received %d tokens: [ ", num_tokens);
                    for (int t = 0; t < num_tokens; ++t) {
                        printf("%d ", token_buffer[t]);
                    }
                    printf("]\n");
                    fflush(stdout); // Ensure output prints immediately
                } else if (num_tokens == -1) {
                    fprintf(stderr, "Error: Failed to retrieve tokens.\n");
                }

                free(token_buffer);
            } else if (token_count == -1) {
                fprintf(stderr, "Error: Failed to get token count.\n");
            }
        }

        // Cleanup the list object (but not the request or executor yet)
        trt_destroy_response_list(responses);
    }

    printf("\nGeneration complete.\n");

    // 7. Cleanup
    trt_destroy_request(req);
    trt_destroy_executor(engine);

    printf("Shutdown successful.\n");
    return 0;
}