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

# Кэшируем conan install
COPY conanfile.txt /src/conanfile.txt
COPY conan/profiles/linux-gcc-release.profile /src/conan/profiles/
COPY conan/scripts/install_conan_release.sh /src/conan/scripts/install_conan_release.sh
RUN chmod +x conan/scripts/install_conan_release.sh && \
    ./conan/scripts/install_conan_release.sh

COPY . /src

RUN cmake --preset release-gcc && \
    cmake --build --preset release-gcc --target order_service -j "$(nproc)"

# ---- runtime stage ----
FROM ubuntu:26.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 \
    librdkafka1 \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=build /src/build/release-gcc/order-service/order_service /app/order_service
COPY order-service/order-service.json /app/order-service.json

EXPOSE 8080

ENTRYPOINT ["/app/order_service"]
CMD ["--config", "/app/order-service.json"]
