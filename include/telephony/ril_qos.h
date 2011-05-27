/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef ANDROID_RIL_QOS_H
#define ANDROID_RIL_QOS_H 1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Defines a set of roles which have a pre determined set of flow and filter
 * specs
 *
 */
typedef enum {
    RIL_QOS_CONVERSTATIONAL,
    RIL_QOS_STREAMING,
    RIL_QOS_INTERACTIVE,
    RIL_QOS_BACKGROUND
} RIL_QosClass;

typedef enum {
    RIL_QOS_TX,
    RIL_QOS_RX
} RIL_QosDirection;

/* QoS status */
typedef enum {
    RIL_QOS_STATUS_NONE,        /* Qos not active */
    RIL_QOS_STATUS_ACTIVATED,   /* Qos currently active */
    RIL_QOS_STATUS_SUSPENDED    /* Qos Suspended */
} RIL_QosStatus;

/* Enum for status of the QoS flows */
typedef enum {
    RIL_QOS_SUCCESS,             /* QoS operation complete */
    RIL_QOS_NEGOTIATED,          /* QoS setup resulted in a neogtiated value */
    RIL_QOS_USER_RELEASE,        /* QoS released by the user */
    RIL_QOS_NETWORK_RELEASE,     /* QoS released by the network */
    RIL_QOS_ERROR_UNKNOWN        /* Any other error */
} RIL_QosIndStates;

/* Keys the QoS spec along with the description of their values */
typedef enum {
    RIL_QOS_SPEC_INDEX,                         /* Positive numerical value */

    RIL_QOS_FLOW_DIRECTION,                     /* RIL_QosDirection */
    RIL_QOS_FLOW_TRAFFIC_CLASS,                 /* RIL_QosClass */
    RIL_QOS_FLOW_DATA_RATE_MIN,                 /* Positive number in bps */
    RIL_QOS_FLOW_DATA_RATE_MAX,                 /* Positive number in bps */
    RIL_QOS_FLOW_LATENCY,                       /* Positive number in milliseconds */

    RIL_QOS_FLOW_3GPP2_PROFILE_ID,              /* Positive numerical value */
    RIL_QOS_FLOW_3GPP2_PRIORITY,                /* Positive numerical value */

    RIL_QOS_FILTER_DIRECTION,                   /* RIL_QosDirection */
    RIL_QOS_FILTER_IPV4_SOURCE_ADDR,            /* Format: xxx.xxx.xxx.xxx/yy */
    RIL_QOS_FILTER_IPV4_DESTINATION_ADDR,       /* Format: xxx.xxx.xxx.xxx/yy */
    RIL_QOS_FILTER_IPV4_TOS,                    /* Positive numerical Value (max 6-bit number) */
    RIL_QOS_FILTER_IPV4_TOS_MASK,               /* Mask for the 6 bit TOS value */

    /**
     * *PORT_START is the starting port number,
     * *PORT_RANGE is the number of continuous ports from *PORT_START key
     */
    RIL_QOS_FILTER_TCP_SOURCE_PORT_START,
    RIL_QOS_FILTER_TCP_SOURCE_PORT_RANGE,
    RIL_QOS_FILTER_TCP_DESTINATION_PORT_START,
    RIL_QOS_FILTER_TCP_DESTINATION_PORT_RANGE,
    RIL_QOS_FILTER_UDP_SOURCE_PORT_START,
    RIL_QOS_FILTER_UDP_SOURCE_PORT_RANGE,
    RIL_QOS_FILTER_UDP_DESTINATION_PORT_START,
    RIL_QOS_FILTER_UDP_DESTINATION_PORT_RANGE,

    RIL_QOS_FILTER_IPV6_SOURCE_ADDR,        /* Format: xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx/yyy */
    RIL_QOS_FILTER_IPV6_DESTINATION_ADDR,   /* Format: xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx/yyy */
    RIL_QOS_FILTER_IPV6_TRAFFIC_CLASS,
    RIL_QOS_FILTER_IPV6_FLOW_LABEL
} RIL_QosSpecKeys;

#ifdef __cplusplus
}
#endif

#endif /*ANDROID_RIL_QOS_H*/
