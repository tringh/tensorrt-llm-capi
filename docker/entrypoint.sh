#!/bin/bash
set -e
WORKSPACE_PATH="${WORKSPACE_PATH:-/code/tensorrt_llm}"
echo "Starting Native TensorRT-LLM Dev Container..."

# Sync Permissions
if [ -d "$WORKSPACE_PATH" ]; then
    HOST_UID=$(stat -c "%u" "$WORKSPACE_PATH")
    HOST_GID=$(stat -c "%g" "$WORKSPACE_PATH")
    CURRENT_UID=$(id -u developer)
    if [ "$CURRENT_UID" != "$HOST_UID" ]; then
        echo "Syncing UID to Host ($HOST_UID)..."
        groupmod -o -g "$HOST_GID" developer
        usermod -o -u "$HOST_UID" developer
        chown -R developer:developer /home/developer
    fi
fi

# Start SSH
exec /usr/sbin/sshd -D -e
