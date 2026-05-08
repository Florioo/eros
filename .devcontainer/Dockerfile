FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        gcc \
        g++ \
        clang \
        clang-format \
        clang-tidy \
        lldb \
        gdb \
        lcov \
        git \
        ca-certificates \
        curl \
        pkg-config \
        python3 \
        python3-venv \
    && rm -rf /var/lib/apt/lists/*

# Unity C test framework — used by the host C tests (test/CMakeLists.txt picks
# it up via $UNITY_DIR).
ARG UNITY_REF=v2.6.0
ENV UNITY_DIR=/opt/Unity
RUN git clone --depth 1 --branch ${UNITY_REF} \
        https://github.com/ThrowTheSwitch/Unity.git ${UNITY_DIR} \
    && rm -rf ${UNITY_DIR}/.git

# Install uv (Python package/project manager) — pinned by digest for reproducibility
COPY --from=ghcr.io/astral-sh/uv@sha256:2381d6aa60c326b71fd40023f921a0a3b8f91b14d5db6b90402e65a635053709 /uv /uvx /bin/

USER vscode
RUN mkdir -p /home/vscode/.claude \
    && ln -sf /home/vscode/.claude/claude.json /home/vscode/.claude.json
ENV PATH="/home/vscode/.local/bin:${PATH}"
USER root

WORKDIR /workspaces/eros
