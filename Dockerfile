# LumVorax Dockerfile - Production Ready
# Multi-stage build for optimized image size
# Version: 1.0.0

# ============================================================================
# Stage 1: Builder - Compile LumVorax
# ============================================================================
FROM ubuntu:22.04 AS builder

# Metadata
LABEL maintainer="LumVorax Team <contact@lumvorax.io>"
LABEL version="1.0.0"
LABEL description="LumVorax Quantum Computing Distributed System"

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    g++ \
    make \
    cmake \
    git \
    libpthread-stubs0-dev \
    libssl-dev \
    libm-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Build LumVorax
RUN make clean && \
    make all -j$(nproc) && \
    make test

# Verify build
RUN ls -lh /build/bin/ && \
    file /build/bin/lumvorax_main

# ============================================================================
# Stage 2: Runtime - Minimal production image
# ============================================================================
FROM ubuntu:22.04 AS runtime

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libpthread-stubs0-dev \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create non-root user for security
RUN groupadd -r lumvorax && \
    useradd -r -g lumvorax -s /bin/bash -d /var/lib/lumvorax lumvorax

# Create directories
RUN mkdir -p /var/lib/lumvorax/data \
             /var/lib/lumvorax/logs \
             /var/lib/lumvorax/config \
             /etc/lumvorax && \
    chown -R lumvorax:lumvorax /var/lib/lumvorax /etc/lumvorax

# Copy binaries from builder
COPY --from=builder /build/bin/* /usr/local/bin/
COPY --from=builder /build/lib/* /usr/local/lib/

# Copy configuration files
COPY --from=builder /build/config/*.conf /etc/lumvorax/

# Update library cache
RUN ldconfig

# Set working directory
WORKDIR /var/lib/lumvorax

# Switch to non-root user
USER lumvorax

# Expose ports
# 8080: HTTP API
# 8081: Metrics (Prometheus)
# 9000: P2P Network
# 5432: Database (if standalone)
EXPOSE 8080 8081 9000 5432

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=3 \
    CMD /usr/local/bin/lumvorax_healthcheck || exit 1

# Environment variables
ENV LUMVORAX_HOME=/var/lib/lumvorax \
    LUMVORAX_CONFIG=/etc/lumvorax/lumvorax.conf \
    LUMVORAX_LOG_LEVEL=INFO \
    LUMVORAX_CACHE_SIZE=1000 \
    LUMVORAX_NUM_WORKERS=4

# Volume for persistent data
VOLUME ["/var/lib/lumvorax/data", "/var/lib/lumvorax/logs"]

# Entry point
ENTRYPOINT ["/usr/local/bin/lumvorax_main"]

# Default command (can be overridden)
CMD ["--config", "/etc/lumvorax/lumvorax.conf", "--daemon"]