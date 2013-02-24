##
##  Makefile -- Build procedure for r.in.gvar module
##
MODULE_TOPDIR := ../../..

PGM = r.in.gvar

GVAR_INCLUDEDIR=/usr/local/include/libgvar
GVAR_LIBDIR=/usr/local/lib

GRASS_INCLUDEDIR=/usr/lib/grass64/include
EXTRA_INC := -I$(GVAR_INCLUDEDIR) 
LIBES = $(GISLIB) -L$(GVAR_LIBDIR) -lgvar
DEPENDENCIES = $(GISDEP)
MOD_OBJS = $(subst .c,.o,$(wildcard *.c)) $(subst .cpp,.o,$(wildcard *.cpp))

include $(MODULE_TOPDIR)/include/Make/Module.make
# For debugging
CXXFLAGS1 = -ggdb -Wall


default: cmd


