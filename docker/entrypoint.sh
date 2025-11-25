#!/bin/bash
set -e

# 1. Determine the Workspace Path
if [ -n "$WORKSPACE_PATH" ]; then
    # Case A: User explicitly set the variable
    TARGET_DIR="$WORKSPACE_PATH"
else
    # Case B: Auto-detect the first directory inside /code
    # (This finds whatever name you bound: /code/my-app, /code/test, etc.)
    TARGET_DIR=$(find /code -maxdepth 1 -mindepth 1 -type d -print -quit)
fi

# Fallback if nothing detected (e.g. user didn't mount anything yet)
TARGET_DIR="${TARGET_DIR:-/code/project}"

echo "Starting Dev Container..."
echo "Detected Workspace: $TARGET_DIR"

# 2. Sync Permissions
if [ -d "$TARGET_DIR" ]; then
    HOST_UID=$(stat -c "%u" "$TARGET_DIR")
    HOST_GID=$(stat -c "%g" "$TARGET_DIR")
    CURRENT_UID=$(id -u developer)

    # Only sync if there is a mismatch AND we are not looking at a root-owned system folder
    if [ "$CURRENT_UID" != "$HOST_UID" ] && [ "$HOST_UID" != "0" ]; then
        echo "Syncing UID to Host ($HOST_UID)..."
        groupmod -o -g "$HOST_GID" developer
        usermod -o -u "$HOST_UID" developer
        chown -R developer:developer /home/developer

        # Optional: update permission on the parent /code if needed
        chown developer:developer /code
    fi
else
    echo "Note: Workspace directory not found. Skipping permission sync."
fi

# 3. Start SSH
exec /usr/sbin/sshd -D -e