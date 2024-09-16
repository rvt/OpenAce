# Fetch ubuntu image
FROM ubuntu:22.04

# Install prerequisites
RUN \
    apt update && \
    apt install -y git python3 ninja-build cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential libstdc++-arm-none-eabi-newlib libusb-dev

# Install Pico SDK
RUN \
    cd /opt/ && \
    git clone https://github.com/raspberrypi/pico-sdk.git --branch master && \
    cd pico-sdk/ && \
    git submodule update --init && \
    cd /

RUN \
    cd /opt/ && \
    git clone https://github.com/FreeRTOS/FreeRTOS.git --branch main && \
    cd FreeRTOS/ && \
    git submodule update --init && \
    cd /

RUN \
    mkdir -p /opt/src/

# Set Environments
ENV PICO_SDK_PATH=/opt/pico-sdk/
ENV FREERTOS_KERNEL_PATH=/opt/FreeRTOS/FreeRTOS/Source

#
# # Create entrypoint script that will be used at container start
RUN echo '#!/bin/bash\n\
  echo "Using ninja"\n\
  mkdir -p /opt/src/pico/dockerbuild\n\
  cd /opt/src/pico\n\
  cmake -B dockerbuild -G Ninja && ninja -C dockerbuild $1\n\
  exit' > /entrypoint.sh

RUN chmod +x /entrypoint.sh

# Use the custom entrypoint script to build and exit
ENTRYPOINT ["/entrypoint.sh"]
