##
##  Makefile -- Build procedure for r.in.gvar module
##
MODULE_TOPDIR := /var/cache/apt-build/build/grass-6.4.0~rc5
GRASS_HOME := .
RUN_GISBASE:= .
#MODULE_TOPDIR := /usr/lib/grass64/

PGM = r.in.gvar

GVAR_INCLUDEDIR=/usr/local/include/libgvar
GVAR_LIBDIR=/usr/local/lib

EXTRA_INC := -I$(GVAR_INCLUDEDIR) -I/usr/lib/grass64/include
LIBES = $(GISLIB) -L$(GVAR_LIBDIR) -levent -lgvar -lpthread
DEPENDENCIES = $(GISDEP)
MOD_OBJS = $(subst .c,.o,$(wildcard *.c)) $(subst .cpp,.o,$(wildcard *.cpp))

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

$(OBJDIR)/%.o : %.cpp $(LOCAL_HEADERS) 
	@test -d $(OBJDIR) || mkdir $(OBJDIR)   
	$(CXX) $(CXXFLAGS) $(EXTRA_CFLAGS) $(NLS_CFLAGS) $(EXTRA_INC) $(INC)  -o $(OBJDIR)/$*.o -c $*.cpp


