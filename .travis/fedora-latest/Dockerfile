FROM fedora:latest

RUN dnf -y install \
    gcc-c++ \
    glib2-devel \
    icecream-devel \
    meson \
    ncurses-devel \
    ninja-build

RUN mkdir -p /root/icecream-sundae/builddir
COPY . /root/icecream-sundae/
WORKDIR /root/icecream-sundae/builddir/
