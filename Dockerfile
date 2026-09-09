FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install base dependencies and LLVM GPG keyring setup
RUN apt-get update && apt-get install -y \
    wget \
    gnupg \
    lsb-release \
    software-properties-common \
    curl \
    git \
    cmake \
    ninja-build \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Add official LLVM 22 APT repository
RUN wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - && \
    add-apt-repository "deb http://apt.llvm.org/noble/ llvm-toolchain-noble-22 main" && \
    apt-get update && apt-get install -y \
    clang-22 \
    clang-format-22 \
    clang-tidy-22 \
    clangd-22 \
    lldb-22 \
    lld-22 \
    && rm -rf /var/lib/apt/lists/*

# Set Clang 22 binaries as default system commands
RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-22 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-22 100 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-22 100 && \
    update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-22 100 && \
    update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-22 100

# Install Node.js 22 LTS & pnpm/yarn for React Frontend development
RUN curl -fsSL https://deb.nodesource.com/setup_22.x | bash - && \
    apt-get install -y nodejs && \
    npm install -g pnpm yarn && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Expose backend (18080) and React Vite Dev Server (5173) ports
EXPOSE 18080 5173

CMD ["/bin/bash"]
