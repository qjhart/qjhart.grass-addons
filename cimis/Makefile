#! /usr/bin/make
# in Debian make ARCH_DISTDIR=/usr/lib/grass64 MODULE_TOPDIR=/usr/lib/grass64 DOCSDIR=docs MANDIR=man HTMLDIR=docs/html install
MODULE_TOPDIR = ../..
PGM = cg
include $(MODULE_TOPDIR)/include/Make/Script.make

scripts:=cg v.in.etxml r.heliosat cg.daily.k cg.daylight.intervals cg.previous.mapsets r.lowpass cg.proximate.mapsets
ETCDIR=${DIST_DIR}/etc/${PGM}

default: install

${ETCDIR}:
	if [ ! -d $(ETCDIR) ]; then $(MKDIR) $(ETCDIR); fi

install: ${ETCDIR}
	$(INSTALL) etc/*.mk ${ETCDIR}
	${INSTALL} $(patsubst %,scripts/%,${scripts}) ${DIST_DIR}/scripts
