# icecream-sundae
Commandline Monitor for [Icecream](https://github.com/icecc/icecream).

# Building

## Prerequsites
* [ncurses](https://www.gnu.org/software/ncurses/ncurses.html)
* [glib](https://developer.gnome.org/glib/stable/) >= 2.0
* [Icecream](https://github.com/icecc/icecream)
* [meson](http://mesonbuild.com/)
* [ninja](https://ninja-build.org/)

### Fedora
`dnf install dnf install glib2-devel ncurses-devel icecream-devel meson ninja-build`

### Debian/Ubuntu
`apt-get install ninja-build meson libncursesw5-dev libicecc-dev libglib2.0-dev`

## Compiling
To build icecream-sundae, download the latest release, extract it, then run:
```
mkdir builddir
cd builddir
meson .. --buildtype release
ninja
sudo -E ninja install
```

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
