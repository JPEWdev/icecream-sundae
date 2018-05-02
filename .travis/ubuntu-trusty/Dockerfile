FROM ubuntu:trusty

RUN apt-get -y update && apt-get -y install \
    clang \
    g++ \
    libcap-ng-dev \
    libglib2.0-dev \
    libicecc-dev \
    liblzo2-dev \
    libncursesw5-dev \
    python3-pip \
    software-properties-common \
    unzip \
    wget

RUN add-apt-repository -y ppa:jonathonf/python-3.5
RUN add-apt-repository -y ppa:ubuntu-toolchain-r/test
RUN add-apt-repository -y ppa:jonathonf/binutils
RUN apt-get -y update && apt-get -y install \
    g++-6 \
    python3.5 \
    clang-3.8

RUN apt-get -y update && apt-get -y --force-yes install binutils

RUN update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-6 100
RUN update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-3.8 100

RUN python3.5 -m pip install --user meson
ENV PATH="/root/.local/bin:${PATH}"

# Install pre-built ninja
RUN wget https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-linux.zip -O ninja-linux.zip
RUN unzip ninja-linux.zip -d /usr/local/bin

RUN mkdir -p /root/icecream-sundae/builddir
COPY . /root/icecream-sundae/
WORKDIR /root/icecream-sundae/builddir
