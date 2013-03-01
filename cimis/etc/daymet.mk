#! /usr/bin/make -f

ifndef configure.mk
include configure.mk
endif

interpolate.mk:=1

# Some overview Links
use_dme:=1;	# Comment out to not use Daymet

# Lapse Rates/Tensions/Smooth for dewp and min_at
T_tension:=10
T_smooth:=0.03
d_dewp_lr:=5
d_min_at_lr:=5
d_max_at_lr:=5

# Generic vol.rst parmeters
WIND3:=$(loc)/$(MAPSET)/WIND3
DEFAULT_WIND3:=$(loc)/PERMANENT/WIND3
tension:=5
zmult:=30
smooth:=0.05
tzs:=t${tension}_z$(zmult)_s$(smooth)

.PHONY: info
info:
	@echo interpolate.mk
	@echo use_dme=$(use_dme)


# Define some .PHONY raster interpolation targets
$(foreach p,U2 Tn Tx Tdew RHx ea Rnl FAO_Rso es,$(eval $(call grass_shorthand_names,$(p))))

###############################################################
# Generic/Simple s.vol.rst interpolations
# These are used to calculate 
# d_max_rh and d_avg_ws
# These show the input parameters in the name so that we can keep
# a few around.  Mostly, they're used in combination of other rasters anyways
###############################################################
$(WIND3): $(DEFAULT_WIND3)
	cp $< $@

define spline_template
$(rast)/$(1)_t$(2)_z$(3)_s$(4): $(site_lists)/$(1) $(WIND3)
	s.vol.rst input=$(1) cellinp=Z@2km \
	  field=1 maskmap=state@2km \
	  tension=$(2) zmult=$(3) smooth=$(4) \
	  cellout=$(1)_t$(2)_z$(3)_s$(4) > /dev/null &>/dev/null;
endef

$(foreach p,d_max_rh d_avg_ws,$(eval $(call spline_template,$(p),$(tension),$(zmult),$(smooth))))

###############################################################
# U2 uses only the average wind speed spline fit.
##############################################################
$(rast)/U2: $(rast)/d_avg_ws_$(tzs)
	g.copy -o rast=$(notdir $<),$(notdir $@) &> /dev/null
	$(call colorize,$(notdir $@))
	@r.colors map=$(notdir $@) rast=U2@default_colors &>/dev/null

# _dme rules for T and rh
define daymet
.PHONY: $(1)_dme

$(1)_dme: $(rast)/$(1)_dme

$(rast)/$(1)_dme: $(site_lists)/$(1)
	s.daymet input=$(1) output=$(1)_dme &>/dev/null
endef
$(foreach p,d_min_at d_max_at d_max_rh d_dewp,$(eval $(call daymet,$(p))))

# Currently all Temperature estimations (Tn,Tx,Tdew)
# use an average of the lapse rate (?_ns) and daymet (?_dme) interpolations.
# The current implementation uses a different _dme suffix
ifdef use_dme

define avg_T

.PHONY: $(1) $(1)_err
$(1): $(rast)/$(1)
$(1)_err: $(rast)/$(1)_err

$(rast)/$(1): $(rast)/d_$(2)_dme $(rast)/d_$(2)_ns
	r.mapcalc ' $(1)=(d_$(2)_dme+d_$(2)_ns)/2 ' &> /dev/null
	@r.colors map=$(1) rast=at@default_colors >/dev/null

$(rast)/$(1)_err: $(rast)/$(1) $(rast)/d_$(2)_ns
	r.mapcalc '$(1)_err=sqrt(2)*abs($(1)-d_$(2)_ns)' &> /dev/null
endef

$(rast)/RHx: $(rast)/d_max_rh_dme $(rast)/d_max_rh_$(tzs)
	r.mapcalc 'RHx=(d_max_rh_dme+d_max_rh_$(tzs))/2' &> /dev/null
	@r.colors map=RHx rast=rh@default_colors > /dev/null

$(rast)/RHx_err: $(rast)/RHx $(rast)/d_max_rh_dme
	r.mapcalc 'RHx_err=sqrt(2)*abs(RHx-d_max_rh_dme)' &> /dev/null

else
define avg_T

$(rast)/$(1): $(rast)/d_$(2)_ns
	r.mapcalc '$(1)=d_$(2)_ns' &> /dev/null
	@r.colors map=$(1) rast=at@default_colors >/dev/null
endef

$(rast)/RHx: $(rast)/d_max_rh_$(tzs)
	r.mapcalc 'RHx=d_max_rh_$(tzs)' &> /dev/null
	@r.colors map=RHx rast=rh@default_colors > /dev/null

endif
$(eval $(call avg_T,Tn,min_at))
$(eval $(call avg_T,Tx,max_at))
$(eval $(call avg_T,Tdew,dewp))

