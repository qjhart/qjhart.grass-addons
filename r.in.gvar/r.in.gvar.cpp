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

//#include "grasswriterLib.h"
#include <Block.h>
#include <fstream>
#include <string>
#include <Gvar.h>
#include <unistd.h>      //  for sleep()
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

// Define Grass GIS header files
extern "C" {
#include "grass/gis.h"
#include "grass/glocale.h"
}

// the file descriptor for each channel
int ch_fd[NUM_OF_CHANNELS];

void *m_outs[NUM_OF_CHANNELS] ;

struct Cell_head window[NUM_OF_CHANNELS];

void sig_handler(int signo)
{
  if (signo == SIGINT)
    G_fatal_error("received SIGINT\n");
  else 
    G_fatal_error(_("Received message %d"),signo);
}

using namespace std;

void writeRawBlock(Gvar::Block *block,string fn) {
  ofstream os;
  os.open (fn.c_str (), ios::out | ios::binary) ;  
  //write Block0 info into block0file... metadata info
  os.write ((const char *)block->getRawData(), 
		     block->getRawDataLen()) ;
  os.close () ;
}

void writeDataToChannel(int channelNo, uint16_t* data, int dataLen) {
  CELL val;
  //RASTER_MAP_TYPE data_type = CELL_TYPE;
  for(int j=0; j<dataLen; j++) {
     val = (CELL) data[j];
     ((CELL*)m_outs[channelNo])[j] = val;
  }
  if (G_put_map_row(ch_fd[channelNo], (CELL*) m_outs[channelNo]) < 0)
      G_fatal_error (_("Can't write map row for channel <%d>"), channelNo);
}

void closeFrame() {
  for (int channelNo=0; channelNo<NUM_OF_CHANNELS; channelNo++) {    
    if ( ch_fd[channelNo] != 0 ) {
      G_close_cell(ch_fd[channelNo]);
      ch_fd[channelNo] = 0;
    }
  }
}

void newFrame(Gvar::Block *block,Gvar::Block0Doc *block0doc) {
  // reset north, south, east, west values
  double m_north =  block0doc->nsln(); 
  double m_south = (block0doc->sln() + 1.0);
  double m_east = (block0doc->epx()+ 1.0) ;
  double m_west = block0doc->wpx() ;

  CdaTime* curTime = block0doc->getCurrentTime();
  
  if(curTime == NULL)
    G_fatal_error(_("Current time is NULL"));
  
  char imcIdentifier[5] ;
  block0doc->getImcIdentifier (imcIdentifier) ;
  imcIdentifier[4] = '\0' ;
  
  // create the parent directory for current frames
  char* locationString =  curTime->getYMDEpoch(imcIdentifier) ;
  
  char* strtmp = locationString;
  strtmp[10] = '\0'; 
  
  char timeHH[3];
  char timeMM[3];
  
  sprintf (timeHH, "%.2d", curTime->hrs());
  sprintf (timeMM, "%.2d", curTime->min());
  
  G_strip((char*)&timeHH);
  G_strip((char*)&timeMM); 
  
  G_strcat(strtmp, "T");
  G_strcat(strtmp, (char*)&timeHH);
  G_strcat(strtmp, (char*)&timeMM);
  G_strip(strtmp);
  
  if (strlen(strtmp) >= 15) {
    strtmp[15] = '\0';
  }
  char* strmap = strtmp;
  char* location = G_location_path();
  
  char* mapset_name = strmap;
  
  if (G__make_mapset(NULL, NULL, mapset_name) != 0)
    G_message(_("Cannot create a new mapset: %s"),mapset_name);

  G_setenv(_("MAPSET"), mapset_name);

  G_message(_("%s: n=%g s=%g, e=%g w=%g"),
	    mapset_name,m_north,m_south,m_east,m_west);
  
  for (int channelNo=0; channelNo<NUM_OF_CHANNELS; 
       channelNo++) {
    
    char channel_name[10];
    sprintf(channel_name, "ch%d", (channelNo+1));
    if (G_legal_filename( channel_name ) < 0) {
      G_fatal_error (_("Illegal cell file name <%s>"), channel_name);
    }
    
    window[channelNo].north = m_north * (-1.0);
    window[channelNo].south = m_south * (-1.0);
    window[channelNo].east = m_east;
    window[channelNo].west = m_west;
    window[channelNo].proj = 99;
    window[channelNo].zone = 0;
    window[channelNo].cols = (int)(((float) (m_east - m_west ))/((float) Gvar::ew_res[channelNo])+ 0.5);
    window[channelNo].rows = (int) (((float) (m_south - m_north ))/((float) Gvar::ns_res[channelNo]) + 0.5);
    window[channelNo].ew_res = Gvar::ew_res[channelNo];
    window[channelNo].ns_res = Gvar::ns_res[channelNo];
    window[channelNo].format = 1;
    window[channelNo].compressed = 0;

    G_set_window(&window[channelNo]);

    if (G_put_cellhd(channel_name, &window[channelNo]) < 0 )
      G_fatal_error(_("Can't write cellhd for channel <%d>"), channelNo);
    
    RASTER_MAP_TYPE data_type = CELL_TYPE;
    
    if ((ch_fd[channelNo] = G_open_raster_new(channel_name, data_type)) < 0)
      G_fatal_error(_("Could not open <%s>"), channel_name);
    
    m_outs[channelNo] = (CELL*) G_allocate_raster_buf(data_type);
  } // for loop

  // open new files to write
  string block0File = (string)location + "/" + (string)mapset_name + "/block0file" ;
  writeRawBlock(block,block0File);
}

