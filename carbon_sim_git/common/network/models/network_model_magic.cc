#include "simulator.h"
#include "network.h"
#include "network_model_magic.h"
#include "log.h"

NetworkModelMagic::NetworkModelMagic(Network *net, SInt32 network_id)
   : NetworkModel(net, network_id)
{
   _frequency = 1.0;
   _flit_width = -1;
   _has_broadcast_capability = false;
}

NetworkModelMagic::~NetworkModelMagic()
{}

void
NetworkModelMagic::routePacket(const NetPacket &pkt, queue<Hop> &next_hops)
{
   // A latency of '1'
   int latency = 1;
   latency = Sim()->getCfg()->getInt("atac/design/delay_optical_network", 1);
   Hop hop(pkt, TILE_ID(pkt.receiver), RECEIVE_TILE, latency, 0);
   next_hops.push(hop);
}

void NetworkModelMagic::outputSummary(std::ostream &out)
{
   NetworkModel::outputSummary(out);
}
