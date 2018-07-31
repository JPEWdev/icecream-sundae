# icecream-sundae
Commandline Monitor for [Icecream](https://github.com/icecc/icecream).

[![Build Status](https://travis-ci.org/JPEWdev/icecream-sundae.svg?branch=master)](https://travis-ci.org/JPEWdev/icecream-sundae)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/9e75106b1701452488d361efe2d1ad88)](https://www.codacy.com/app/JPEWdev/icecream-sundae?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=JPEWdev/icecream-sundae&amp;utm_campaign=Badge_Grade)
[![Coverage Status](https://coveralls.io/repos/github/JPEWdev/icecream-sundae/badge.svg?branch=master)](https://coveralls.io/github/JPEWdev/icecream-sundae?branch=master)

# Building

## Prerequsites
* [ncurses](https://www.gnu.org/software/ncurses/ncurses.html)
* [glib](https://developer.gnome.org/glib/stable/) >= 2.0
* [Icecream](https://github.com/icecc/icecream)
* [meson](http://mesonbuild.com/)
* [ninja](https://ninja-build.org/)

### Fedora 27 & 28
```shell
sudo dnf install gcc-c++ glib2-devel icecream-devel meson ncurses-devel ninja-build
```

### Ubuntu 17.10 (Artful Aardvark) & 18.04 (Bionic Beaver)
```shell
sudo apt-get install g++ libcap-ng-dev libglib2.0-dev libicecc-dev liblzo2-dev libncursesw5-dev meson ninja-build
```

### Ubuntu 16.04 (Xenial Xerus)
This version of Ubuntu requires a newer version of meson:

```shell
sudo apt-get install g++ libcap-ng-dev libglib2.0-dev libicecc-dev liblzo2-dev libncursesw5-dev meson python3-pip ninja-build
pip3 install --user meson
```

### Ubuntu 14.04 (Trusty Tahr)
While it is possible to install on this ancient version of Ubuntu, it requires a lot of work:

```shell
sudo add-apt-repository ppa:jonathonf/python-3.5
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo add-apt-repository ppa:jonathonf/binutils

sudo apt-get update

sudo apt-get install g++-6 libcap-ng-dev libglib2.0-dev libicecc-dev liblzo2-dev libncursesw5-dev python3-pip python3.5 wget unzip

wget https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-linux.zip -O ninja-linux.zip
sudo unzip ninja-linux.zip -d /usr/local/bin

python3.5 -m pip install --user meson

export CXX=g++-6
```

## Compiling
To build icecream-sundae, download the latest release, extract it, then run:
```
mkdir builddir
cd builddir
meson .. --buildtype release
ninja
sudo -E ninja install
```

*Note:* For Ubuntu 16.04 & 14.04, you may need to run `meson` as `~/.local/bin/meson`

# Running

Simply running `icecream-sundae` should be sufficent. Without any arguements, the program will try to
discover the default scheduler. For help, run `icecream-sundae --help`

## Display


### Columns

The following table describes the columns that are shown for each compile node

| Column        | Description                                                                       |
|---------------|-----------------------------------------------------------------------------------|
| `ID`          | The unique ID for the node, as assigned by the scheduler                          |
| `NAME`        | The Name of the node. Each node is assigned a color based on a hash of its string name. Nodes that cannot accept remote jobs (i.e. have the "NoRemote" property set to "true") are displayed underlined. |
| `IN`          | The total number of jobs this node has compiled for other nodes                   |
| `CUR`         | The current number of jobs this node is compiling                                 |
| `MAX`         | The maximum number of jobs the node can compile at once                           |
| `JOBS`        | A graph of the current jobs (see below)                                           |
| `OUT`         | The total number of jobs this node has sent to other nodes to be compiled remotely|
| `LOCAL`       | The total number of jobs this node as compiled locally                            |
| `ACTIVE`      | The current number of jobs this node has working on the cluster                   |
| `PENDING`     | The number of jobs this node has that are waiting to be assigned a node           |
| `SPEED`       | The speed of the node. This is measured by the nodes as the KB/sec of source compiled by the node |

### Job Graphs
Jobs in the job bar graphs are displayed using the following legend:

| Character | Meaning               |
|-----------|-----------------------|
| `%`       | Local compile job     |
| `=`       | Remote compile job    |

The color of the job marker matches the color of the source node

*Note:* If there are nodes on the cluster that do not accept remote jobs, it is entirely possible that there can be
more "Active" jobs than "Maximum" slots, if those nodes are doing local compiles

## Key bindings

| Key(s)            | Action                                                |
|-------------------|-------------------------------------------------------|
| `down arrow`, `j` | Move highlight down to next host                      |
| `up arrow`, `k`   | Move highlight up to previous host                    |
| `left arrow`, `h` | Move sort left one column                             |
| `right arrow`, `l`| Move sort right one column                            |
| `tab`             | Move sort right one column (wraps)                    |
| `space`           | Toggle host details                                   |
| `a`               | Toggle all host details                               |
| `r`               | Reverse sort                                          |
| `q`               | Quit                                                  |
| `T`               | Toggle job tracking (shows jobs with unique letter)   |

# Notes

C++ is not my most fluent language... Apologies for any travesties I've committed.

# TODO

* User configurable columns (The code supports this, I just need to figure out a good UI)
* "Managment" interface. It would be nice to be able to blacklist servers and such via a menu that communicates with the scheduler telnet interface
* Better unit tests
* Scrolling in the host view
