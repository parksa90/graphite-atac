#ifndef CARBON_USER_H
#define CARBON_USER_H

#ifdef __cplusplus
#include "atac_user.h"
extern "C"
{
#endif

#include <pthread.h>
#include "fixed_types.h"
#include "capi.h"
#include "sync_api.h"
#include "thread_support.h"
#include "perf_counter_support.h"
#include "dvfs.h"

SInt32 CarbonStartSim(int argc, char **argv);
void CarbonStopSim();
//core_id_t CarbonGetCoreId();
tile_id_t CarbonGetTileId();
UInt64 CarbonGetTime();

#ifdef __cplusplus
}
#endif

#endif // CARBON_USER_H
