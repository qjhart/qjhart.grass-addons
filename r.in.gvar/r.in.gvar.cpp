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

#include "grasswriterLib.h"
#include <Gvar.h>
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

// the file descriptor for each channel
int ch_fd[NUM_OF_CHANNELS];

void *m_outs[NUM_OF_CHANNELS] ;

int m_numOfRowsPerChannel[NUM_OF_CHANNELS] ;
int m_numOfColsPerChannel[NUM_OF_CHANNELS] ;

double m_north ;
double m_south ;
double m_east ;
double m_west ;

struct Cell_head window[NUM_OF_CHANNELS];

using namespace std;

void writeRawBlock(Block *block,string fn) {
  ofstream os;
  os.open (fn.c_str (), ios::out | ios::binary) ;  
  //write Block0 info into block0file... metadata info
  os.write ((const char *)block->getRawData(), 
		     block->getRawDataLen()) ;
  os.close () ;
}

void writeDataToChannel(int channelNo, uint16_t* data, int dataLen) {
  m_numOfRowsPerChannel[channelNo] ++ ;

  if(m_numOfColsPerChannel[channelNo] == 0)
    m_numOfColsPerChannel[channelNo] = dataLen ;

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
      m_numOfRowsPerChannel[channelNo] = 0 ;
      m_numOfColsPerChannel[channelNo] = 0;
    }
  }
}

void newFrame(Block *block,Block0Doc *block0doc) {
  closeFrame();
  // reset north, south, east, west values
  m_north =  block0doc->nsln(); 
  m_south = (block0doc->sln() + 1.0);
  m_east = (block0doc->epx()+ 1.0) ;
  m_west = block0doc->wpx() ;
  
  cout<<"nsln = "<< m_north << endl;
  cout<<"nln = "<< block0doc->nln() << endl;
  cout<<"sln = " << block0doc->sln() << endl;
  cout<<"epx = " << m_east << endl;
  cout<<"wpx = " << m_west << endl;
  
  CdaTime* curTime = block0doc->getCurrentTime();
  
  if(curTime == NULL) {
    cout << "Current time is NULL" << endl;
    exit(1);
  }
  
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
  
  if (G__make_mapset(NULL, NULL, mapset_name) != 0) {
    cout<< "Cannot create a new mapset: " << mapset_name << endl; 
  }

  G_setenv(_("MAPSET"), mapset_name);
  
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
    window[channelNo].cols = (int)(((float) (m_east - m_west ))/((float) ew_res[channelNo])+ 0.5);
    window[channelNo].rows = (int) (((float) (m_south - m_north ))/((float) ns_res[channelNo]) + 0.5);
    window[channelNo].ew_res = ew_res[channelNo];
    window[channelNo].ns_res = ns_res[channelNo];
    window[channelNo].format = 1;
    window[channelNo].compressed = 0;

    G_set_window(&window[channelNo]);

    if (G_put_cellhd(channel_name, &window[channelNo]) < 0 )
      G_fatal_error(_("Can't write cellhd for channel <%d>"), channelNo);
    
    RASTER_MAP_TYPE data_type = CELL_TYPE;
    
    if ((ch_fd[channelNo] = G_open_raster_new(channel_name, data_type)) < 0)
      G_fatal_error(_("Could not open <%s>"), channel_name);
    
    m_outs[channelNo] = (CELL*) G_allocate_raster_buf(data_type);
    
    m_numOfColsPerChannel[channelNo] = window[channelNo].cols;
    m_numOfRowsPerChannel[channelNo] = window[channelNo].rows;
    
  } // for loop

  // open new files to write
  string block0File = (string)location + "/" + (string)mapset_name + "/block0file" ;
  writeRawBlock(block,block0File);
}

void write(Block* block) {

  Header* header = block->getHeader () ;

  if (header->blockId () == 1 || 
           header->blockId () == 2) {
    Block1or2* block1or2 = new Block1or2 (block) ;
    uint16_t* data;
    LineDoc* lineDoc;

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

    Block3to10* block3to10 = new Block3to10 (block) ;
    uint16_t* data;
    LineDoc* lineDoc;
  
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

  char *ipaddr; // server ip address
  char *cport;  // char point for server port
  int iport;  // port number

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

  if (nserver->answer && nport->answer ) {
    Gvar::Stream* gvar;

    /* stores options and flags parser */
    ipaddr = nserver->answer;
    cport = nport->answer;
    
    // convert to int
    iport = atoi(cport); 
    
    // Start initial
    cout << "Server: "<< ipaddr << endl;
    cout << "Port: " << iport << endl;
    gvar = new Gvar::Stream(ipaddr, iport);

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

    Gvar::Block *last[4];
    Block0Doc *scan = NULL;
    Block0Doc *frame = NULL;
    char *infile = ninfile->answer;
    Gvar::File* gvar = new Gvar::File(infile);
    
    if (vip->answer) {
      gvar->readVIPHeader();
    }
    int count = 0;
    Header *header;
    while ( (header = gvar->readHeader()) ) {
      Block *block = gvar->readBlock(header);
      int id=header->blockId();
      if ( block->crc16_ok() )  { // Good
	if (id == 0) {
	  Block0 *block0 = new Block0(block);
	  scan = block0->getBlock0Doc();
	  if ((frame == (Block0Doc *)NULL) || 
	      (scan->frame() != frame->frame())) {
	    frame=scan;
	    std::cout << "Start new frame: " << frame->frame() << endl;
	    newFrame(block,frame);
	  }
	  //	  std::cerr << scan->aScanCount() << endl;
	} else if (frame != (Block0Doc *)NULL) {
	  if (id <=2) {
	    write(block);
	    last[id]=block;
	  } else if (id >= 3 && id <= 10) {
	    write(block);
	    last[3]=block;
	  }
	}
      } else {
	if ( frame != (Block0Doc *)NULL) {
	  std::cerr << "Bad Block:"<<header->blockId()<<" Frame:"<<scan->frame()<<" ScanCnt:"<<scan->aScanCount()<< endl;
	  if (id==1 || id == 2) {
	    write(last[id]);
	  } else if (id <= 10) {
	    write(last[3]);
	  }
	}
      }
    }
    std::cerr << "Closing Frame" << endl;
    closeFrame();
    gvar->close();
  } else {
    G_fatal_error("Either include a filename, or an address and port");
  }
  exit(EXIT_SUCCESS);
} 
