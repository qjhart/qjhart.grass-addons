#! /usr/bin/make -f

ifndef configure.mk
include configure.mk
endif

ca-goes.mk:=1

goes-loc:=$(shell pkg-config --variable=goes_loc cg)
ca-goes-loc:=$(shell pkg-config --variable=ca_goes_loc cg)
ca-save-pattern:=$(shell pkg-config --variable=ca_save_pattern cg)

mapsets:=$(shell cd ${goes-loc}; echo ${ca-save-pattern})

.PHONY: info

info::
	@echo ca-daily
	@echo "Copy GOES data into the California Projection"

ifneq (${LOCATION_NAME},$(notdir ${ca-goes-loc}))
  $(error LOCATION_NAME neq $(notdir ${ca-goes-loc}))
endif

ch1:=$(patsubst %,${ca-goes-loc}/%/cellhd/ch1,${mapsets})

ch1:${ch1}

${ch1}:${ca-goes-loc}/%/cellhd/ch1:${goes-loc}/%/cellhd/ch1
	@save=`g.gisenv MAPSET`; \
	(g.mapset -c $*;\
	r.proj location=$(notdir ${goes-loc}) mapset=$* input=ch1;) 2>/dev/null >/dev/null; \
	if [[ $$? != 0 ]] ; then \
	echo '$* to $(notdir ${ca-goes-loc})...failed'; \
	echo 'if bad rm -rf ${goes-loc}/$*)'; else \
	echo '$* to $(notdir ${ca-goes-loc})'; fi; \
	g.mapset $$save 2>/dev/null >/dev/null






