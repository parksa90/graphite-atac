#include "atac_perf_model_tester.h"

int AtacTester::first_pkt_time = 0;

void AtacTester::modifyPacketTime(NetPacket &pkt){
   if(CHIP_DELAY_TEST == 1){
      if(AtacTester::first_pkt_time == 0){
         AtacTester::first_pkt_time = pkt.init_time;
      }else{
         int transportDelay = pkt.time - pkt.init_time;
         pkt.init_time = AtacTester::first_pkt_time;
         pkt.time = AtacTester::first_pkt_time + transportDelay;
      }
   }
}