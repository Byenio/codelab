# Build

FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    python3 \
    ninja-build \
    autoconf \
    libtool \
    wget \
    && rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-linux-x86_64.sh \
    -q -O /tmp/cmake-install.sh \
    && chmod +x /tmp/cmake-install.sh \
    && /tmp/cmake-install.sh --skip-license --prefix=/usr/local \
    && rm /tmp/cmake-install.sh

WORKDIR /opt/vcpkg
RUN git clone https://github.com/microsoft/vcpkg.git . && \
    ./bootstrap-vcpkg.sh

WORKDIR /app
COPY vcpkg.json .
RUN /opt/vcpkg/vcpkg install --triplet x64-linux

COPY . .

RUN cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    -DCMAKE_BUILD_TYPE=Release \
    -GNinja

RUN cmake --build build --target codelab_server codelab_shell

# Runtime

FROM ubuntu:22.04

ENV DEBIAN_FRONTENT=noninteractive

RUN apt-get update && apt-get install -y \
    openssh-server \
    git \
    libsqlite3-0 \
    libssl3 \
    libcurl4 \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir /var/run/sshd
RUN useradd -m -d /home/git -s /bin/bash git
RUN passwd -d git

WORKDIR /app
RUN mkdir -p /app/data/repositories && \
    mkdir -p /app/data/db && \
    chown -R git:git /app/data

COPY --from=builder /app/build/backend/codelab_server /app/codelab_server
COPY --from=builder /app/build/backend/codelab_shell /app/codelab_shell
COPY --from=builder /app/backend/db/schema.sql /app/data/db/schema.sql

RUN mkdir -p /app/db
COPY --from=builder /app/backend/db/schema.sql /app/db/schema.sql

RUN sed -i 's/#AuthorizedKeysFile/AuthorizedKeysFile/' /etc/ssh/sshd_config
RUN sed -i 's/AuthorizedKeysFile.*/AuthorizedKeysFile .ssh\/authorized_keys/' /etc/ssh/sshd_config
RUN sed -i 's/#PasswordAuthentication yes/PasswordAuthentication no/' /etc/ssh/sshd_config

RUN echo "AllowUsers git" >> /etc/ssh/sshd_config

COPY entrypoint.sh /app/entrypoint.sh

RUN apt-get update && apt-get install -y dos2unix && dos2unix /app/entrypoint.sh && rm -rf /var/lib/apt/lists/*

RUN chmod +x /app/entrypoint.sh

EXPOSE 8080 22

CMD ["/app/entrypoint.sh"]