void write(Gvar::Block* block) {

  Gvar::Header* header = block->getHeader () ;

  if (header->blockId () == 1 || 
           header->blockId () == 2) {
    Gvar::Block1or2* block1or2 = new Gvar::Block1or2 (block) ;
    uint16_t* data;
    Gvar::LineDoc* lineDoc;

    for(int i=0; i<4; i++) {
        lineDoc = block1or2->getLineDoc(i) ;
        data = block1or2->getData(i) ;
	if(lineDoc->licha() >= 2 && lineDoc->licha() <= 6 ) {
	    writeDataToChannel (lineDoc->licha()-1, data, 
				block1or2->getDataLen(i)) ;
	}
    }
    delete block1or2;
    delete block ;
  }// end else if block 1-2
  else if (header->blockId () >= 3 && 
           header->blockId () <= 10) {

    Gvar::Block3to10* block3to10 = new Gvar::Block3to10 (block) ;
    uint16_t* data;
    Gvar::LineDoc* lineDoc;
  
    data = block3to10->getData();
    lineDoc = block3to10->getLineDoc() ;

    if(lineDoc->licha() == 1) {
      if((lineDoc->lidet() >= 1) && (lineDoc->lidet() <= 8)) {
	writeDataToChannel (lineDoc->licha()-1, data, 
			    block3to10->getDataLen()) ;
      }
    }//if
    delete block3to10;
    delete block ;
  } // end else if block 3-10

  else { // skip other blocks
    delete block ;
  }

}

//***

int main(int argc, char* argv[]) {

  static char *prog_name;  // argv[0]
  struct GModule *module;
  struct Flag *quiet;
  int verbose;

  struct Flag *vip;

  // Catch Signals for the daemon
  if (signal(SIGINT, sig_handler) == SIG_ERR)
    G_fatal_error("Can't catch SIGINT");

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
  vip->key = 'V';
  vip->description = strdup("VIP File Format");

  if (G_parser(argc, argv))
    exit(EXIT_FAILURE);
    
  /* flags */
  verbose = !quiet->answer;

  Gvar::IO *gvar;

  if (nserver->answer && nport->answer ) {
    char *ipaddr = nserver->answer;
    char *cport = nport->answer;
    
    // convert to int
    int iport = atoi(cport); 
    
    // Start initial
    cout << "Server: "<< ipaddr << endl;
    cout << "Port: " << iport << endl;
    Gvar::Stream* gvars = new Gvar::Stream(ipaddr, iport);

    bool succeed = gvars->listen();
    if (! succeed) {
	gvars->close();
	G_fatal_error ("Can't open Socket");
      }
    gvar = gvars;
  } else if (ninfile->answer) {
    char *infile = ninfile->answer;
    Gvar::File* gvarf = new Gvar::File(infile);
    if (vip->answer) {
      G_message("Reading VIP Header");
      gvarf->readVIPHeader();
    }
    gvar=gvarf;
  } else {
  G_fatal_error("Either include a filename, or an address and port");
  }

  Gvar::Block *last[4];
  Gvar::Block0Doc *scan = NULL;
  Gvar::Block0Doc *frame = NULL;
  Gvar::Header *header;

  while ( header = gvar->readHeader() ) {
    // header->print(cout);
    Gvar::Block *block = gvar->readBlock(header);
    int id=header->blockId();
    if ( block->crc16_ok() )  { // Good
      if (id == 0) {
     	Gvar::Block0 *block0 = new Gvar::Block0(block);
     	scan = block0->getBlock0Doc();
	if ( (frame == (Gvar::Block0Doc *)NULL) || 
	     (scan->frame() != frame->frame()) ) {
	  if (frame != (Gvar::Block0Doc *)NULL) {
	    closeFrame();
	  }
	  frame=scan;
	  // G_important_message(_("Start new frame: <%d>"),frame->frame());
	  newFrame(block,frame);
	}
	G_debug(4,_("Scan <%d>"),scan->aScanCount());
      } else if (frame != (Gvar::Block0Doc *)NULL) {
     	if (id <=2) {
     	  write(block);
     	  last[id]=block;
     	} else if (id >= 3 && id <= 10) {
     	  write(block);
     	  last[3]=block;
     	}
      }
    } else {
      if ( frame != (Gvar::Block0Doc *)NULL) {
	G_message(_("Bad Block: <%d> Frame: <%d> Scan:<%d>"),
		  header->blockId(),scan->frame(),scan->aScanCount());
	if (id==1 || id == 2) {
	  write(last[id]);
	} else if (id <= 10) {
	  write(last[3]);
	}
      }
    }
  }
  closeFrame();
  gvar->close();
  exit(EXIT_SUCCESS);
}
