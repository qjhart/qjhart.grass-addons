#! /usr/bin/make -f

ifndef configure.mk
include configure.mk
endif

ca-daily-vis.mk:=1
ca-daily-vis-loc:=$(shell pkg-config --variable=ca_daily_vis_loc cg)
interval:=$(shell pkg-config --variable=ca_daily_vis_interval cg)

ifneq (${LOCATION_NAME},$(notdir ${ca-daily-vis-loc}))
  $(error LOCATION_NAME neq $(notdir ${ca-daily-vis-loc}))
endif

#date:=$(shell date --date='today' +%Y-%m-%d)
date:=$(shell g.gisenv MAPSET)
#nowz:=$(shell date --utc +%Y-%m-%dT%H%M)
now-s:=$(shell date +%s)
hrs:=$(shell cg.daylight.intervals --noexists delim=' ' --filename=%hh%mm --interval=${interval} sretr=sretr ssetr=ssetr)

${ca-daily-vis-loc}/cellhd/sretr:
	g.mapset -c ${date}
	r.solpos date=${date} sretr=sretr ssetr=ssetr ssha=ssha;

hr-rast:=$(patsubst %,${ca-daily-vis-loc}/cellhd/vis%,${hrs})

# Get the UTC version of the file.
# Get current time compares

before:=$(shell for h in ${hrs}; do ms=$$(date --date="$$(date --date="${date} $$h")" +%s); if [[ ${now-s} -gt $$ms ]]; then echo " $$h"; else echo "not $$h"; fi;done )

beforet:=$(shell for h in ${hrs}; do ms=$$(date --date="$$(date --date="${date} $$h")" +%s); if [[ ${now-s} -ge $$ms ]]; then echo " $$h"; else echo "not $$h"; fi;done )



.PHONY: info

info::
	@echo ca-daily-vis
	@echo "Copy Dayight Visible GOES data into CIMIS daily summary"
	@echo "date:${date} (${now-s})"
	@echo "daylight-hours:${hrs}"
	@echo "before:${before}"
	@echo "beforet:${beforet}"

${hr-rast}:${ca-daily-vis-loc}/cellhd/%
	h=$${$*#vis*}; \
m=$$(date --date="$$(date --date="${date} $$h")" --utc +%Y-%m-%dT%H%M);\
ms=$$(date --date="$$(date --date="${date} $$h")" +%s);\
if [[ ${now-s} -ge $$ms ]]; then \
  if [[ -n ${goes-loc}/$$m/cellhd/ch1 ]]; then \
   echo $${v} from $$ld; \
     files=$$(r.proj -l input=ch1 mapset=$$m location=$(notdir ${goes-loc}) 2>/dev/null | grep ch1);\
     if [[ -n $$files ]]; then \
       r.proj input=ch1 mapset=$$m location=$(notdir ${goes-loc}) output=temp 2>/dev/null > /dev/null;\
       r.mapcalc $$v=0.585454025*temp-16.9781625;\
       g.remove temp;\
   else\
     echo $$m or ch1@$$m not found;\
   fi;\
else\
  echo $$m is in the future;\
fi;


