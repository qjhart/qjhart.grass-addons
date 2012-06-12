#! /usr/bin/make -f

ifndef configure.mk
include configure.mk
endif

ifndef spline.mk
include spline.mk
endif

#ifndef daymet.mk
#include daymet.mk
#endif

interpolate.mk:=1

.PHONY: info
info::
	@echo interpolate.mk
	@echo use_dme=$(use_dme)

###############################################################
# All interpolations depend on the etxml vector
##############################################################
${vect}/etxml:
	v.in.etxml date=${MAPSET}

# Define some .PHONY raster interpolation targets
$(foreach p,U2 Tn Tx Tdew RHx ea es,$(eval $(call grass_raster_shorthand,$(p))))

###############################################################
# U2 uses only the average wind speed spline fit.
##############################################################
$(rast)/U2: ${rast}/day_wind_spd_avg_${tzs}
	g.copy --o rast=$(notdir $<),$(notdir $@) &> /dev/null
	r.support map=U2 title='U2' units='m/s' \
	description='Daily average windspeed at 2m height' \
	source1='3D spline from CIMIS data'
	map=$(notdir $@);
	$(call colorize,$(notdir $@))

# Currently all Temperature estimations (Tn,Tx,Tdew)
# use an average of the lapse rate (?_ns) and daymet (?_dme) interpolations.
# The current implementation uses a different _dme suffix
ifdef use_dme

define avg_T

.PHONY: $(1) $(1)_err
$(1): $(rast)/$(1)
$(1)_err: $(rast)/$(1)_err

$(rast)/$(1): $(rast)/d_$(2)_dme $(rast)/$(2)_ns
	r.mapcalc ' $(1)=(d_$(2)_dme+$(2)_ns)/2 ' &> /dev/null
	@r.colors map=$(1) rast=at@default_colors >/dev/null

$(rast)/$(1)_err: $(rast)/$(1) $(rast)/d_$(2)_ns
	r.mapcalc '$(1)_err=sqrt(2)*abs($(1)-d_$(2)_ns)' &> /dev/null
endef

$(rast)/RHx: $(rast)/day_rel_hum_max_dme $(rast)/day_rel_hum_max_$(tzs)
	r.mapcalc 'RHx=(day_rel_hum_max_dme+day_rel_hum_max_$(tzs))/2' &> /dev/null
	@r.colors map=RHx rast=rh@default_colors > /dev/null

$(rast)/RHx_err: $(rast)/RHx $(rast)/day_rel_hum_max_dme
	r.mapcalc 'RHx_err=sqrt(2)*abs(RHx-day_rel_hum_max_dme)' &> /dev/null

else
define avg_T

$(rast)/$(1): $(rast)/$(2)_ns
	r.mapcalc '$(1)=$(2)_ns' &> /dev/null
	@r.colors map=$(1) rast=at@default_colors >/dev/null
endef

$(rast)/RHx: $(rast)/day_rel_hum_max_$(tzs)
	r.mapcalc 'RHx=day_rel_hum_max_$(tzs)' &> /dev/null
	@r.colors map=RHx rast=rh@default_colors > /dev/null

endif
$(eval $(call avg_T,Tn,day_air_tmp_min))
$(eval $(call avg_T,Tx,day_air_tmp_max))
$(eval $(call avg_T,Tdew,day_dew_pnt))

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

$(rast)/ea_dewp_ns: $(rast)/day_dew_pnt_ns
	r.mapcalc '$(notdir $@)=0.6108*exp(($(notdir $<)*17.27/(($(notdir $<)+237.30))))' &> /dev/null;

ifdef use_dme

# These methods only use dew point Temperature in Calculations
$(rast)/ea_dewp_dme: $(rast)/day_dew_pnt_dme
	r.mapcalc '$(notdir $@)=0.6108*exp(($(notdir $<)*17.27/(($(notdir $<)+237.30))))' &> /dev/null; 

$(rast)/ea: $(rast)/ea_dewp_dme $(rast)/ea_dewp_ns
	r.mapcalc 'ea=(ea_dewp_dme+ea_dewp_ns)/2' &> /dev/null

$(rast)/ea_err: $(rast)/ea
	r.mapcalc '$(notdir $@)=sqrt(2)*abs(ea-ea_dewp_ns) ' &> /dev/null

else 

$(rast)/ea: $(rast)/ea_dewp_ns
	r.mapcalc 'ea=ea_dewp_ns' &> /dev/null

#$(rast)/ea_err: $(rast)/ea
#	r.mapcalc '$(notdir $@)=sqrt(2)*abs(ea-ea_dewp_ns) ' &> /dev/null

endif # use_dme

endif # use_rh

