/*
===================================================================
=   Program: r.in.gvar.cpp
=   
=   This program is to stream down GVAR format data from a GOES 
=   server with defined frame size, then write each frame as GRASS GIS
=   format files in a predefined region for late application.
=
=   Notes: gisdbase and location directory must be created first if they
=          are not existing and an initial PERMANENT mapset defined for 
=          the region must be presented as initial setup in your GRASS 
=          initial file (e.g. /home/user1/.grassrc). 
=
===================================================================
=
=   Required Modules, Libs, and Classes:
=   1) grasswriterLib.o, GvarStream.o, ShmRowFifo.o, GvarConverter.o 
=      and Objects under image subdirectory;
=
=   2) GRASS v 6.2 or late version libgrass_gis 
=      (also need libgrass_datetime if run 6.3 v);
=
=   3) In-house libgvar library;
=
=   4) If you are running GRASS v 6.2, "make_mapset.o" may be required.
=
===================================================================  
=  Modifications:
=  1) fix for Warning "string constant to char*" due to new g++ v 4.3
=
=
===================================================================
*/

#include "grasswriterLib.h"
#include "GvarStream.h"
#include "Block.h"
#include <unistd.h>      //  for sleep()
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>


// Define Grass GIS header files
extern "C" {
#include "grass/gis.h"
#include "grass/glocale.h"
}

using namespace std;
using namespace Geostream ;

int main(int argc, char* argv[]) {

  static char *prog_name;  // argv[0]
  struct GModule *module;
  struct Flag *quiet;
  int verbose;

  struct Flag *vip;

  char *ipaddr; // server ip address
  char *cport;  // char point for server port
  int iport;  // port number
  char *infile; // Input filename to use

  // following two are set to continue stream scanlines
  int channelNo = 1; // default channel
  int num_of_rows = 1000000; // default rows

 // G_debug(2,"Starting");
  G_gisinit(prog_name = argv[0]);
  module = G_define_module();
  module->keywords = strdup("raster, GOES, download");
  module->description = strdup("Reads in data from a GOES satellite server.");
  
  // Inputs  Should read from the command line
  struct Option *nserver;
  nserver = G_define_option();
  nserver->key          = strdup("server");
  nserver->type         = TYPE_STRING;
  nserver->key_desc     = strdup("server name");
  nserver->required     = NO;
  nserver->multiple     = NO;
  nserver->description  = strdup("location of the server");

  struct Option *nport;
  nport = G_define_option();
  nport->key          = strdup("port");
  nport->type         = TYPE_STRING;
  nport->key_desc     = strdup("port name");
  nport->required     = NO;
  nport->multiple     = NO;
  nport->description  = strdup("port of the server");

  struct Option *ninfile;
  ninfile = G_define_option();
  ninfile->key          = strdup("filename");
  ninfile->type         = TYPE_STRING;
  ninfile->key_desc     = strdup("input filename");
  ninfile->required     = NO;
  ninfile->multiple     = NO;
  ninfile->description  = strdup("Input file to read the stream");

  /* Flags */
  quiet = G_define_flag();
  quiet->key = 'q';
  quiet->description = strdup("Quiet");

  /* Flags */
  vip = G_define_flag();
  vip->key = 'v';
  vip->description = strdup("VIP File Format");

  if (G_parser(argc, argv))
    exit(EXIT_FAILURE);
    
  /* flags */
  verbose = !quiet->answer;

  GvarStream* gvar;

  if (nserver->answer && nport->answer ) {
    /* stores options and flags parser */
    ipaddr = nserver->answer;
    cport = nport->answer;
    
    // convert to int
    iport = atoi(cport); 
    
    // Start initial
    Block0* block0 = NULL;
    cout << "Server: "<< ipaddr << endl;
    cout << "Port: " << iport << endl;
    gvar = new GvarStream(ipaddr, iport);

    grasswriterLib* grasswriter = new grasswriterLib();
    while (true) { 
      bool succeed = gvar->listen();
      while (succeed && 
	 (num_of_rows == 0 || 
	  (grasswriter->getNumOfRowsPerChannel(channelNo - 1) < num_of_rows)
	  )
	 ) {
	Block *block = gvar->readBlock();
	  
	if (block == NULL) {
	  succeed = false ;
	} else {
	    grasswriter->write(block);
	  }
	}
      
      if (num_of_rows != 0 &&
	  (grasswriter->getNumOfRowsPerChannel(channelNo - 1) >= num_of_rows)
	  ) {
	  break ;
	}
      gvar->close () ;
      sleep (10) ;
    }    
    gvar->close () ;

    delete gvar ;
    delete grasswriter ;
    cout << "Done"<<endl;
  } else if (ninfile->answer) {
    infile = ninfile->answer;
    gvar = new GvarFile(infile);
    grasswriterLib* grasswriter = new grasswriterLib();
    
  } else {
    G_fatal_error("Either include a filename, or an address and port");
  }
  exit(EXIT_SUCCESS);
} 
