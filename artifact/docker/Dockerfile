# General information (FROM, ARG, LABEL)
# ------------------------------------------------------------------------------
FROM docker.io/ubuntu:22.04
LABEL maintainer="matti.schulze@fau.de"


# Build time arguments (ARG)
# ------------------------------------------------------------------------------

ARG USER_ID
ARG GROUP_ID


# Runtime arguments (ENV)
# ------------------------------------------------------------------------------

ENV USER_ID=$USER_ID
ENV GROUP_ID=$GROUP_ID


ENV DEBIAN_FRONTEND=noninteractive

ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

ENV PATH=/home/user/.local/bin:$PATH


# Image setup (ADD, RUN, COPY, WORKDIR, ONBUILD)
# ------------------------------------------------------------------------------

# Set the locale
RUN /bin/sh -c 'apt-get update \
    && apt-get install -y --no-install-recommends locales \
    && sed -i -e "s/# en_US.UTF-8 UTF-8/en_US.UTF-8 UTF-8/" /etc/locale.gen \
    && locale-gen \
    && rm -rf /var/lib/apt/lists/* \
    && rm -rf /etc/localtime \
    && ln -s /usr/share/zoneinfo/Europe/Berlin /etc/localtime'

# Add an user with sudo access
RUN /bin/sh -c 'apt-get update \
    && apt-get install -y --no-install-recommends \
      sudo \
    && rm -rf /var/lib/apt/lists/*'
RUN /bin/sh -c 'addgroup --gid $GROUP_ID user \
  && adduser --uid $USER_ID --home "/home/user" --gid $GROUP_ID \
    --shell "/bin/bash" --disabled-password --gecos "" user \
  && adduser user sudo \
  && echo "%sudo ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers'

# General system dependencies
# Taken from https://buildroot.org/downloads/manual/manual.html#requirement-mandatory
RUN /bin/sh -c 'apt-get update \
    && apt-get install -y --no-install-recommends \
      debianutils sed make build-essential binutils gcc g++ bash patch gzip bzip2 perl tar cpio unzip rsync file bc openssh-client \
      wget python3 \
      bzr cvs git mercurial subversion \
      libncurses5 libncurses5-dev asciidoc-base w3m \
      less vim nano \
      libssl-dev \
      openjdk-18-jdk \
      snapd \
      cmake \
      perl \
      ninja-build \
      bear \
    && rm -rf /var/lib/apt/lists/*'

RUN /bin/sh -c 'wget "https://developer.arm.com/-/media/Files/downloads/gnu/12.3.rel1/binrel/arm-gnu-toolchain-12.3.rel1-x86_64-aarch64-none-elf.tar.xz?rev=a8bbb76353aa44a69ce6b11fd560142d&hash=20124930455F791137DDEA1F0AF79B10" -O /arm-gnu-toolchain.xz && tar -xf /arm-gnu-toolchain.xz'

# RUN /bin/sh -c 'mkdir -p /arm-gnu-toolchain && tar -xf /arm-gnu-toolchain -C /arm-gnu-toolchain --strip-components=1'

# Privilege drop
USER $USER_ID:$GROUP_ID
WORKDIR /home/user

# Copy the entrypoint script to the image
COPY ./entrypoint.sh /usr/local/bin/entrypoint.sh


# Volume setup (VOLUME)
# ------------------------------------------------------------------------------


# Runtime setup (ENTRYPOINT, CMD, SHELL, EXPOSE, USER, STOPSIGNAL, HEALTCHECK)
# ------------------------------------------------------------------------------

ENV CROSS_COMPILE=/arm-gnu-toolchain-12.3.rel1-x86_64-aarch64-none-elf/bin/aarch64-none-elf-
ENTRYPOINT [ "/usr/local/bin/entrypoint.sh" ]
CMD [ "/bin/bash" ]
