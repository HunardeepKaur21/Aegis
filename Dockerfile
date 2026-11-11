# Multi-stage Dockerfile for AEGIS C++ Crow Backend
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libsqlite3-dev \
    libasio-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY backend ./backend
COPY database ./database

WORKDIR /app/backend/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc) aegis_server

# Runtime stage
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libsqlite3-0 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/backend/build/bin/aegis_server ./aegis_server
COPY database ./database

EXPOSE 8080
ENV PORT=8080

CMD ["sh", "-c", "./aegis_server database/aegis.db database ${PORT}"]
