#! /usr/bin/make -f 

ifndef configure.mk
include configure.mk
endif

ifndef interpolate.mk
include interpolate.mk
endif

ifndef insolation.mk
include insolation.mk
endif

ifndef zipcode.mk
include zipcode.mk
endif 

ifndef png.mk
include png.mk
endif

cimis.mk:=1

$(foreach p,et0,$(eval $(call grass_raster_shorthand,$(p))))

#######################################################################
# Finally make the et0 calculation
#######################################################################
clean::
	g.remove rast=et0

$(rast)/et0: $(rast)/Rs $(rast)/Rnl $(rast)/ea $(rast)/Tx $(rast)/Tn $(rast)/U2 $(rast)/Tm $(rast)/es
	DEL="(4098.17*0.6108*(exp(Tm*17.27/(Tm+237.3)))/(Tm+237.3)^2)"; \
	GAM="psychrometric_constant@2km"; \
	r.mapcalc et0="(900.0*$$GAM/(Tm+273)*U2*(es-ea)+0.408*$$DEL*(Rs*(1.0-0.23)+Rnl))/($$DEL+$$GAM*(1.0+0.34*U2))" &>/dev/null;
	@r.colors map=$(notdir $@) rast=$(notdir $@)@default_colors > /dev/null

.PHONY:clean-rast

clean-rast::
	@echo "Removing ALMOST all rasters"
	@tmp=`g.mlist type=rast pattern=* | grep -v '^vis....$$' | grep -v '^vis...._[0-9]$$' | tr "\n" ','`; \
	if [[ $$tmp != ',' && $$tmp != '' ]]; then \
	echo "g.remove rast=$$tmp";\
	g.remove rast=$$tmp > /dev/null;\
	else \
	echo None in MAPSET; \
	fi;
