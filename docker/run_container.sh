docker run -d \
  --gpus all \
  --ipc=host \
  --ulimit memlock=-1 \
  --ulimit stack=67108864 \
  -p 2522:22 \
  -v .:/code/tensorrt_llm_capi \
  -v .:/code/tensorrt_llm \
  --name trtllm-capi-dev \
  htring/trtdev:1.0.0
