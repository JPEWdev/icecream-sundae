FROM ubuntu:rolling

RUN apt-get -y update && apt-get -y install \
    clang \
    g++ \
    libglib2.0-dev \
    libicecc-dev \
    libncursesw5-dev \
    meson \
    python3-pip \
    ninja-build

RUN pip3 install meson

RUN mkdir -p /root/icecream-sundae/builddir
COPY . /root/icecream-sundae/
WORKDIR /root/icecream-sundae/builddir/

