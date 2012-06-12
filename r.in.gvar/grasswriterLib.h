#ifndef GRASSWRITERLIB_H
#define GRASSWRITERLIB_H

#include "Block.h"
#include <fstream>
#include <string>

extern "C" {
#include <grass/gis.h>
}

using namespace Gvar;

class grasswriterLib {
private:
   static const int m_numOfChannels = 6;

   // the ew and ns resolution for each channel
   static const int ew_res[grasswriterLib::m_numOfChannels];
   static const int ns_res[grasswriterLib::m_numOfChannels];

   int m_prevFrameId;
   int newFrameId;
   int framelinecnt;
   int prevlinecnt;
   Gvar::Block0 *m_block0 ;
   ofstream m_block0Out;
   // the file descriptor for each channel
   int ch_fd[grasswriterLib::m_numOfChannels];

   void *m_outs[grasswriterLib::m_numOfChannels] ;

   int m_numOfRowsPerChannel[grasswriterLib::m_numOfChannels] ;
   int m_numOfColsPerChannel[grasswriterLib::m_numOfChannels] ;

   double m_north ;
   double m_south ;
   double m_east ;
   double m_west ;

   struct Cell_head window[grasswriterLib::m_numOfChannels];
   void writeDataToChannel (int channelNo, uint16* data, int dataLen) ;

public:
   grasswriterLib () ;
   ~grasswriterLib () ;
   void write(Block* block);
   inline int getNumOfRowsPerChannel(int channelNo) {
     return m_numOfRowsPerChannel[channelNo] ;
   }
   inline int getNumOfColsPerChannel(int channelNo) {
     return m_numOfColsPerChannel[channelNo] ;
   }
   void resetNumOfRowsAndColsPerChannel() ;
//   char* gisloc;

};

#endif
