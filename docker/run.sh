docker run -d \
  --gpus all \
  --ipc=host \
  --ulimit memlock=-1 \
  --ulimit stack=67108864 \
  -p 2522:22 \
  -v $SSH_AUTH_SOCK:/ssh-agent \
  -e SSH_AUTH_SOCK:=/ssh-agent \
  -v ~/.gitconfig:/home/developer/.gitconfig \
  -v .:/code/tensorrt_llm_capi \
  --name trtllm-capi-dev \
  htring/trtdev:1.0.0
