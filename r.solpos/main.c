/*
 * r.solpos: Solar position info generation based on GRASS r.sunmask and NREL solpos
 * 
 * See module->description below
 *
 * Carlos A. Rueda. Created 2003-04-22
 * $Id: main.c,v 1.1.1.1 2009-03-11 05:57:03 qjhart Exp $
 *********************************************************************/

#define MAIN

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <values.h>   // for MAXDOUBLE
#include <grass/gis.h>
#include <grass/gprojects.h>
#include <string.h>
#include <assert.h>
#include "solpos00.h"

#include "global.h"
#include "timezone.h"
#include "cstars-projection.h"
#include "cstars-solpos.h"

static char* prog_name;   // argv[0]

static struct posdata pd;
static struct posdata *pdat = &pd;

static struct Cell_head window;


static char* module_description() {
  static char str[1024];
  sprintf(str,
	  "CSTARS r.solpos  0.1 (%s %s) Solar position info generation based on GRASS r.sunmask and NREL solpos", __DATE__, __TIME__
	  );
  return str;
}


static void report(int verbose) {
  if ( verbose ) {
    fprintf(stdout, " %d-%02d-%02d, daynum %d, time: %02i:%02i:%02i  timezone=%f\n",
	    pdat->year, pdat->month, pdat->day, pdat->daynum,
	    pdat->hour, pdat->minute, pdat->second,
	    pdat->timezone
	    );
    fprintf(stderr, " sretr=%02.0f:%02.0f ssetr=%02.0f:%02.0f\n", 
	    floor(pdat->sretr/60.), fmod(pdat->sretr, 60.), 
	    floor(pdat->ssetr/60.), fmod(pdat->ssetr, 60.)
	    );
  }
  
  fprintf(stdout, 
	  "longitude=%f\n\
latitude=%f\n\
dayang=%f\n\
declin=%f\n\
eqntim=%f\n\
cosinc=%f\n\
etr=%f\n\
etrn=%f\n\
etrtilt=%f\n\
azim=%f\n\
elevref=%f\n\
sretr=%f\n\
ssetr=%f\n\
ssha=%f\n\
", 	
	  pdat->longitude, pdat->latitude,
	  pdat->dayang,
	  pdat->declin,
	  pdat->eqntim,
	  pdat->cosinc,
	  pdat->etr,
	  pdat->etrn,
	  pdat->etrtilt,
	  pdat->azim,           // sun azimuth
	  pdat->elevref,        // sun angle above horz.(refraction corrected
	  pdat->sretr,          // Sunrise time, minutes (without refraction)
	  pdat->ssetr,          // Sunset time, minutes  (without refraction)
	  pdat->ssha            
	  );
}

typedef struct {
  struct Option *option;
  int outfd;
  unsigned char *outrast;
  
  double stats_min, stats_max, stats_avg;  // for a simple stats on generated values
  
} OutputInfo;


#define NUM_OUTPUTS 6

static OutputInfo outputs[NUM_OUTPUTS];

static char* output_keys[NUM_OUTPUTS] = { "ssha", "hrang", "sretr", "ssetr", "long", "lat"  };
static char output_descriptions[NUM_OUTPUTS][1024];


static void output_init_options() {
  int ii;
  for ( ii = 0; ii < NUM_OUTPUTS; ii++ ) {
    outputs[ii].option = G_define_option();
    outputs[ii].option->key = output_keys[ii];
    outputs[ii].option->type = TYPE_STRING;
    outputs[ii].option->required = NO;
    outputs[ii].option->description = output_descriptions[ii];
    sprintf(output_descriptions[ii], "Name for '%s' raster", output_keys[ii]);
    
  }
}

// returns the number of rasters to create
static int output_init_rasters() {
  int number = 0;
  int ii;
  for ( ii = 0; ii < NUM_OUTPUTS; ii++ ) {
    // stats:
    outputs[ii].stats_min = MAXDOUBLE;
    outputs[ii].stats_max = -MAXDOUBLE;
    outputs[ii].stats_avg = 0.0;
    if ( outputs[ii].option->answer ) {
      outputs[ii].outrast = G_allocate_raster_buf(FCELL_TYPE);
      if ( (outputs[ii].outfd = G_open_raster_new (outputs[ii].option->answer, FCELL_TYPE)) < 0)
	G_fatal_error("%s: Could not create %s", prog_name, outputs[ii].option->answer);
      
      number++;
    }
  }  
  return number;
}


static double get_value(char* param_name) {
  if ( strcmp(param_name, "hrang") == 0 )
    return (double) pdat->hrang;
  else if ( strcmp(param_name, "ssha") == 0 )
    return (double) pdat->ssha;
  else if ( strcmp(param_name, "sretr") == 0 )
    return (double) pdat->sretr;
  else if ( strcmp(param_name, "ssetr") == 0 )
    return (double) pdat->ssetr;
  else if ( strcmp(param_name, "long") == 0 )
    return (double) pdat->longitude;
  else if ( strcmp(param_name, "lat") == 0 )
    return (double) pdat->latitude;
  
  assert(0 && "impossible");
}


static void output_add_column(int col) {
  int ii;
  for ( ii = 0; ii < NUM_OUTPUTS; ii++ ) {
    double rval = get_value(output_keys[ii]);
    
    // stats:
    if ( outputs[ii].stats_min > rval ) outputs[ii].stats_min = rval;
    if ( outputs[ii].stats_max < rval ) outputs[ii].stats_max = rval;
    outputs[ii].stats_avg += rval;
    
    if ( outputs[ii].option->answer )
      ((FCELL *) outputs[ii].outrast)[col] = (FCELL) rval;
  }
}

