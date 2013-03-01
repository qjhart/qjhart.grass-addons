#! /usr/bin/make
# in Debian 
#GH=$(pkg-config --variable=prefix grass) # /usr/lib/grass64
#GRASS_ADDON=~/grass
#mkdir -p ${GRASS_ADDON}/scripts 
#make GRASS_HOME=. MODULE_TOPDIR=${GH} DIST_DIR=${GRASS_ADDON} install

MODULE_TOPDIR = ../..
PGM = cg
include $(MODULE_TOPDIR)/include/Make/Script.make

scripts:=cg v.in.etxml r.heliosat cg.daily.k cg.daylight.intervals cg.previous.mapsets r.lowpass cg.proximate.mapsets cg.cgi cg.cgi.new cg.daily
ETCDIR=${DIST_DIR}/etc/${PGM}

default: install

${ETCDIR}:
	if [ ! -d $(ETCDIR) ]; then $(MKDIR) $(ETCDIR); fi

install: ${ETCDIR}
	$(INSTALL) etc/*.mk ${ETCDIR}
	${INSTALL} $(patsubst %,scripts/%,${scripts}) ${DIST_DIR}/scripts
