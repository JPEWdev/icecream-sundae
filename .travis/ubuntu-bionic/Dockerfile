FROM ubuntu:bionic

RUN apt-get -y update && apt-get -y install \
    clang \
    g++ \
    libcap-ng-dev \
    libglib2.0-dev \
    libicecc-dev \
    liblzo2-dev \
    libncursesw5-dev \
    meson \
    ninja-build

# For coveralls
RUN apt-get -y update && \
    apt-get -y install \
        git-core \
        lcov \
        python-pip && \
    pip install --user cpp-coveralls
ENV PATH ~/.local/bin:$PATH

RUN mkdir -p /root/icecream-sundae/builddir
COPY . /root/icecream-sundae/
WORKDIR /root/icecream-sundae/builddir/

