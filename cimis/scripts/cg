#!/bin/sh

############################################################################
#
# MODULE:       cg for GRASS 6
# AUTHOR(S):    Quinn Hart qjhart at ucdavis
# PURPOSE:      To perform Spatial CIMIS calculations
# COPYRIGHT:    (C) 2012 by Quinn Hart
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%Module
#%  description: Performs steps for Spatial CIMIS calculations
#%  keywords: CIMIS evapotranspiration
#%End
#%flag
#% key: l
#% description: List commands
#% guisection: Main
#%end
#%flag
#% key: n
#% description: dry-run (Pass -n to make command)
#% guisection: Main
#%end
#%flag
#% key: B
#% description: force (Pass -B to make command)
#% guisection: Main
#%end
#%option
#% key: cmd
#% type: string
#% description: Command 
#% multiple: yes
#% required: no
#% guisection: Main
#%end
#%option
#% key: sec
#% type: string
#% description: Spatial CIMIS command section
#% answer: cimis
#% required: yes
#% guisection: Main
#%end
#%option
#% key: etc
#% type: string
#% description: Debug etc location 
#% multiple: no
#% required: no
#% guisection: Main
#%end
#%option
#% key: make
#% type: string
#% description: Make command
#% answer: make --no-builtin-rules
#% multiple: no
#% required: no
#% guisection: Main
#%end

if  [ -z "$GISBASE" ] ; then
    echo "You must be in GRASS GIS to run this program."
    exit 1
fi

# save command line
if [ "$1" != "@ARGS_PARSED@" ] ; then
    CMDLINE=`basename "$0"`
    for arg in "$@" ; do
        CMDLINE="$CMDLINE \"$arg\""
    done
    export CMDLINE
    exec g.parser "$0" "$@"
fi

g.message -d message="$CMDLINE"

# check if we have make installed
if [ ! -x "`which make`" ] ; then
    g.message -e "'make' is required, please install it first"
    exit 1
fi

if [ ! -x "`which awk`" ] ; then
    g.message -e "'awk' is required, please install it first"
    exit 1
fi

# check if we have perl installed
if [ ! -x "`which perl`" ] ; then
    g.message -e "'perl' is required, please install it first"
    exit 1
fi

#######################################################################
# name:     exitprocedure
# purpose:  removes all temporary files
#
exitprocedure()
{
	g.message -e 'User break!'
	exit 1
}
trap "exitprocedure" 2 3 15

if [ -z $GIS_OPT_ETC ] ; then
    if [ -d ${GRASS_ADDON_ETC}/cg ] ; then
	etc=${GRASS_ADDON_ETC}/cg
    else
	etc=${GISBASE}/etc/cg
    fi
else
    etc=$GIS_OPT_ETC
fi

make=${GIS_OPT_MAKE}
if [ $GIS_FLAG_N -eq 1 ] ; then
    make="${make} -n"
fi

if [ $GIS_FLAG_B -eq 1 ] ; then
    make="${make} -B"
fi

#### Test for listing commands
#http://stackoverflow.com/questions/3063507/list-goals-targets-in-gnu-make
#make -qp | awk -F':' '/^[a-zA-Z0-9][^$#\/\t=]*:([^=]|$)/ {split($1,A,/ /);for(i in A)print A[i]}' 
if [ $GIS_FLAG_L -eq 1 ] ; then
    g.message -d message="${make} -I ${etc} -f ${etc}/${GIS_OPT_SEC}.mk -qp | awk -F':' '/^[a-zA-Z0-9][^$#\/\t=]*:([^=]|$)/ {split(\$1,A,/ /);for(i in A)print A[i]}' | sort -u"
    eval "${make} -I ${etc} -f ${etc}/${GIS_OPT_SEC}.mk -qp | awk -F':' '/^[a-zA-Z0-9][^$#\/\t=]*:([^=]|$)/ {split(\$1,A,/ /);for(i in A)print A[i]}' | sort -u "
else
    if [ -n "$GIS_OPT_CMD" ] ; then
	cmd=`echo ${GIS_OPT_CMD} | tr ',' ' '`
	g.message -d "${make} -I ${etc} -f ${etc}/${GIS_OPT_SEC}.mk ${cmd}"
	eval "${make} -I ${etc} -f ${etc}/${GIS_OPT_SEC}.mk ${cmd}"
	if [ $? -ne 0 ] ; then
	    g.message -e "make failure" 
	    exit 1
	fi
    else
	g.message -e message="Specify a cmd or -l"
    fi
fi
exit
