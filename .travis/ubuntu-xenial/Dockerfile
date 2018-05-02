FROM ubuntu:xenial

RUN apt-get -y update && apt-get -y install \
    clang \
    g++ \
    libcap-ng-dev \
    libglib2.0-dev \
    libicecc-dev \
    liblzo2-dev \
    libncursesw5-dev \
    meson \
    python3-pip \
    ninja-build

RUN pip3 install --user meson
ENV PATH="/root/.local/bin:${PATH}"

RUN mkdir -p /root/icecream-sundae/builddir
COPY . /root/icecream-sundae/
WORKDIR /root/icecream-sundae/builddir/

