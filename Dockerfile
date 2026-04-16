
FROM ubuntu:22.04

LABEL maintainer="ReadMarkable Contributors"
LABEL description="AArch64 cross-compilation environment for reMarkable Paper Pro Move (chiappa SDK)"
LABEL org.opencontainers.image.source="https://github.com/readmarkable/readmarkable"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    ninja-build \
    curl \
    file \
    git \
    rsync \
    openssh-client \
    xz-utils \
    ca-certificates \
    python3 \
    && rm -rf /var/lib/apt/lists/*

ARG SDK_INSTALLER=remarkable-production-image-5.6.75-chiappa-public-x86_64-toolchain.sh
ARG SDK_INSTALL_DIR=/opt/codex/chiappa/5.6.75
ARG SDK_ENV_SETUP=environment-setup-cortexa55-remarkable-linux

COPY ${SDK_INSTALLER} /tmp/sdk-installer.sh
RUN chmod +x /tmp/sdk-installer.sh \
    && /tmp/sdk-installer.sh -y -d ${SDK_INSTALL_DIR} \
    && rm /tmp/sdk-installer.sh

RUN test -f ${SDK_INSTALL_DIR}/${SDK_ENV_SETUP} \
    || (echo "ERROR: SDK environment-setup script not found at ${SDK_INSTALL_DIR}/${SDK_ENV_SETUP}" && exit 1)

ENV SDK_ENV_PATH=${SDK_INSTALL_DIR}/${SDK_ENV_SETUP}

ENTRYPOINT ["/bin/bash", "-c", \
    "source $SDK_ENV_PATH && exec \"$@\"", \
    "--"]

CMD ["bash"]
