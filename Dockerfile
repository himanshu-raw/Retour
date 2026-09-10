# Stage 1: Build the C++ engine
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libssl-dev \
    libcurl4-openssl-dev \
    curl \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the source code
COPY . .

# Build the project
RUN mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && cmake --build .

# Stage 2: Create the runtime image
FROM ubuntu:22.04

# Install runtime dependencies (curl and SSL are needed for TomTom requests)
RUN apt-get update && apt-get install -y \
    libssl3 \
    libcurl4 \
    curl \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the built executable and the public web directory from the builder
COPY --from=builder /app/build/retour_app ./build/retour_app
COPY --from=builder /app/public ./public
# Create the data directory for the SQLite database
RUN mkdir -p data

# Expose the API and Web port
EXPOSE 8080

# Start the C++ Backend Engine
CMD ["./build/retour_app"]
