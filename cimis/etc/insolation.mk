#! /usr/bin/make -f

ifndef configure.mk
include configure.mk
endif

insolation.mk:=1

# Check this is a YYYY-MM-DD setup
$(call hasDD)


# Define some .PHONY raster interpolation targets
$(foreach p,FAO_Rso Rso Rs K Dk Bk sretr ssetr ssha,$(eval $(call grass_raster_shorthand,$(p))))

# Sunrise/Sunset parameters are taken from r.solpos
${rast}/sretr ${rast}/ssetr ${rast}/ssha:
	r.solpos date=$(MAPSET) sretr=sretr ssetr=ssetr ssha=ssha

############################################################################
# Calculations of the CIMIS data, are still handled outside this file
# I will think about inserting them into this setup.
# This includes K,Dk,Bk,Trb,Trd
############################################################################
insolation: ${rast}/Bc ${rast}/Dc ${rast}/Gc ${rast}/Trd ${rast}/Trd ${rast}/FAO_Rso ${rast}/Rs ${rast}/Rso

clean:: clean-tmp
	g.remove rast=Rs,B,D,Bc,Dc,Bk,Dk,Rso,G,K,Bc,Dc,Gc,Trb,Trd,FAO_Rso &> /dev/null
	g.mremove -f rast=[knp]????


# Wh/m^2 day -> MJ/m^2 day
${rast}/Rso: ${rast}/Gc
	r.mapcalc "Rso=(Gc*0.0036)" &> /dev/null; 
	@r.colors map=Rso rast=Rso@default_colors > /dev/null

${rast}/Rs: ${rast}/G
	r.mapcalc "Rs=(G*0.0036)" &> /dev/null; 
	@r.colors map=Rs rast=Rso@default_colors > /dev/null


########################################################################
# This is the CIMIS method of calculating the Extraterrestrial Radiation
#############################################################################
.PHONY: FAO_Rso
FAO_Rso: ${rast}/FAO_Rso

${rast}/FAO_Rso: ${rast}/ssha
	$(call NOMASK)
	eval `r.solpos -r date=$(MAPSET)`; \
	r.mapcalc "FAO_Rso=(0.0036)*(0.75+0.00002*'Z@2km')*$$etrn*24/3.14159*\
	((ssha*3.14159/180)*sin(latitude_deg@2km)*sin($$declin)\
	+cos(latitude_deg@2km)*cos($$declin)*sin(ssha))" &> /dev/null
	@r.colors map=$(notdir $<) rast=Rso@default_colors > /dev/null


visHHMM:=$(shell cg.daylight.intervals --exists --int=60 delim=' ' filename='vis%hh%mm' date=$(MAPSET) sretr=sretr ssetr=ssetr)
HHMM:=$(subst vis,,$(visHHMM))
kXXXX:=$(patsubst %,k%,$(HHMM))

##############################################################################
# Calculation of Albedo.  We need:
# k=1 cloud value (max brightness on 9x9)
# albedo for each hour
##############################################################################

#cloud-cover::

define r.cloud_cover

#cloud-cover:: $(rast)/cloud$(1)
#$(eval $(call grass_shorthand_names,n$(1)))
#$(eval $(call grass_shorthand_names,p$(1)))
#$(eval $(call grass_shorthand_names,k$(1)))
#$(eval $(call grass_shorthand_names,cloud$(1)))

$(rast)/vis$(1)_9:$(rast)/vis$(1)
	$(call NOMASK)
	r.neighbors --q size=9 input=vis${1} output=vis${1}_9 method=average

$(rast)/vis$(1)_m:$(rast)/vis$(1)
	$(call NOMASK)
	r.neighbors --q size=5 input=vis${1} output=vis${1}_m method=median

# We should look at replacing this with a simpler method (like top 2%)
$(etc)/maxmedian/vis$(1): $(rast)/vis$(1)_m
	[[ -d $(etc)/maxmedian ]] || mkdir -p $(etc)/maxmedian
	max=$$$$(for p in `cg.proximate.mapsets --past=14 rast=vis$(1)_m --delim=' '`; do \
	  r.info -r $$$$p | sed -n -e 's/max=// p'; \
	done | sort -n | tail -1); \
	echo $$$$max > $$@

# We should look at replacing this with a simpler method (like top 2%)
$(etc)/max/vis$(1): $(rast)/vis$(1)_9
	[[ -d $(etc)/max ]] || mkdir -p $(etc)/max
	max=$$$$(for p in `cg.proximate.mapsets --past=14 rast=vis$(1)_9 --delim=' '`; do \
	  r.info -r $$$$p | sed -n -e 's/max=// p'; \
	done | sort -n | tail -1); \
	echo $$$$max > $$@

