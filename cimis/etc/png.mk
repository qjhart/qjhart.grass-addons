#! /usr/bin/make -f 

ifndef configure.mk
include configure.mk
endif

png.mk:=1

# New one will be this
daily_dir:=$(YYYY)/$(MM)/$(DD)
htdocs:=/var/www/cimis
html:=$(htdocs)/$(daily_dir)

.PHONY: info
info::
	@echo png.mk
	@echo html files to ${html}

# PNG Writing Functions
define d_map 
d.frame -e; \
d.rast $1; \
d.vect counties@PERMANENT color=white; \
d.font font=romand; \
d.frame -c frame=legend at=7,52,2,17; \
d.erase color=white; \
d.legend -s map=$1 color=black; \
if [[ -n $3 ]]; then \
 d.frame -c frame=units at=2,7,2,17; \
 d.erase color=white; \
 echo $3 | d.text color=black size=60; \
fi; \
d.frame -c frame=title at=90,95,55,95; \
d.erase color=white; \
echo -e ".B 1\n$2" | d.text color=black size=60;
endef

define d_png
d.mon -r; \
sleep 1; \
GRASS_WIDTH=500 \
GRASS_HEIGHT=550 \
GRASS_TRUECOLOR=TRUE \
GRASS_BACKGROUND_COLOR=FFFFFF \
GRASS_PNGFILE=$1 \
d.mon start=PNG &> /dev/null; \
$(call d_map, $2, $3, $4) \
d.mon stop=PNG &> /dev/null;
endef

define test
echo Hello
endef
# Don't inlcude RHx anymore
#html_layers:= Rso Rs K Rnl Tdew et0 Tx Tn U2 mc_et0_avg mc_et0_err_3
html_layers:= Rso Rs K Rnl Tdew et0 Tx Tn U2

html: $(patsubst %,$(html)/%.png,$(html_layers)) $(patsubst %,$(html)/%.asc.gz,$(html_layers))
#html:=${html} $(html)/station_compare.csv $(html)/station_compare_7.csv

clean-html::
	rm -rf $(html)

define PNG
.PHONY: $(1).png

$(1).png: $(html)/$(1).png $(html)/$(1).asc.gz
$(1).asc.gz: $(html)/$(1).asc.gz

$(html)/$(1).asc.gz: $(rast)/$(1)
	@echo $(1).asc.gz
	@[[ -d $(html) ]] || mkdir -p $(html)
	@r.out.arc input=$(1) output=$(html)/$(1).asc &>/dev/null;
	@gzip -f $(html)/$(1).asc;

#	$$(call d_png $(html)/$(1).png,$(1),$(2),$(3),$(4));
#	after d.rast...
#	d.vect counties@PERMANENT color=white fcolor=none;
$(html)/$(1).png: $(rast)/$(1)
	@echo $(1).png
	@[[ -d $(html) ]] || mkdir -p $(html);
	@d.mon -r; sleep 1; \
	$(call MASK) \
	GRASS_WIDTH=500 GRASS_HEIGHT=550 \
	GRASS_TRUECOLOR=TRUE GRASS_BACKGROUND_COLOR=FFFFFF \
	GRASS_PNGFILE=${html}/$1.png d.mon start=PNG &> /dev/null; \
	d.frame -e; d.rast $1; \
	d.legend -s at=7,52,2,8 map=$1 color=black; \
	if [[ -n "$3" ]]; then \
	 echo '$3' | d.text color=black at=2,2 size=3; \
	fi; \
	echo -e ".B 1\n$2" | d.text at=45,90 color=black size=4; \
	d.mon stop=PNG &> /dev/null;\
	$(call NOMASK)

$(html)/$(1).png.old: $(rast)/$(1)
	@echo $(1).png
	@[[ -d $(html) ]] || mkdir -p $(html);
	@d.mon -r; sleep 1; \
	$(call MASK) \
	GRASS_WIDTH=500 GRASS_HEIGHT=550 \
	GRASS_TRUECOLOR=TRUE GRASS_BACKGROUND_COLOR=FFFFFF \
	GRASS_PNGFILE=${html}/$1.png d.mon start=PNG &> /dev/null; \
	d.frame -e; d.rast $1; \
	d.font font=romand; \
	d.frame -c frame=legend at=7,52,2,17; \
	d.erase color=white; \
	d.legend -s map=$1 color=black; \
	if [[ -n "$3" ]]; then \
	 d.frame -c frame=units at=2,7,2,17; \
	 d.erase color=white; \
	 echo '$3' | d.text color=black size=60; \
	fi; \
	d.frame -c frame=title at=90,95,55,95; \
	d.erase color=white; \
	echo -e ".B 1\n$2" | d.text color=black size=60;
	d.mon stop=PNG &> /dev/null;\
	$(call NOMASK)

endef

# Special for report
$(eval $(call PNG,nd_max_at_lr5_t10_s0.03,,C))
$(eval $(call PNG,d_max_at_dme,,C))
$(eval $(call PNG,d_max_at_ns,,C))
$(eval $(call PNG,d_max_rh_dme,,%))
$(eval $(call PNG,d_max_rh_$(tzs),,%))
$(eval $(call PNG,FAO_Rso,CIMIS Radiation,W/m^2))
$(foreach p,00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24,$(eval $(call PNG,vis$(p)00,Visible GOES-$(p)00,count)))
$(foreach p,00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24,$(eval $(call PNG,p$(p)00,Albedo GOES-$(p)00,count)))
$(foreach p,00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24,$(eval $(call PNG,k$(p)00,Clear Sky-$(p)00,count)))

$(eval $(call PNG,Tn,Tn, C))
$(eval $(call PNG,Tx,Tx, C))
$(eval $(call PNG,Tdew,Tdew, C))
$(eval $(call PNG,RHx,RHx, C))
$(eval $(call PNG,U2,Wind Speed, m/s))

$(eval $(call PNG,Rs,Rs View,MJ/m^2 day))
$(eval $(call PNG,Rso,Clear Sky Radiation,MJ/m^2 day))
$(eval $(call PNG,K,Clear Sky Parameter, ))
$(eval $(call PNG,et0,ET0 View, mm))
$(eval $(call PNG,Rnl,Long wave Radiation, MJ/m^2))

$(eval $(call PNG,mc_et0_avg,ET0 Confidence Avg.,mm))
$(eval $(call PNG,mc_et0_err_3,ET0 Confidence Std.,mm))