$(rast)/Tm: $(rast)/Tx $(rast)/Tn
	r.mapcalc 'Tm=(Tx+Tn)/2.0' &>/dev/null;

###########################################################################
# es is calculated from min/max at
# ea is calculated two ways,
# - from extrapolated dewpt
# - from Tn * extrapolated RHx.
# Depending on settings, we either use the dewpt or no method.
# Which is set with use_rh_for_ea
###########################################################################
$(rast)/es: $(rast)/Tx $(rast)/Tn
	r.mapcalc '$(notdir $@)=0.6108 / 2 * (exp(Tn * 17.27/ (Tn + 237.3))+ exp(Tx * 17.27/ (Tx + 237.3)))' &> /dev/null;

#use_rh_for_ea:=0  # Comment out to use only dewpt as estimator for ea

ifdef use_rh_for_ea

$(rast)/ea_rh: $(rast)/RHx $(rast)/Tn
	r.mapcalc 'ea_rh=0.6108*(exp(Tn * 17.27/ (Tn + 237.3))*RHx/100)' &> /dev/null; 

$(rast)/ea_Tdew: $(rast)/Tdew
	r.mapcalc '$(notdir $@)=0.6108*exp($(notdir $<)*17.27/(($(notdir $<)+237.3)))' &> /dev/null;

$(rast)/ea: $(rast)/ea_Tdew $(rast)/ea_rh
	r.mapcalc 'ea=(ea_Tdew+ea_rh)/2' &> /dev/null

$(rast)/ea_err: $(rast)/ea
	r.mapcalc 'ea_err=sqrt(2)*abs(ea-ea_Tdew)' &> /dev/null

else
# These methods only use dew point Temperature in Calculations
$(rast)/ea_dewp_dme: $(rast)/d_dewp_dme
	r.mapcalc '$(notdir $@)=0.6108*exp(($(notdir $<)*17.27/(($(notdir $<)+237.30))))' &> /dev/null; 

$(rast)/ea_dewp_ns: $(rast)/d_dewp_ns
	r.mapcalc '$(notdir $@)=0.6108*exp(($(notdir $<)*17.27/(($(notdir $<)+237.30))))' &> /dev/null;

$(rast)/ea: $(rast)/ea_dewp_dme $(rast)/ea_dewp_ns
	r.mapcalc 'ea=(ea_dewp_dme+ea_dewp_ns)/2' &> /dev/null

$(rast)/ea_err: $(rast)/ea
	r.mapcalc '$(notdir $@)=sqrt(2)*abs(ea-ea_dewp_ns) ' &> /dev/null

endif

#####################################################################
# These rules build termperture lapse-rate normalized, rasters
# Using 2-d splines to fit the normalized data
# 
# The output is a raster $(name)_ns to indicate normalized splines
# intermediate files include: normalized site_list,normalized_spline
#####################################################################
define normalized_T
.PHONY: $(1)_ns

n$(1): $(site_lists)/n$(1)_lr$(2)

$(1)_ns: $(rast)/$(1)_ns

# Normalized Site_Lists
$(site_lists)/n$(1)_lr$(2): $(site_lists)/$(1)
	perl -p -F'\|' -a -e '$$$$lapse=$(2);' \
	  -e '/\%(\-?[\d.]+)/ and $$$$t=sprintf("%.2f",$$$$1+$$$$lapse*$$$$F[2]/1000);' \
	  -e 's/\%\-?[\d.]+/\%$$$$t/;' $$<  > $$@

# Normalized Spline for Lapse Rate Calculation
$(rast)/n$(1)_lr$(2)_t$(3)_s$(4): $(site_lists)/n$(1)_lr$(2)
	$(call MASK)
	s.surf.rst input=n$(1)_lr$(2) \
	  field=1 maskmap=MASK \
	  tension=$(3) smooth=$(4) \
	  elev=n$(1)_lr$(2)_t$(3)_s$(4) > /dev/null &>/dev/null;

# ReUnNormalized back to Elevation
$(rast)/$(1)_ns: $(rast)/n$(1)_lr$(2)_t$(3)_s$(4)
	r.mapcalc $(1)_ns=n$(1)_lr$(2)_t$(3)_s$(4)-$(2)*Z@2km/1000 &> /dev/null

endef

$(foreach p,d_dewp,$(eval $(call normalized_T,$(p),$(d_dewp_lr),$(T_tension),$(T_smooth))))
$(foreach p,d_min_at,$(eval $(call normalized_T,$(p),$(d_min_at_lr),$(T_tension),$(T_smooth))))
$(foreach p,d_max_at,$(eval $(call normalized_T,$(p),$(d_max_at_lr),$(T_tension),$(T_smooth))))