$(rast)/p$(1): $(rast)/vis$(1)
	maps=`cg.proximate.mapsets --quote --delim=',' --past=14 rast=vis$(1)`; \
	r.mapcalc "p$(1)=min($$$$maps)" &> /dev/null

$(rast)/cloud$(1): $(rast)/p$(1) $(etc)/max/vis$(1)
	$(call NOMASK)
	max=`cat $(etc)/max/vis$(1)`; \
	r.mapcalc "cloud$(1)=int(if(isnull(p$(1)),if(vis$(1)>$$$$max,1,vis$(1)/$$$$max),if(vis$(1)>$$$$max,1,(vis$(1)-p$(1))/($$$$max-p$(1))))*255)" &> /dev/null

$(rast)/n$(1): $(rast)/p$(1) $(etc)/max/vis$(1)
	$(call NOMASK) 
	max=`cat $(etc)/max/vis$(1)`; \
	r.mapcalc "n$(1)=if($$$$max>p$(1),($$$$max-vis$(1))/($$$$max-p$(1)),1.2)" &> /dev/null

$(rast)/k$(1): $(rast)/n$(1)
	$(call NOMASK)
	r.mapcalc 'k$(1)=if((n$(1)>1.2),1.2,if((n$(1)>0.2),n$(1),(if((n$(1)>-0.1),(5.0/3.0)*n$(1)*n$(1)+(1.0/3.0)*n$(1)+(1.0/15.0),0.05))))' &> /dev/null

endef
$(foreach h,$(HHMM),$(eval $(call r.cloud_cover,$(h))))


########################################################################
# Heliosat method for Radiation
########################################################################
# Linke Turbidity
tl_DD:=01 01 01 01 07 07 07 07 07 07 \
    07 15 15 15 15 15 15 15 21 21 \
    21 21 21 21 28 28 28 28 28 28 28

tl_mapset:=${MM}-$(word ${DD},${tl_DD})@linke

# Instead we calculate the tl directly
#tl_mapset:=$(shell ymd=`date --date=${MAPSET} --rfc-3339=date`; day=`date --date=$$ymd +%d`; while [[ $$day != "07" ]] && [[ $$day != "15" ]] && [[ $$day != "21" ]] && [[$$day != "28" ]]; do ymd=`date --date="$$ymd - 1 day" --rfc-3339=date`; day=`date --date=$$ymd +%d`; done ; date --date=$$ymd +%m-%d@linke)

${rast}/Bk ${rast}/Dk ${rast}/K ${rast}/G: ${rast}/ssha $(patsubst %,${rast}/%,$(kXXXX)) ${rast}/Bc ${rast}/Dc ${rast}/Gc
	cg.daily.k ssha=ssha k="$(kXXXX)" tl="${tl_mapset}" Bc=Bc Dc=Dc Gc=Gc > /dev/null 2>/dev/null
	@r.colors map=K rast=K@default_colors > /dev/null

${rast}/Bc ${rast}/Dc ${rast}/Gc ${rast}/Trb ${rast}/Trd: ${rast}/ssha
	@g.remove rast=Bc,Dc,Gc,Trb,Trd > /dev/null
	r.heliosat --prefix='_' --noinstant --date=$(MAPSET) elevin=Z@2km linkein="${tl_mapset}" lat=latitude_deg@2km ssha=ssha Bc=Bc Dc=Dc Gc=Gc Trb=Trb Trd=Trd >/dev/null 2>/dev/null
	@r.colors map=Gc rast=G@default_colors >/dev/null
	@r.colors map=Bc rast=G@default_colors >/dev/null
	@r.colors map=Dc rast=G@default_colors >/dev/null

clean::
	@g.remove rast=Rnl;

${rast}/Rnl: ${rast}/Tx ${rast}/Tn ${rast}/ea ${rast}/K
	r.mapcalc 'Rnl=-(1.35*K-0.35)*(0.34-0.14*sqrt(ea))*4.9e-9*(((Tx+273.16)^4+(Tn+273.16)^4)/2)' &> /dev/null

# Shortwave Radiation has been measured to have about 15% error.  Though this
# is just an estimate.  Less likely is the 15% estimate on Long wave radiation
ifndef error.mk
include error.mk
endif

$(foreach p,Rs Rnl,$(eval $(call r.percent.err,$(p),0.15)))

.PHONY:clean-tmp
clean-tmp::
	g.mremove -f rast=_*
	g.mremove -f rast=Bk????
	g.mremove -f rast=Dk????
