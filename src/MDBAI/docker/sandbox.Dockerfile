FROM node:20-alpine

# Sécurité: utilisateur non-root
RUN addgroup -g 1000 sandbox && \
    adduser -D -u 1000 -G sandbox sandbox

# Outils nécessaires
RUN apk add --no-cache \
    git \
    python3 \
    py3-pip \
    cargo \
    gcc \
    g++ \
    make

# Répertoire de travail
WORKDIR /workspace
RUN chown sandbox:sandbox /workspace

# Copier libmdbai_forensic.so
COPY forensic/libmdbai_forensic.so /usr/local/lib/
ENV LD_PRELOAD=/usr/local/lib/libmdbai_forensic.so

# Timeout automatique 10 minutes
RUN echo '#!/bin/sh\ntimeout 600 "$@"' > /usr/local/bin/timeout-wrapper && \
    chmod +x /usr/local/bin/timeout-wrapper

USER sandbox
ENTRYPOINT ["/usr/local/bin/timeout-wrapper"]
CMD ["/bin/sh"]

# Made with Bob
