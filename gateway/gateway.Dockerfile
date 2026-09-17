# ---- build stage ----
FROM ubuntu:26.04 AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gcc-15 g++-15 \
    cmake \
    ninja-build \
    python3 python3-pip pipx \
    git \
    curl \
    && rm -rf /var/lib/apt/lists/*

RUN pipx install conan && pipx ensurepath
ENV PATH="/root/.local/bin:${PATH}"

WORKDIR /src

COPY conanfile.txt /src/conanfile.txt
COPY conan/profiles/linux-gcc-release.profile /src/conan/profiles/
COPY conan/scripts/install_conan_release.sh /src/conan/scripts/install_conan_release.sh
RUN chmod +x conan/scripts/install_conan_release.sh && \
    ./conan/scripts/install_conan_release.sh

COPY . /src

RUN cmake --preset release-gcc && \
    cmake --build --preset release-gcc --target gateway_service -j "$(nproc)"

# ---- runtime stage ----
FROM ubuntu:26.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=build /src/build/release-gcc/gateway/gateway_service /app/gateway_service
COPY gateway/gateway-service.json /app/gateway-service.json

EXPOSE 8081

ENTRYPOINT ["/app/gateway_service"]
CMD ["--config", "/app/gateway-service.json"]
