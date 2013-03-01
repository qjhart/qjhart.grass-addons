#! /usr/bin/make -f

ifndef configure.mk
include configure.mk
endif

long_term_avg.mk:=1

# Define some .PHONY raster interpolation targets
$(foreach p,et0_ltx et0_lts,$(eval $(call grass_raster_shorthand,$(p))))

$(rast)/et0_s1:
	maps=( `cg.proximate.mapsets --quote --past=7 --future=7 --delim=' ' rast=et0` ); \
	maps=$$(printf '+%s' "$${maps[@]}"); \
	maps=$${maps:1}; \
	r.mapcalc "et0_s1=($$maps)" &> /dev/null

$(rast)/et0_s2:
	maps=( `cg.proximate.mapsets --quote --past=7 --future=7 --delim=' ' rast=et0` ); \
	maps=$$(printf '+%s^2' "$${maps[@]}"); \
	maps=$${maps:1}; \
	cnt=$${#maps[@]}; \
	r.mapcalc "et0_s2=($$maps)"

$(rast)/et0_ltx: $(rast)/et0_s1
	maps=( `cg.proximate.mapsets --quote --past=7 --future=7 --delim=' ' rast=et0` ); \
	cnt=$${#maps[@]}; \
	r.mapcalc "et0_ltx=et0_s1/$$cnt" &> /dev/null

$(rast)/et0_lts: $(rast)/et0_s2 $(rast)/et0_s1
	maps=( `cg.proximate.mapsets --quote --past=7 --future=7 --delim=' ' rast=et0` ); \
	cnt=$${#maps[@]}; \
	r.mapcalc "et0_lts=sqrt(($$cnt*et0_s2+et0_s1^2)/$$cnt*($$cnt-1))" &> /dev/null



