# [GLOBAL ARG]
ARG TRT_LLM_VERSION=latest

# ==============================================================================
# STAGE 1: Builder (Download CLion)
# ==============================================================================
FROM ubuntu:24.04 AS builder

ARG CLION_VERSION=2025.2.1
ENV CLION_VERSION=${CLION_VERSION}

RUN apt-get update && \
    apt-get install -y --no-install-recommends curl ca-certificates tar gzip && \
    rm -rf /var/lib/apt/lists/*

ARG CLION_INSTALL_DIR=/opt/jetbrains/clion
RUN mkdir -p ${CLION_INSTALL_DIR}
RUN echo "Downloading CLion ${CLION_VERSION}..." && \
    curl --http1.1 -fL --retry 5 --retry-all-errors --retry-delay 5 \
    "https://download.jetbrains.com/cpp/CLion-${CLION_VERSION}.tar.gz" \
    -o /tmp/clion.tar.gz && \
    tar -xzf /tmp/clion.tar.gz -C ${CLION_INSTALL_DIR} --strip-components=1 && \
    rm /tmp/clion.tar.gz

# ==============================================================================
# STAGE 2: Final Image (Native NVIDIA Environment)
# ==============================================================================
FROM nvcr.io/nvidia/tensorrt-llm/devel:${TRT_LLM_VERSION}

# --- 1. System Tools ---
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    openssh-server sudo nano unzip rsync locales passwd \
    python3-pip python3-venv \
    && rm -rf /var/lib/apt/lists/*

RUN locale-gen en_US.UTF-8

# --- 2. Unlock Python (Mandatory for Ubuntu 24.04) ---
# The build_wheel.py script attempts to install 'virtualenv' to the system python.
# We must remove the PEP 668 lock file to allow this.
RUN rm -f /usr/lib/python3.*/EXTERNALLY-MANAGED

# --- 3. SSH Configuration ---
RUN sed -i 's/#PasswordAuthentication yes/PasswordAuthentication yes/' /etc/ssh/sshd_config && \
    mkdir -p /var/run/sshd && \
    ssh-keygen -A

# --- 4. User Configuration ---
RUN (userdel -r ubuntu 2>/dev/null || true) && \
    (groupdel ubuntu 2>/dev/null || true) && \
    groupadd -g 1000 developer && \
    useradd -u 1000 -g developer -m -s /bin/bash developer && \
    echo 'developer:password' | chpasswd && \
    usermod -aG sudo developer && \
    echo '%developer ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# --- 5. Environment Inheritance ---
# Ensure developer inherits root's NVIDIA environment variables
RUN echo "export PATH=${PATH}" >> /etc/profile && \
    echo "export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}" >> /etc/profile && \
    echo "export CUDA_HOME=${CUDA_HOME}" >> /etc/profile && \
    echo "export HPCX_DIR=${HPCX_DIR}" >> /etc/profile && \
    echo "export OPAL_PREFIX=${OPAL_PREFIX}" >> /etc/profile

# --- 6. Install CLion ---
ARG CLION_INSTALL_DIR=/opt/jetbrains/clion
COPY --from=builder ${CLION_INSTALL_DIR} ${CLION_INSTALL_DIR}

# --- 7. Register IDE Backend ---
USER developer
WORKDIR /home/developer
RUN touch /home/developer/.hushlogin && \
    ${CLION_INSTALL_DIR}/bin/remote-dev-server.sh registerBackendLocationForGateway

# --- 8. Entrypoint ---
USER root
RUN mkdir -p /code/tensorrt_llm
COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

EXPOSE 22
ENTRYPOINT ["entrypoint.sh"]
