FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ninja-build \
        gcc-aarch64-linux-gnu \
        g++-aarch64-linux-gnu \
        git \
        ca-certificates \
        curl \
        xz-utils \
        file \
        dpkg-dev \
        fakeroot \
        pkg-config \
        python3 \
    && rm -rf /var/lib/apt/lists/*

ARG CMAKE_VERSION=3.31.6
RUN curl -fsSL "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-aarch64.tar.gz" \
        | tar -xz -C /opt \
    && ln -s "/opt/cmake-${CMAKE_VERSION}-linux-aarch64/bin/cmake" /usr/local/bin/cmake \
    && ln -s "/opt/cmake-${CMAKE_VERSION}-linux-aarch64/bin/ctest" /usr/local/bin/ctest \
    && ln -s "/opt/cmake-${CMAKE_VERSION}-linux-aarch64/bin/cpack" /usr/local/bin/cpack

ARG GO_VERSION=1.27.3
RUN curl -fsSL "https://go.dev/dl/go${GO_VERSION}.linux-arm64.tar.gz" | tar -xz -C /usr/local

ENV PATH="/usr/local/go/bin:${PATH}"
