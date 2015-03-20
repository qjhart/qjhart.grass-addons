# Grass Addons

These are a number of grass commands that can be used for various
purposes.  Most of these are used for the CIMIS program, but they are
general enough to be used for other reasons.

## Compiling

Each of the directories in the grass-addons source need to be compiled
seperately, so the first step is to cd to the diretory that you want.

```
#!bash
# First install the grass-addons
hg clone https://qjhart@code.google.com/p/qjhart.grass-addons/ 
cd qjhart.grass-addons/r.in.gvar
```

If you want to keep your grass binaries in a directory separate from
the standard distribution, which is probably a good idea, then you can
install into a GRASS_ADDON directory, and add that to your path.  In
this case, you don't need to have sudo permissions.  Also, the
GRASS_ADDON directory can be shared with any number of computers using
the same OS and distro. This should work for both binaries and
scripts.  This example shows compiling for Debian, there the RedHat
version, is slightly more complicated, and shown below.

```
#!bash
GH=$(pkg-config --variable=prefix grass) # /usr/lib/grass64
GHLIB=${GH}/lib # RedHat (below) uses a different GHLIB value
GRASS_ADDON=~/grass
mkdir -p ${GRASS_ADDON}/bin ${GRASS_ADDON}/scripts
make GRASS_HOME=. MODULE_TOPDIR=${GH} DISTDIR=${GRASS_ADDON} ARCH_DISTDIR=${GRASS_ADDON} \
     ARCH_INC=-I${GH}/include ARCH_LIBDIR=${GHLIB} cmd
```

Then add those paths to your environment, (like in your .bashrc file eg.) when running grass.

```
#!bash
export GRASS_ADDON_PATH=${GRASS_ADDON}/bin:${GRASS_ADDON}/scripts
export GRASS_ADDON_ETC=${GRASS_ADDON}/etc
```

###  RedHat

Although REDHAT provides grass development files, this is not
sufficient to compile grass add-ons, as some of the development files
are missing from the installation, see for example,
(https://bugzilla.redhat.com/show_bug.cgi?format=multiple&id=855524
Bug 855524).  This includes the architecture specific grass makefiles
used when compiling the grass code.  Therefore grass must be compiled
from source.  There are many online documents on compiling source RPM
packages, including: (http://wiki.centos.org/HowTos/RebuildSRPM
HowTos/RebuildSRPM), which can be investigated.  Even though grass
needs to be compiled to install the add-ons.  The standard grass RPMs
can be used for the operation.  That is, the compilation of grass
below, is simply used to create the files needed to successfully
compile the grass add-ons.

```
#!bash
# First get the grass source
version=6.4.1
yumdownloader --source grass-${version}
#Setup your build environment ( this uses home directory)
sudo yum install rpm-build redhat-rpm-config
mkdir -p ~/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
echo '%_topdir %(echo $HOME)/rpmbuild' > ~/.rpmmacros
# Build the package (if this fails you made need to install more dependencies)
sudo yum install blas-devel fftw2-devel lapack-devel libjpeb-devel \ 
libXmu-devel mesa-libGLw-devel mysql-devel ncurses-devel \
postgresql-devel python-devel readline-devel sqlite-devel \
tk-devel unixODBC-devel
rpmbuild --recompile grass-6.4.1-3.el6.src.rpm
```
 
Since we are using the standard libraries, but using the compilation
files from the source build, the compilation requires that the proper
paths are set.  The following steps install the r.in.gvar package.
Note that the r.in.gvar package is installed in the ~/grass
subdirectory of the cimis user.  Extra directories to be included in
the grass, can be set by setting the environment parameter,
GRASS_ADDON_PATH.

```
#!bash
version=6.4.1
GH=~/rpmbuild/BUILD/grass-${version}
GHLIB=/usr/lib64
GRASS_ADDON=~/grass
mkdir -p ${GRASS_ADDON}/bin ${GRASS_ADDON}/scripts
make GRASS_HOME=. MODULE_TOPDIR=${GH} DISTDIR=${GRASS_ADDON} ARCH_DISTDIR=${GRASS_ADDON} \
     ARCH_INC=-I${GH}/include ARCH_LIBDIR=${GHLIB} cmd 
```

### Debian

The grass-dev package of debian (6.4) is slightly broken. The good new
however is that mostly it's just directory problems, and you can
compile the binaries with some extra care for the directory naming.

To install in a user-defined location,

```
#!bash
GH=$(pkg-config --variable=prefix grass) # /usr/lib/grass64
GHLIB=${GH}/lib
GRASS_ADDON=~/grass
mkdir -p ${GRASS_ADDON}/bin ${GRASS_ADDON}/scripts
make GRASS_HOME=. MODULE_TOPDIR=${GH} DISTDIR=${GRASS_ADDON} ARCH_DISTDIR=${GRASS_ADDON} \
     ARCH_INC=-I${GH}/include ARCH_LIBDIR=${GHLIB} cmd
```

If instead you'd like to install into the system diretories, the
following commands can be used to compile and install the grass
add-ons in the system standard location.

```
#!bash
GH=$(pkg-config --variable=prefix grass) # /usr/lib/grass64
sudo make GRASS_HOME=. MODULE_TOPDIR=${GH} ARCH_INC=-I${GH}/include ARCH_LIBDIR=${GH}/lib cmd
```

This puts your binaries into the standard GRASS distribution
location. The doc builder stuff doesn't work for a few reasons, but
there is no separate binary install.

Note that installing the grass-dev package is no guarantee that the
-devs neeeded to compile grass are included, so you will probably have
to install some of your own devs as well.
