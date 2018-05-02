FROM fedora:28

RUN dnf -y install \
    clang \
    gcc-c++ \
    glib2-devel \
    icecream-devel \
    libasan \
    libubsan \
    meson \
    ncurses-devel \
    ninja-build

RUN mkdir -p /root/icecream-sundae/builddir
COPY . /root/icecream-sundae/
WORKDIR /root/icecream-sundae/builddir/
