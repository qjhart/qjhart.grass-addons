#! /usr/bin/make -f
SHELL=/bin/bash

# Are we currently Running Grass?
ifndef GISBASE
  $(error Must be running in GRASS)
endif

# Set ETXML Username and password
include etxml_configure.mk
$(shell g.gisenv set=ETXML_USER=${ETXML_USER})
$(shell g.gisenv set=ETXML_PASSWD=${ETXML_PASSWD})

# Some overview Links
# use_dme:=1;	# Comment out to not use Daymet

GISDBASE:=$(shell g.gisenv get=GISDBASE)
LOCATION_NAME:=$(shell g.gisenv get=LOCATION_NAME)
MAPSET:=$(shell g.gisenv get=MAPSET)

# Check on whether the MAPSET is a day, month, or otherwise
YYYY:=$(shell echo $(MAPSET) | perl -n -e '/^(20\d{2})(-([01]\d)(-([0123]\d))?)?$$/ and print $$1;')
MM:=$(shell echo $(MAPSET) | perl -n -e '/^(20\d{2})(-([01]\d)(-([0123]\d))?)?$$/ and print $$3;')
DD:=$(shell echo $(MAPSET) | perl -n -e '/^(20\d{2})(-([01]\d)(-([0123]\d))?)?$$/ and print $$5;')

###################################################
# Check for YYYY / MM / DD
##################################################
define hasYYYY
ifndef YYYY
$(error MAPSET  is not a YYYY*)
endif
endef

define hasMM
ifndef MM
$(error MAPSET is not YYYY-MM*)
endif
endef

define hasMM
ifndef DD
$(error MAPSET is not YYYY-MM-DD)
endif
endef

# Shortcut Directories
loc:=$(GISDBASE)/$(LOCATION_NAME)
rast:=$(loc)/$(MAPSET)/cellhd
vect:=$(loc)/$(MAPSET)/vector
site_lists:=$(loc)/$(MAPSET)/site_lists
# etc is our special location for non-grass datafiles
etc:=$(loc)/$(MAPSET)/etc

SQLITE:=sqlite3
db.connect.database:=${loc}/${MAPSET}/sqlite.db

# Common Rasters
state:=state@2km
Z:=Z@2km

.PHONY: info
info::
	@echo GISRC: $(GISRC)
	@g.gisenv;
	@echo YYYY/MM/DD: $(YYYY)/$(MM)/$(DD)
	@echo Times =$(HHMM)
	@echo use_dme=$(use_dme)


#########################################################################
# Add default colors to a layer
#
# $(rast)/foo:
#	..something to make foo..
#	$(call colorize,$(notdir $@))
#########################################################################
define colorize
	@r.colors map=${1} rast=${1}@default_colors &>/dev/null
endef

#########################################################################
# Allow Shorthand notations, so we can call rasters by their name.
#
# Use like this...
#$(foreach p,foo bar baz,$(eval $(call grass_raster_shorthand,$(p))))
#########################################################################
define grass_raster_shorthand
.PHONY: $(1)
$(1): $(rast)/$(1)
endef

define grass_vect_shorthand
.PHONY: $(1)
$(1): $(vect)/$(1)
endef



##############################################################################
# MASK defines
##############################################################################
define MASK
	(g.findfile element=cellhd file=MASK || g.copy rast=state@2km,MASK) > /dev/null;
endef

define NOMASK
	if ( g.findfile element=cellhd file=MASK > /dev/null); then g.remove MASK &>/dev/null; fi;
endef


.PHONY:clean

clean::
	@echo Nothing to clean in configure.mk
