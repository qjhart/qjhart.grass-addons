/*
 * cstars-solpos: supporting functions for SOLPOS
 * 
 * Carlos A. Rueda
 * 2003-05-01
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <grass/gis.h>
#include <string.h>
#include <assert.h>
#include "solpos00.h"
#include "global.h"
#include "cstars-solpos.h"
 
//////////////////////////////////////////////////////////////////////
void solpos_init(struct posdata *pdat, double timezone, int year, int month, int day, int hour, int minute, int second)
{
  S_init (pdat);
  
  pdat->timezone  = timezone;   /* DO NOT ADJUST FOR DAYLIGHT SAVINGS TIME. */
  
  pdat->year      = year;
  pdat->month     = month;
  pdat->day       = day; 
  pdat->hour      = hour;
  pdat->minute    = minute;
  pdat->second    = second;
  
  pdat->temp      =   20.0;
  pdat->press     = 1013.0;
  
  pdat->aspect    = 180.0;
}

//////////////////////////////////////////////////////////////////////
void solpos_set_hour(struct posdata *pdat, int hour, int minute, int second)
{
  pdat->hour      = hour;
  pdat->minute    = minute;
  pdat->second    = second;
}


//////////////////////////////////////////////////////////////////////
long solpos_calculate(struct posdata *pdat, double longitude, double latitude)
{
  long retval;             /* to capture S_solpos return codes */
  
  pdat->longitude = longitude;  /* Note that latitude and longitude are  */
  pdat->latitude  = latitude;   /*   in DECIMAL DEGREES, not Deg/Min/Sec */
  pdat->tilt      = pdat->latitude;  /* tilted at latitude */
  
  pdat->function &= ~S_DOY;
  
  retval = S_solpos (pdat);  /* S_solpos function call: returns long value */
  S_decode(retval, pdat);    /* prints an error in case of problems */
  
  return retval;
}

