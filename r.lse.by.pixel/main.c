#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <grass/gis.h>
#include <grass/glocale.h>

int main(int argc, char *argv[])
{
    int nrows, ncols;
    int i,j;
    int row, col;
    int verbose;
    static char* prog_name;   // argv[0]
    struct GModule *module;
    struct
    {
	struct Flag *q;
    } flag;

    G_debug(2,"Starting");
    
    G_gisinit(prog_name = argv[0]);		/* reads grass env, stores program name to G_program_name() */

    module = G_define_module();
    module->keywords = _("raster,linear regression");
    module->description =
      _("Outputs a least squares regression parameters on a"
	"per pixel basis for user-specified raster map layer(s).");

    /* Inputs */
    typedef struct {
      struct Option *option;
      char axis;
      int nfiles;
      int *fd;
      DCELL **buf;
    } InputRasts;

    union {
      InputRasts rast[2];
      struct {
	InputRasts x;
	InputRasts y;
      };
    } in;
    in.x.axis='x';
    in.y.axis='y';

    G_debug(2,"Inputs");

    for (i=0;i<sizeof(in)/sizeof(InputRasts);i++) {
      in.rast[i].option = G_define_option();
      in.rast[i].option->key          = "x";
      //      sprintf (in.rast[i].option->key,"%c",in.rast[i].axis);
      in.rast[i].option->type         = TYPE_STRING;
      in.rast[i].option->key_desc     = "name";
      in.rast[i].option->required     = YES;
      in.rast[i].option->multiple     = YES;
      in.rast[i].option->gisprompt    = "old,cell,raster";
      in.rast[i].option->description  = "Name of X raster maps";
      //      sprintf(in.rast[i].option->description,"Name of %c raster maps",in.rast[i].axis);
    }
    in.x.option->key          = "x";
    in.x.option->description="Name of x raster maps";
    in.y.option->key          = "y";
    in.y.option->description="Name of y raster maps";


    G_debug(2,"Outputs");


    /* Outputs */
    typedef struct {
      struct Option *option;
      int fd;
      DCELL *buf;
    } Rast;

    union {
      Rast rast[3];
      struct {
	Rast b;
	Rast m;
	Rast r2;
      };
    } o;

    for (i=0;i<sizeof(o)/sizeof(Rast);i++) {
      o.rast[i].option = G_define_option();
      o.rast[i].option->type         = TYPE_STRING;
      o.rast[i].option->required     = NO;
    }

    /* output names */
    o.b.option->key = "b";
    o.b.option->description = "Name for intercept (b) raster";  
    o.m.option->key = "m";
    o.m.option->description = "Name for slope (m) raster";  
    o.r2.option->key = "r2";
    o.r2.option->description = "Name for root mean squared (r2) raster";  
        
    flag.q = G_define_flag();
    flag.q->key = 'q';
    flag.q->description = _("Quiet");

    if (G_parser(argc, argv))
	exit(EXIT_FAILURE);

    /* flags */
    verbose = !flag.q->answer;

    /* Standard output buffer setups */
    /* Get Buffers and Open Files */
    for (i=0; i<sizeof(o)/sizeof(Rast); i++) {
      G_debug(2,"Checking output rast[%i] %s=%s",i,o.rast[i].option->key,o.rast[i].option->answer);
      if ( o.rast[i].option->answer ) {
	if (G_legal_filename(o.rast[i].option->answer) < 0)
	  G_fatal_error(_("[%s] is an illegal name"), o.rast[i].option->answer);
	
	if ( (o.rast[i].fd = G_open_raster_new (o.rast[i].option->answer, DCELL_TYPE)) < 0)
	  G_fatal_error("%s: Could not create <%s>", prog_name, o.rast[i].option->answer);
      }
      /* Always make the buffer however */
      o.rast[i].buf = G_allocate_raster_buf(DCELL_TYPE);
    }
    /* We also need sum and sum2 */
    DCELL *sumx = G_allocate_raster_buf(DCELL_TYPE);
    DCELL *sumxx = G_allocate_raster_buf(DCELL_TYPE);
    DCELL *sumy = G_allocate_raster_buf(DCELL_TYPE);
    DCELL *sumyy = G_allocate_raster_buf(DCELL_TYPE);
    DCELL *sumxy = G_allocate_raster_buf(DCELL_TYPE);


    /* count the number of input maps */
    G_debug(2,"Checking number of input files");
    for (i=0;i<sizeof(in)/sizeof(InputRasts);i++) {
      for (in.rast[i].nfiles = 0; in.rast[i].option->answers[in.rast[i].nfiles]; in.rast[i].nfiles++)
	;
    }
    if (in.x.nfiles != in.y.nfiles) {
      G_fatal_error(_("Different number of x and y rasters"));
    }
    for (i=0;i<sizeof(in)/sizeof(InputRasts);i++) {
      in.rast[i].fd   = (int *) G_malloc (in.rast[i].nfiles * sizeof(int));
      in.rast[i].buf = (DCELL **) G_malloc (in.rast[i].nfiles * sizeof (DCELL *));
      for (j = 0; j < in.rast[i].nfiles; j++) {
	char *name, *mapset;
	in.rast[i].buf[j] = G_allocate_d_raster_buf();
	name = in.rast[i].option->answers[i];
	mapset = G_find_cell(name, "");
	if (!mapset)
	  G_fatal_error(_("%s - raster map not found"), name);
	in.rast[i].fd[j] = G_open_cell_old (name, mapset);
	if (in.rast[i].fd[j] < 0)
	  G_fatal_error(_("%s - can't open raster map"), name);
      }
    }

    /* Now calculate the linear interpolation */
    nrows = G_window_rows();
    ncols = G_window_cols();
    
    if (verbose)
      fprintf (stderr, "%s: complete ... ", G_program_name());

    for (row = 0; row < nrows; row++)
      {
	if (verbose)
	  G_percent (row, nrows, 1);
	for (i = 0; i < in.x.nfiles; i++) {
	  if (G_get_d_raster_row (in.x.fd[i], in.x.buf[i], row) < 0)
	    exit(1);
	  if (G_get_d_raster_row (in.y.fd[i], in.y.buf[i], row) < 0)
	    exit(1);
	}
	for (col = 0; col < ncols; col++) {
	  int count=0;
	  sumx[col]=0;
	  sumxx[col]=0;
	  sumy[col]=0;
	  sumyy[col]=0;
	  sumxy[col]=0;
	  for(i=0; i< in.x.nfiles; i++) {
	    if (! G_is_d_null_value(&in.x.buf[i][col]) && 
		! G_is_d_null_value(&in.y.buf[i][col])) {
	      count++;
	      sumx[col]+=in.x.buf[i][col];
	      sumxx[col]+=in.x.buf[i][col]*in.x.buf[i][col];
	      sumy[col]+=in.y.buf[i][col];
	      sumyy[col]+=in.y.buf[i][col]*in.y.buf[i][col];
	      sumxy[col]+=in.x.buf[i][col]*in.y.buf[i][col];
	    }
	  }
	  double ssxx=sumxx[col]+count*sumx[col]*sumx[col];
	  double ssyy=sumyy[col]+count*sumy[col]*sumy[col];
	  double ssxy=sumxy[col]+count*sumx[col]*sumy[col];

	  o.m.buf[col]=ssxy/ssxx;
	  o.b.buf[col]=sumy[col]/count - o.m.buf[col]*sumx[col]/count;
	  o.r2.buf[col]=ssxy*ssxy/ssxx/ssyy;
	}
	/* write raster row to output raster file */
	int i;
	for (i=0; i<sizeof(o)/sizeof(Rast);i++) {
	  G_debug(3,"Save rast[%i] %s=%s ?",i,o.rast[i].option->key,o.rast[i].option->answer);
	  if ( o.rast[i].option->answer ) {
	    G_debug(3,"Writing row %i rast[%i] %s=%s",row,i,o.rast[i].option->key,o.rast[i].option->answer);      
	    if ( G_put_raster_row(o.rast[i].fd, o.rast[i].buf, DCELL_TYPE) < 0 )
	      G_fatal_error("%s: Cannot write to %s", prog_name, o.rast[i].option->answer);	    
	  }
	}
      } /* End of row */

    /* closing raster files */
    struct History hist;
    for (i=0; i<sizeof(o)/sizeof(Rast);i++) {
      char *name;
      name = o.rast[i].option->answer;
      if ( name ) {
	G_short_history(name, "raster", &hist);
	sprintf(hist.title, "%s",name);
	sprintf(hist.edhist[0], "Created from: r.lse.by.pixel" );
	G_write_history (name, &hist);
	G_close_cell(o.rast[i].fd);
      }
    }  
    exit(EXIT_SUCCESS);
}
