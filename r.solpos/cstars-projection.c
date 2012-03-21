/*
 * cstars-projection: supporting functions for projections
 * 
 * Carlos A. Rueda
 * 2003-05-01
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <grass/gis.h>
#include <string.h>
#include <assert.h>
#include <grass/gprojects.h>
#include "global.h"
#include "cstars-projection.h"

static struct pj_info iproj;    /* input map proj parameters  */
static struct pj_info oproj;    /* output map proj parameters  */

// projection info:
static struct Key_Value *in_proj_info, *in_unit_info; /* projection information of input map */

static int transform_needed;


//////////////////////////////////////////////////////////////////////
void projection_init(struct Cell_head window)
{
  /* we don't like to run G_calc_solar_position in xy locations */
  if ( window.proj == 0 )
    G_fatal_error("Can't calculate sun position in xy locations. Specify sunposition directly.");
  
  transform_needed = 0;
  
  /* if coordinates are not in lat/long format, transform them: */
  if ((G_projection() != PROJECTION_LL) && window.proj != 0)
    {
      transform_needed = 1;
      
      /* read current projection info */
      if ((in_proj_info = G_get_projinfo()) == NULL)
	G_fatal_error("Can't get projection info of current location");
      
      if ((in_unit_info = G_get_projunits()) == NULL)
	G_fatal_error("Can't get projection units of current location");
      
      if (pj_get_kv(&iproj, in_proj_info, in_unit_info) < 0)
	G_fatal_error("Can't get projection key values of current location");
      
      /* set output projection to lat/long for solpos*/
      oproj.zone = 0;
      oproj.meters = 1.;
      sprintf(oproj.proj, "ll");
      if ((oproj.pj = pj_latlong_from_proj(iproj.pj)) == NULL)
	G_fatal_error("Unable to set up lat/long projection parameters");
    }
}


//////////////////////////////////////////////////////////////////////
void projection_transform(double *longitude, double *latitude)
{
  if ( transform_needed )
    {
      if(pj_do_proj(longitude, latitude, &iproj, &oproj) < 0)
	{
	  fprintf(stderr,"Error in pj_do_proj (projection of input coordinate pair)\n");
	  exit(0);
	}
    }
}

