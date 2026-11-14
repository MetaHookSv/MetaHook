#ifndef NET_API_SVENGINE_H
#define NET_API_SVENGINE_H

#include "net_api.h"
#include <stddef.h>

// SvEngine 8948/10257 use a larger address ABI than the original HLSDK.
// Keep the engine-owned address payload opaque; pass it to AdrToString intact.
typedef struct netadr_svengine_s
{
    int           type; // NA_LOOPBACK and NA_IP retain their HLSDK values.
    unsigned char opaque_address[32];
} netadr_svengine_t;

typedef struct net_status_svengine_s
{
    int               connected;
    netadr_svengine_t local_address;
    netadr_svengine_t remote_address;
    int               packet_loss;
    double            latency;
    double            connection_time;
    double            rate;
} net_status_svengine_t;

// The net_api_t table layout is unchanged; these two slots take Sven types.
typedef void (*net_api_status_svengine_func_t)(net_status_svengine_t* status);
typedef char* (*net_api_adrtostring_svengine_func_t)(netadr_svengine_t* address);

#ifdef __cplusplus
static_assert(sizeof(netadr_svengine_t) == 36, "SvEngine network address ABI mismatch");
static_assert(offsetof(netadr_svengine_t, opaque_address) == 4, "SvEngine address payload offset mismatch");
static_assert(sizeof(net_status_svengine_t) == 104, "SvEngine network status ABI mismatch");
static_assert(offsetof(net_status_svengine_t, local_address) == 4, "SvEngine local address offset mismatch");
static_assert(offsetof(net_status_svengine_t, remote_address) == 40, "SvEngine remote address offset mismatch");
static_assert(offsetof(net_status_svengine_t, packet_loss) == 76, "SvEngine packet loss offset mismatch");
static_assert(offsetof(net_status_svengine_t, latency) == 80, "SvEngine latency offset mismatch");
static_assert(offsetof(net_status_svengine_t, connection_time) == 88, "SvEngine connection time offset mismatch");
static_assert(offsetof(net_status_svengine_t, rate) == 96, "SvEngine rate offset mismatch");
#endif

#endif // NET_API_SVENGINE_H
