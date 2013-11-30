#ifndef __GEOSTREAM_GVARSTREAM_H
#define __GEOSTREAM_GVARSTREAM_H

#include "LineDoc.h"
#include "Block.h"

#define GVNETBUFFSIZE 32768
#define NUM_OF_CHANNELS 6

namespace Geostream {

  /*
    OLD GVAR Version I-L
  static const char* channelsDesc[NUM_OF_CHANNELS] = {
    "Channel 1, 0.52-0.75microns - cloud cover",
    "Channel 2,   3.8-4.0microns - night cloud cover",
    "Channel 3,   6.4-7.0microns - water vapor",
    "Channel 4, 10.2-11.2microns - sea surface temp.",
    "Channel 5, 11.5-12.5microns - sea surface temp."
  } ;

  static const int xRes[NUM_OF_CHANNELS] = {1,4,4,4,4} ;
  static const int yRes[NUM_OF_CHANNELS] = {1,4,8,4,4} ;
  */

  // GOES Version O-Beyond
  static const char* channelsDesc[NUM_OF_CHANNELS] = {
    "Channel 1, 0.52-0.75 microns - cloud cover",
    "Channel 2,   3.8-4.0 microns - night cloud cover",
    "Channel 3,   6.4-7.0 microns - water vapor",
    "Channel 4, 10.2-11.2 microns - sea surface temp.",
    "Channel 5, 11.5-12.5microns - sea surface temp."
    "Channel 6, 13.3 microns - sea surface temp."
  } ;

  static const int ew_res[NUM_OF_CHANNELS] = {1,4,4,4,4,4} ;
  static const int ns_res[NUM_OF_CHANNELS] = {1,4,4,4,4,4} ;


  class GvarStream {
  private:
	unsigned int seqnum;
	char *ipaddr;
	int port;
	int fd;
	int numread;
	uint8_t *bp;
	uint8_t *end;
	uint8_t blkbuf[GVNETBUFFSIZE];

	// store the last block0 we have seen
	Gvar::Block0 *m_block0 ;

	// store the last frame id we have seen
	int m_prevFrameId ;

	int connect(void);

  public:
	GvarStream(char *ipaddr,int port);

        GvarStream(char* filename);
 
	~GvarStream();
	bool listen(void);

	void close () ;

        Gvar::Block* readBlock () ;

  };
}

#endif
