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

# Install uv (Python package/project manager) — pinned by digest for reproducibility
COPY --from=ghcr.io/astral-sh/uv@sha256:2381d6aa60c326b71fd40023f921a0a3b8f91b14d5db6b90402e65a635053709 /uv /uvx /bin/

# Install Claude Code (native installer — no Node.js required) and set up
# persistent state layout. Both ~/.claude/ and ~/.claude.json need to survive
# rebuilds; we make ~/.claude.json a symlink into ~/.claude/, so a single named
# volume mounted at ~/.claude/ persists everything. Creating the directory here
# (as vscode) means the volume inherits vscode ownership on first mount.
USER vscode
# RUN curl -fsSL https://claude.ai/install.sh | bash \
RUN mkdir -p /home/vscode/.claude \
    && ln -sf /home/vscode/.claude/claude.json /home/vscode/.claude.json
ENV PATH="/home/vscode/.local/bin:${PATH}"
USER root

WORKDIR /workspaces/eros
