#define LINUX

#ifdef OS2
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <time.h>
#include <sys/time.h>
#include <fstream>
#include <errno.h>
//#include <string.h>

#ifdef LINUX
#include <netinet/tcp.h>
#endif

extern "C" {
#include <grass/gis.h>
#include <grass/glocale.h>
}

/* the header for prototypes and errors */
#include "grasswriterLib.h"
#include "Block.h"
#include "Block0Doc.h"
#include "LineDoc.h"
#include "cdaTime.h"

#define DIRPERMS 0755
#define BLOCK0_SIZE 8040

static const int m_numOfChannels = 6;

const int grasswriterLib::ew_res[m_numOfChannels] = {1,4,4,4,4,4};
const int grasswriterLib::ns_res[m_numOfChannels] = {1,4,4,4,4,4};

extern char* gisloc;

grasswriterLib::grasswriterLib () {
  m_prevFrameId = -1 ;
  m_block0 = NULL ;
  resetNumOfRowsAndColsPerChannel () ;
}

grasswriterLib::~grasswriterLib () {
  for (int channelNo=0; channelNo<grasswriterLib::m_numOfChannels; channelNo++) {
    if ( m_outs[channelNo] != NULL )
       m_outs[channelNo] = NULL;

    if ( ch_fd[channelNo] != 0 ) {
      G_close_cell(ch_fd[channelNo]);
     }
  }
  if (m_block0 != NULL) delete m_block0 ;
}

void grasswriterLib::resetNumOfRowsAndColsPerChannel() {
  for (int channelNo=0; channelNo<grasswriterLib::m_numOfChannels; channelNo++) {
    m_numOfRowsPerChannel[channelNo] = 0 ;
    m_numOfColsPerChannel[channelNo] = 0;
  }
}

void grasswriterLib::write(Block* block) {
  Header* header = block->getHeader () ;

  if (header->blockId () == 0) {
    if (m_block0 != NULL) {
      delete m_block0 ;
    }

    m_block0 = new Block0 (block) ;
    Block0Doc* block0doc = m_block0->getBlock0Doc();
    int newFrameId = block0doc->frame () ;
    framelinecnt = block0doc->nsln();

   // flush & close files of old frame if open
      if(m_block0Out.is_open()) {
        m_block0Out.close();
      }

    if (newFrameId != m_prevFrameId) {
    // new frame!!

     cout << "Start new frame: " << newFrameId << endl;

      for (int channelNo=0; channelNo<grasswriterLib::m_numOfChannels; 
	   channelNo++) {

           if ( ch_fd[channelNo] != 0 )
               G_close_cell(ch_fd[channelNo]);
      }

      m_prevFrameId = newFrameId;
  
      resetNumOfRowsAndColsPerChannel () ;
 
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

      CdaTime* curTime = m_block0->getBlock0Doc()->getCurrentTime();

      if(curTime == NULL) {
        cout << "Current time is NULL" << endl;
        exit(1);
      }
      
      char imcIdentifier[5] ;
      m_block0->getBlock0Doc ()->getImcIdentifier (imcIdentifier) ;
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

      for (int channelNo=0; channelNo<grasswriterLib::m_numOfChannels; 
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
      string block0File = (string)location + "/" + (string)mapset_name ; + "/block0file" ;
      m_block0Out.open (block0File.c_str (), 
			ios::out | ios::binary) ;

      //write Block0 info into block0file... metadata info
      m_block0Out.write ((const char *)block->getRawData(), block->getRawDataLen ()) ;
      m_block0Out.close () ;

    } // if newframe

     int cnt = framelinecnt - prevlinecnt;
     if (framelinecnt > 0 and prevlinecnt > 0  and cnt > 7 ) {
      //  cout <<"Missing lines: "<< cnt-1 << endl;
     }
     prevlinecnt = framelinecnt;

  } // end if block0
  else if (header->blockId () == 1 || 
           header->blockId () == 2) {
    if (m_block0 == NULL) {
      delete block ;
      return ;
    }

    Block1or2* block1or2 = new Block1or2 (block) ;
    uint16_t* data;
    LineDoc* lineDoc;

    for(int i=0; i<4; i++) {
        data = block1or2->getData(i) ;
        lineDoc = block1or2->getLineDoc(i) ;

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
    if (m_block0 == NULL) {
      delete block ;
      return ;
    }

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

void grasswriterLib::writeDataToChannel
(int channelNo, uint16_t* data, int dataLen) {
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

