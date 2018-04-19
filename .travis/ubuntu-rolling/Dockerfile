FROM ubuntu:rolling

RUN apt-get -y update && apt-get -y install \
    g++ \
    libglib2.0-dev \
    libicecc-dev \
    libncursesw5-dev \
    meson \
    ninja-build

RUN mkdir -p /root/icecream-sundae/builddir
COPY . /root/icecream-sundae/
WORKDIR /root/icecream-sundae/builddir/

