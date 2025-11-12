# Dockerfile for Circ.io Game Server
# Multi-stage build for smaller final image

# Build stage
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libsodium-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build the server
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . --target game_server -j$(nproc)

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libsodium23 \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user for security
RUN useradd -m -u 1000 gameserver

WORKDIR /app

# Copy only the built server binary
COPY --from=builder /app/build/server/game_server .

# Change ownership to non-root user
RUN chown -R gameserver:gameserver /app

# Switch to non-root user
USER gameserver

# Expose the game server port
# Heroku will set PORT environment variable, default to 40000
EXPOSE 40000

# Run the server
# Use CMD instead of ENTRYPOINT to allow Heroku to override
CMD ["./game_server"]