static void output_put_row() {
  int ii;
  for ( ii = 0; ii < NUM_OUTPUTS; ii++ ){
    if ( outputs[ii].option->answer ) {
      if ( G_put_raster_row(outputs[ii].outfd, outputs[ii].outrast, FCELL_TYPE) < 0 )
	G_fatal_error("%s: Cannot write to %s", prog_name, outputs[ii].option->answer);
    }
  }
}

static void output_end(int total) {
  int ii;
  for ( ii = 0; ii < NUM_OUTPUTS; ii++ ) {
    outputs[ii].stats_avg /= total;
    if ( outputs[ii].option->answer ) {
      G_free(outputs[ii].outrast);
      G_close_cell(outputs[ii].outfd);
      
      fprintf(stderr, "   '%s' created. min=%f max=%f avg=%f\n",
	      outputs[ii].option->answer,
	      outputs[ii].stats_min, outputs[ii].stats_max, outputs[ii].stats_avg
	      );
    }
  }
}

int main(int argc, char *argv[]){
  struct {
    struct Option *date, *east, *north;
  }parm;
  
  struct Flag *report_flag, *verbose_flag;
  struct GModule *module;
  int no_rasters;
  
  char str_tz[64];
  double time_zone;
  double east, north;
  int year, month, day, hour, minutes, seconds;
  long retval;
  
  
  double x, y;
  
  
  int lines, samples;
  int lin,col;
  
  
  G_gisinit(prog_name = argv[0]);
  
  module = G_define_module();
  module->description = module_description(); 
  
  parm.date = G_define_option();
  parm.date->key = "date";
  parm.date->type = TYPE_STRING;
  parm.date->required = YES;
  parm.date->description = "Date YYYY-MM-DDTHH:MM:SS ZZZZ.  YYYY-MM-DD: required; Defaults: Time: 12:00:00 ZZZZ: time zone";
  
  parm.east = G_define_option();
  parm.east->key = "east";
  parm.east->type = TYPE_INTEGER;
  parm.east->required = NO;
  parm.east->description = "East coordinate (default: center of current region)";
  
  parm.north = G_define_option();
  parm.north->key = "north";
  parm.north->type = TYPE_INTEGER;
  parm.north->required = NO;
  parm.north->description = "North coordinate (default: center of current region)";
  
  
  output_init_options();
  
  // flags: 
  report_flag = G_define_flag() ;
  report_flag->key         = 'r' ;
  report_flag->description = "Report for given north/east coordinates (center of region by default)" ;
  
  verbose_flag = G_define_flag() ;
  verbose_flag->key         = 'v' ;
  verbose_flag->description = "Verbose" ;
  
  
  if (G_parser(argc, argv))
    exit(-1);
  
  G_get_window (&window);
  
  
  // noon time, by default:
  hour = 12;	
  minutes = seconds = 0;	
  *str_tz = 0;
  
  if ( sscanf(parm.date->answer, "%d-%d-%dT%d:%d:%d %s", 
	      &year, &month, &day, &hour, &minutes, &seconds, str_tz
	      ) < 3
       ) {
    G_fatal_error("%s: Please specify at least YYYY-MM-DD. Given date=[%s]", 
		  prog_name, parm.date->answer);
  }
  
  // check if timezone was given in date:
  if ( strlen(str_tz) > 0 ) {
    if ( scan_timezone(str_tz, &time_zone) )
      G_fatal_error("%s: Invalid timezone '%s'", prog_name, str_tz);
  }
  else {
    time_zone = get_timezone(year, month, day);
  }
	
  solpos_init(pdat, time_zone, year, month, day, hour, minutes, seconds);
  projection_init(window);
  
  // process:
  no_rasters = output_init_rasters();
  
  if ( report_flag->answer && no_rasters > 0 )
    G_fatal_error("%s: -r does not output rasters", prog_name);
  
  if ( report_flag->answer ) {
    // initially, center of current region.
    east =  (double) (window.west + window.east) / 2;
    north = (double) (window.north + window.south) / 2;
    
    if ( parm.east->answer )
      sscanf(parm.east->answer, "%lf", &east);
    
    if ( parm.north->answer )
      sscanf(parm.north->answer, "%lf", &north);
    
    if ( verbose_flag->answer )
      fprintf(stderr, " north=%f east=%f\n", north, east);
    
    projection_transform(&east, &north);
    retval = solpos_calculate(pdat, east, north);
    if ( retval == 0 )
      report(verbose_flag->answer);
    else
      G_fatal_error("%s: Please correct settings.", prog_name);
    
    return 0;
  }
  else if ( no_rasters > 0 ) {
    if ( verbose_flag->answer )
      fprintf(stderr, "Generating: ");
    
    lines = G_window_rows();
    samples = G_window_cols();

    for ( y = window.north-window.ns_res/2, lin = 0; lin < lines; y -= window.ns_res, lin++ ) {
      if ( verbose_flag->answer )
	G_percent(lin + 1, lines, 2);
      
      for ( x = window.west+window.ew_res/2, col = 0; col < samples; x += window.ew_res, col++ ) {
	double longitude = x;
	double latitude = y;
	
	projection_transform(&longitude, &latitude);
	retval = solpos_calculate(pdat, longitude, latitude);
	
	if (retval)
	  G_fatal_error("%s: Please correct settings.", prog_name);
	
	output_add_column(col);
      }
      output_put_row();
    }
    output_end(lines * samples);
  }
  else
    G_fatal_error("%s: Please specify -r or one or more output rasters.", prog_name);
  
  return 0;
}


