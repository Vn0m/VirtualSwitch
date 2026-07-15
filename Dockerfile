FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    pkg-config \
    libsodium-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY CMakeLists.txt .
COPY src/ src/
COPY include/ include/
COPY relay.cpp .

RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
RUN g++ -std=c++17 relay.cpp -o /relay

FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libsodium23 \
    iproute2 \
    iputils-ping \
    python3 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/vswitch /usr/local/bin/vswitch
COPY --from=builder /relay /usr/local/bin/relay
COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
