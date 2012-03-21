# fix this relative to include/
# or use absolute path to the GRASS source code
MODULE_TOPDIR = ../../..

PGM = r.lse.by.pixel

# Libes did include $(HISTORYLIB)
LIBES = $(GISLIB) $(GPROJLIB) 
DEPENDENCIES = $(GISDEP)
EXTRA_INC = $(PROJINC)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
