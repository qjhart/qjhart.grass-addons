#! /usr/bin/make -f 

ifndef configure.mk
include configure.mk
endif

zipcode.mk:=1

#parms:=Rs et0 K Rnl Tx Tn U2 Rso
parms:=Rs et0

.PHONY: info
info::
	@echo zipcode.mk

ifneq (${LOCATION_NAME},WORLD)

ifneq (${MAPSET},PERMANENT)

.PHONY:zipcode
zipcode:${vect}/zipcode

${vect}/zipcode: ${vect}/zcta510_2012
	g.copy vect=zcta510_2012,zipcode;

${vect}/zcta510_2012:$(patsubst %,${rast}/%,${parms})
	g.copy vect=zcta510_2012@PERMANENT,zcta510_2012;\
	for v in ${parms}; do \
	  g.region rast=$$v res=500; \
	  r.mapcalc $${v}_500=$$v; \
	  v.rast.stats zcta510_2012 raster=$${v}_500 colprefix=$$v 2>/dev/null; \
	  g.remove $${v}_500; \
	done;

#${vect}/zcta510_2012:$(patsubst %,${rast}/%,${parms})
#	g.copy vect=zcta510_2012@PERMANENT,zcta510_2012;\
#	for v in ${parms}; do \
#	  v.rast.stats zcta510_2012 raster=$${v} colprefix=$$v 2>/dev/null; \
#	done;

else

.PHONY:zcta510_2012
zcta510_2012:${vect}/zcta510_2012

${vect}/zcta510_2012:${GISDBASE}/WORLD/zipcode/vector/tl_2012_zcta510_CA
	v.proj input=$(notdir $<) location=WORLD mapset=zipcode output=$(notdir $@); \
	db.dropcol -f table=zcta510_2012 column=INTPTLAT10; \
	db.dropcol -f table=zcta510_2012 column=INTPTLON10; \
	db.dropcol -f table=zcta510_2012 column=ALAND10; \
	db.dropcol -f table=zcta510_2012 column=AWATER10; \
	db.dropcol -f table=zcta510_2012 column=FUNCSTAT10; \
	db.dropcol -f table=zcta510_2012 column=MTFCC10; \
	db.dropcol -f table=zcta510_2012 column=CLASSFP10; \
	db.dropcol -f table=zcta510_2012 column=GEOID10; \
	v.db.renamecol map=zcta510_2012  column=ZCTA5CE10,zipcode
endif 

else 

ifneq (${MAPSET},zipcode)
  $(error LOCATION ${LOCATION_NAME} and MAPSET ${MAPSET} neq zipcode)
endif

.PHONY:state
state:${vect}/tl_2012_us_state ${vect}/STUSPS_CA

${etc}/tl_2012_us_state.zip:url:=http://www2.census.gov/geo/tiger/TIGER2012/STATE/tl_2012_us_state.zip
${etc}/tl_2012_us_state.zip:
	[[ -d ${etc} ]] || mkdir ${etc};\
	wget -O $@ ${url}; \
	cd $(dir $@); \
	unzip $(notdir $@);

${vect}/tl_2012_us_state:${etc}/tl_2012_us_state.zip
	v.in.ogr dsn=${etc} layer=$(notdir $@) output=$(notdir $@) type=boundary

${vect}/STUSPS_CA:${etc}/tl_2012_us_state.zip
	v.in.ogr dsn=${etc} layer=tl_2012_us_state output=$(notdir $@) type=boundary where="STUSPS='CA'"

${etc}/tl_2012_us_zcta510.zip:url:=http://www2.census.gov/geo/tiger/TIGER2012/ZCTA5/tl_2012_us_zcta510.zip
${etc}/tl_2012_us_zcta510.zip:
	[[ -d ${etc} ]] || mkdir ${etc}
	wget -O $@ ${url} 
	cd $(dir $@)
	unzip $(notdir $@)

.PHONY:.zip
zip:${vect}/tl_2012_zcta510_CA
${vect}/tl_2012_zcta510_CA:${etc}/tl_2012_us_zcta510.zip ${vect}/STUSPS_CA
	eval $$(v.info -g STUSPS_CA); \
	v.in.ogr dsn=${etc} spatial=$$west,$$south,$$east,$$north layer=tl_2012_us_zcta510 output=$(notdir $@)_box type=boundary;
	v.select ainput=$(notdir $@)_box binput=STUSPS_CA output=$(notdir $@) operator=overlap
	g.remove vect=$(notdir $@)_box

endif

