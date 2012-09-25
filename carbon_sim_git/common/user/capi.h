#ifndef CAPI_H
#define CAPI_H

#ifdef __cplusplus
extern "C" {
#endif

#define CAPI_ENDPOINT_ALL ((SInt32) 0x10000000)
#define CAPI_ENDPOINT_ANY ((SInt32) 0x20000000)

typedef enum {
   CARBON_NET_USER_1 = 0,
   CARBON_NET_USER_2
} carbon_network_t;

typedef int CAPI_return_t;
typedef int CAPI_endpoint_t;

CAPI_return_t CAPI_Initialize(int rank);
CAPI_return_t CAPI_rank(int *rank);
CAPI_return_t CAPI_message_send_w(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint, char * buffer, int size);
CAPI_return_t CAPI_message_receive_w(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint, char * buffer, int size);

CAPI_return_t CAPI_message_send_w_ex(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint, char * buffer, int size, carbon_network_t net_type);
CAPI_return_t CAPI_message_receive_w_ex(CAPI_endpoint_t send_endpoint, CAPI_endpoint_t receive_endpoint, char * buffer, int size, carbon_network_t net_type);

CAPI_return_t CAPI_set_filter(CAPI_endpoint_t tile_id_arg, unsigned int filter_mask, unsigned int filter_match_operation, unsigned int filter_match_signature);
void CAPI_subscribe_to_virtual_channel(CAPI_endpoint_t tile_id_arg, int vc_id);
void CAPI_unsubscribe_from_virtual_channel(CAPI_endpoint_t tile_id_arg, int vc_id);
CAPI_return_t CAPI_filter_incoming_messages(CAPI_endpoint_t tile_id_arg);
CAPI_return_t CAPI_dequeue_filtered_message(CAPI_endpoint_t tile_id_arg, int id, char *msg_to_receive, int msg_size);

enum {
   CAPI_StatusOk,
   CAPI_SenderNotInitialized,
   CAPI_ReceiverNotInitialized
};

#ifdef __cplusplus
}
#endif

#endif
