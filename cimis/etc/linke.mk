#! /usr/bin/make -f

# Can't configure as we are in heliosat? MAPSET, and test will fail
ifndef configure.mk
include configure.mk
endif

linke:=1

months:=January February March April May June July \
       August September October November December 

ifneq (${MAPSET},linke)
  $(error MAPSET ${MAPSET} neq linke)
endif

# Originally from:
#http://www.helioclim.org/linke/linke_helioserve.html#maps
# Now 
# http://www.soda-is.com/eng/services/linke_turbidity_maps_info.html
# files currently available at (as tifs)

ifeq (${LOCATION_NAME},WORLD)

zips:=$(patsubst %,${etc}/%_trimester.zip,first second third fourth)

zips: ${zips}

${zips}:${etc}/%:
	[[ -d ${etc} ]] || mkdir ${etc}
	wget -O $@ http://www.soda-is.com/doc/linke_turbidity/$*; \
	cd $(dir $@); \
	unzip $(notdir $@); \
	[[ -f $ february.zip ]] && mv februrary.zip Februrary.zip

linke: $(patsubst %,${rast}/%,${months})

$(patsubst %,${rast}/%,${months}):${rast}/%:${etc}/%.tif
	r.in.gdal -o -l input=$< output=$* title='Linke Turbidity ($*)'
	r.region map=$* w=-180 s=-90 e=180 n=90
	r.support map=$* units='1/20 of Linke unit'

else # if LOCATION_NAME=WORLD

m:=01 02 03 04 05 06 07 08 09 10 11 12
prev:=12 01 02 03 04 05 06 07 08 09 10 11
next:=02 03 04 05 06 07 08 09 10 11 12 01

define linke
linke:: ${rast}/${1}-01 ${rast}/${1}-07 ${rast}/${1}-15 ${rast}/${1}-21 ${rast}/${1}-28

${rast}/$(word $1,${months}):
	g.region -d;
	r.proj input=$(word $1,${months}) location=WORLD method=cubic

${rast}/${1}-15:${rast}/$(word $1,${months})
	g.region -d;
	r.mapcalc '"${1}-15"=$(word $1,${months})/20';

${rast}/${1}-01:${rast}/${1}-15 ${rast}/$(word $1,${prev})-15
	g.region -d;
	r.mapcalc '"$1-01"="${1}-15"*0.5+"$(word $1,${prev})-15"*0.5'

${rast}/${1}-07:${rast}/${1}-15 ${rast}/$(word $1,${prev})-15
	g.region -d;
	r.mapcalc '"$1-07"="${1}-15"*0.75+"$(word $1,${prev})-15"*0.25'

${rast}/${1}-21:${rast}/${1}-15 ${rast}/$(word $1,${next})-15
	g.region -d;
	r.mapcalc '"$1-21"="${1}-15"*0.75+"$(word $1,${next})-15"*0.25'

${rast}/${1}-28:${rast}/${1}-15 ${rast}/$(word $1,${next})-15
	g.region -d;
	r.mapcalc '"$1-28"="${1}-15"*0.5+"$(word $1,${next})-15"*0.5'

endef

$(foreach i,${m},$(eval $(call linke,${i})))

endif # if LOCATION_NAME=WORLD
