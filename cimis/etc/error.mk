#! /usr/bin/make -f

ifndef configure.mk
include configure.mk
endif

error.mk:=1

define r.percent.err
.PHONY: $(1)_err

$(1)_err: $(rast)/$(1)_err

$(rast)/$(1)_err: $(rast)/$(1)
	$(call MASK)
	r.mapcalc '$(1)_err=abs($(2)*$(1))' &> /dev/null
	@r.colors map=$(1)_err rast=$(1)@default_colors > /dev/null
endef

