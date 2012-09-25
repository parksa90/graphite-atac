#ifndef ATAC_PERF_MODEL_TESTER_H
#define ATAC_PERF_MODEL_TESTER_H


#include "network.h"

#define CHIP_DELAY_TEST 1

class AtacTester{
public:
   static void modifyPacketTime(NetPacket &pkt);	
   static int first_pkt_time;
};



#endif