/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * ISSUES:
 * - SMS retransmit (specifying TP-Message-ID)
 *
 */

/**
 * TODO
 *
 * Supp Service Notification (+CSSN)
 * GPRS PDP context deactivate notification
 *  
 */


#ifndef ANDROID_RIL_H 
#define ANDROID_RIL_H 1

#include <stdlib.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RIL_VERSION 2

typedef void * RIL_Token;

typedef enum {
    RIL_E_SUCCESS = 0,
    RIL_E_RADIO_NOT_AVAILABLE = 1,     /* If radio did not start or is resetting */
    RIL_E_GENERIC_FAILURE = 2,
    RIL_E_PASSWORD_INCORRECT = 3,      /* for PIN/PIN2 methods only! */
    RIL_E_SIM_PIN2 = 4,                /* Operation requires SIM PIN2 to be entered */
    RIL_E_SIM_PUK2 = 5,                /* Operation requires SIM PIN2 to be entered */
    RIL_E_REQUEST_NOT_SUPPORTED = 6,
    RIL_E_CANCELLED = 7,
    RIL_E_OP_NOT_ALLOWED_DURING_VOICE_CALL = 8, /* data ops are not allowed during voice
                                                   call on a Class C GPRS device */
    RIL_E_OP_NOT_ALLOWED_BEFORE_REG_TO_NW = 9,  /* data ops are not allowed before device
                                                   registers in network */
    RIL_E_SMS_SEND_FAIL_RETRY = 10		/* fail to send sms and need retry */	
} RIL_Errno;

typedef enum {
    RIL_CALL_ACTIVE = 0,
    RIL_CALL_HOLDING = 1,
    RIL_CALL_DIALING = 2,    /* MO call only */
    RIL_CALL_ALERTING = 3,   /* MO call only */
    RIL_CALL_INCOMING = 4,   /* MT call only */
    RIL_CALL_WAITING = 5     /* MT call only */
} RIL_CallState;

typedef enum {
    RADIO_STATE_OFF = 0,          /* Radio explictly powered off (eg CFUN=0) */
    RADIO_STATE_UNAVAILABLE = 1,  /* Radio unavailable (eg, resetting or not booted) */
    RADIO_STATE_SIM_NOT_READY = 2,      /* Radio is on, but the SIM interface is not ready */
    RADIO_STATE_SIM_LOCKED_OR_ABSENT = 3, /* SIM PIN locked, PUK required, network 
                               personalization locked, or SIM absent */
    RADIO_STATE_SIM_READY = 4           /* Radio is on and SIM interface is available */
} RIL_RadioState;

typedef struct {
    RIL_CallState   state;
    int             index;      /* GSM Index for use with, eg, AT+CHLD */
    int             toa;        /* type of address, eg 145 = intl */
    char            isMpty;     /* nonzero if is mpty call */
    char            isMT;       /* nonzero if call is mobile terminated */
    char            als;        /* ALS line indicator if available 
                                   (0 = line 1) */
    char            isVoice;    /* nonzero if this is is a voice call */

    char *          number;     /* phone number */
} RIL_Call;

typedef struct {
    int             cid;        /* Context ID */
    int             active;     /* nonzero if context is active */
    char *          type;       /* X.25, IP, IPV6, etc. */
    char *          apn;
    char *          address;
} RIL_PDP_Context_Response;

typedef struct {
    int messageRef;   /*TP-Message-Reference*/
    char *ackPDU;     /* or NULL if n/a */
} RIL_SMS_Response;

/** Used by RIL_REQUEST_WRITE_SMS_TO_SIM */
typedef struct {
    int status;     /* Status of message.  See TS 27.005 3.1, "<stat>": */
                    /*      0 = "REC UNREAD"    */
                    /*      1 = "REC READ"      */
                    /*      2 = "STO UNSENT"    */
                    /*      3 = "STO SENT"      */
    char * pdu;     /* PDU of message to write, as a hex string. */
    char * smsc;    /* SMSC address in GSM BCD format prefixed by a length byte
                       (as expected by TS 27.005) or NULL for default SMSC */
} RIL_SMS_WriteArgs;

/** Used by RIL_REQUEST_DIAL */
typedef struct {
    char * address;
    int clir;
            /* (same as 'n' paremeter in TS 27.007 7.7 "+CLIR"
             * clir == 0 on "use subscription default value"
             * clir == 1 on "CLIR invocation" (restrict CLI presentation)
             * clir == 2 on "CLIR suppression" (allow CLI presentation)
             */

} RIL_Dial;

typedef struct {
    int command;    /* one of the commands listed for TS 27.007 +CRSM*/
    int fileid;     /* EF id */
    char *path;     /* "pathid" from TS 27.007 +CRSM command.
                       Path is in hex asciii format eg "7f205f70"
                     */
    int p1;
    int p2;
    int p3;
    char *data;     /* May be NULL*/
    char *pin2;     /* May be NULL*/
} RIL_SIM_IO;

typedef struct {
    int sw1;
    int sw2;
    char *simResponse;  /* In hex string format ([a-fA-F0-9]*). */
} RIL_SIM_IO_Response;

/* See also com.android.internal.telephony.gsm.CallForwardInfo */

typedef struct {
    int             status;     /*
                                 * For RIL_REQUEST_QUERY_CALL_FORWARD_STATUS
                                 * status 1 = active, 0 = not active
                                 *
                                 * For RIL_REQUEST_SET_CALL_FORWARD:
                                 * status is:
                                 * 0 = disable
                                 * 1 = enable
                                 * 2 = interrogate
                                 * 3 = registeration
                                 * 4 = erasure
                                 */

    int             reason;     /* from TS 27.007 7.11 "reason" */
    int             serviceClass;/* From 27.007 +CCFC/+CLCK "class"
                                    See table for Android mapping from
                                    MMI service code 
				    0 means user doesn't input class */
    int             toa;        /* "type" from TS 27.007 7.11 */
    char *          number;     /* "number" from TS 27.007 7.11. May be NULL */
    int             timeSeconds; /* for CF no reply only */
}RIL_CallForwardInfo;

typedef struct {
   char * cid;         /* Cell Id (as described in TS 27.005) in 16 bits in GSM
                        * Primary Scrambling Code (as described in TS 25.331)
                        *         in 9 bits in UMTS 
                        * Valid values are hexadecimal 0x0000 - 0xffff.
                        */
   int    rssi;        /* Received RSSI in 2G,
                        * Level index of CPICH Received Signal Code Power in 3G
                        */
} RIL_NeighboringCell;

/* See RIL_REQUEST_LAST_CALL_FAIL_CAUSE */
typedef enum {
    CALL_FAIL_NORMAL = 16,
    CALL_FAIL_BUSY = 17,
    CALL_FAIL_CONGESTION = 34,
    CALL_FAIL_ACM_LIMIT_EXCEEDED = 68,
    CALL_FAIL_CALL_BARRED = 240,
    CALL_FAIL_FDN_BLOCKED = 241,
    CALL_FAIL_ERROR_UNSPECIFIED = 0xffff
} RIL_LastCallFailCause;

/* See RIL_REQUEST_LAST_PDP_FAIL_CAUSE */
typedef enum {
    PDP_FAIL_BARRED = 8,         /* no retry; prompt user */
    PDP_FAIL_BAD_APN = 27,       /* no retry; prompt user */
    PDP_FAIL_USER_AUTHENTICATION = 29, /* no retry; prompt user */
    PDP_FAIL_SERVICE_OPTION_NOT_SUPPORTED = 32,  /*no retry; prompt user */
    PDP_FAIL_SERVICE_OPTION_NOT_SUBSCRIBED = 33, /*no retry; prompt user */
    PDP_FAIL_ERROR_UNSPECIFIED = 0xffff  /* This and all other cases: retry silently */
} RIL_LastPDPActivateFailCause;

/* Used by RIL_UNSOL_SUPP_SVC_NOTIFICATION */
typedef struct {
    int     notificationType;   /*
                                 * 0 = MO intermediate result code
                                 * 1 = MT unsolicited result code
                                 */
    int     code;               /* See 27.007 7.17
                                   "code1" for MO
                                   "code2" for MT. */
    int     index;              /* CUG index. See 27.007 7.17. */
    int     type;               /* "type" from 27.007 7.17 (MT only). */
    char *  number;             /* "number" from 27.007 7.17
                                   (MT only, may be NULL). */
} RIL_SuppSvcNotification;

/* see RIL_REQUEST_GET_SIM_STATUS */
#define RIL_SIM_ABSENT      		0
#define RIL_SIM_NOT_READY   		1
/* RIL_SIM_READY means that the radio state is RADIO_STATE_SIM_READY. 
 * This is more
 * than "+CPIN: READY". It also means the radio is ready for SIM I/O
 */
#define RIL_SIM_READY       		2
#define RIL_SIM_PIN         		3
#define RIL_SIM_PUK         		4
#define RIL_SIM_NETWORK_PERSONALIZATION 5

/* The result of a SIM refresh, returned in data[0] of RIL_UNSOL_SIM_REFRESH */
typedef enum {
    /* A file on SIM has been updated.  data[1] contains the EFID. */
    SIM_FILE_UPDATE = 0,
    /* SIM initialized.  All files should be re-read. */
    SIM_INIT = 1,
    /* SIM reset.  SIM power required, SIM may be locked and all files should be re-read. */
    SIM_RESET = 2
} RIL_SimRefreshResult;

/* No restriction at all including voice/SMS/USSD/SS/AV64 and packet data. */
#define RIL_RESTRICTED_STATE_NONE           0x00
/* Block emergency call due to restriction. But allow all normal voice/SMS/USSD/SS/AV64. */
#define RIL_RESTRICTED_STATE_CS_EMERGENCY   0x01
/* Block all normal voice/SMS/USSD/SS/AV64 due to restriction. Only Emergency call allowed. */
#define RIL_RESTRICTED_STATE_CS_NORMAL      0x02
/* Block all voice/SMS/USSD/SS/AV64	including emergency call due to restriction.*/
#define RIL_RESTRICTED_STATE_CS_ALL         0x04
/* Block packet data access due to restriction. */
#define RIL_RESTRICTED_STATE_PS_ALL         0x10

/** 
 * RIL_REQUEST_GET_SIM_STATUS
 *
 * Requests status of the SIM interface and the SIM card
 * 
 * "data" is NULL
 *
 * "response" must be an int * pointing to RIL_SIM_* constant 
 * This should always succeed (RIL_SUCCESS)
 *
 * If the radio is off or unavailable, return RIL_SIM_NOT_READY 
 *
 * Please note: RIL_SIM_READY means that the radio state 
 * is RADIO_STATE_SIM_READY.   This is more than "+CPIN: READY". 
 * It also means the radio is ready for SIM I/O
 *
 * Valid errors:
 *  Must never fail
 */
#define RIL_REQUEST_GET_SIM_STATUS 1

/**
 * RIL_REQUEST_ENTER_SIM_PIN
 *
 * Supplies SIM PIN. Only called if SIM status is RIL_SIM_PIN
 *
 * "data" is const char **
 * ((const char **)data)[0] is PIN value
 *
 * "response" must be NULL
 *
 * Valid errors:
 *  
 * SUCCESS 
 * RADIO_NOT_AVAILABLE (radio resetting)
 * GENERIC_FAILURE
 * PASSWORD_INCORRECT
 */

#define RIL_REQUEST_ENTER_SIM_PIN 2


/**
 * RIL_REQUEST_ENTER_SIM_PUK
 *
 * Supplies SIM PUK and new PIN. 
 *
 * "data" is const char **
 * ((const char **)data)[0] is PUK value
 * ((const char **)data)[1] is new PIN value
 *
 * "response" must be NULL
 *
 * Valid errors:
 *  
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 *  PASSWORD_INCORRECT
 *     (PUK is invalid)
 */

#define RIL_REQUEST_ENTER_SIM_PUK 3

/**
 * RIL_REQUEST_ENTER_SIM_PIN2
 *
 * Supplies SIM PIN2. Only called following operation where SIM_PIN2 was
 * returned as a a failure from a previous operation.
 *
 * "data" is const char **
 * ((const char **)data)[0] is PIN2 value
 *
 * "response" must be NULL
 *
 * Valid errors:
 *  
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 *  PASSWORD_INCORRECT
 */

#define RIL_REQUEST_ENTER_SIM_PIN2 4

/**
 * RIL_REQUEST_ENTER_SIM_PUK2
 *
 * Supplies SIM PUK2 and new PIN2. 
 *
 * "data" is const char **
 * ((const char **)data)[0] is PUK2 value
 * ((const char **)data)[1] is new PIN2 value
 *
 * "response" must be NULL
 *
 * Valid errors:
 *  
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 *  PASSWORD_INCORRECT
 *     (PUK2 is invalid)
 */

#define RIL_REQUEST_ENTER_SIM_PUK2 5

/**
 * RIL_REQUEST_CHANGE_SIM_PIN
 *
 * Supplies old SIM PIN and new PIN. 
 *
 * "data" is const char **
 * ((const char **)data)[0] is old PIN value
 * ((const char **)data)[1] is new PIN value
 *
 * "response" must be NULL
 *
 * Valid errors:
 *  
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 *  PASSWORD_INCORRECT
 *     (old PIN is invalid)
 *      
 */

#define RIL_REQUEST_CHANGE_SIM_PIN 6


/**
 * RIL_REQUEST_CHANGE_SIM_PIN2
 *
 * Supplies old SIM PIN2 and new PIN2. 
 *
 * "data" is const char **
 * ((const char **)data)[0] is old PIN2 value
 * ((const char **)data)[1] is new PIN2 value
 *
 * "response" must be NULL
 *
 * Valid errors:
 *  
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 *  PASSWORD_INCORRECT
 *     (old PIN2 is invalid)
 *      
 */

#define RIL_REQUEST_CHANGE_SIM_PIN2 7

/**
 * RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION
 *
 * Requests that network personlization be deactivated
 *
 * "data" is const char **
 * ((const char **)(data))[0]] is network depersonlization code
 *
 * "response" must be NULL
 *
 * Valid errors:
 *  
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 *  PASSWORD_INCORRECT
 *     (code is invalid)
 */

#define RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION 8

/**
 * RIL_REQUEST_GET_CURRENT_CALLS 
 *
 * Requests current call list
 *
 * "data" is NULL
 *
 * "response" must be a "const RIL_Call **"
 * 
 * Valid errors:
 *  
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 *      (request will be made again in a few hundred msec)
 */

#define RIL_REQUEST_GET_CURRENT_CALLS 9


/** 
 * RIL_REQUEST_DIAL
 *
 * Initiate voice call
 *
 * "data" is const RIL_Dial *
 * "response" is NULL
 *  
 * This method is never used for supplementary service codes
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_DIAL 10

/**
 * RIL_REQUEST_GET_IMSI
 *
 * Get the SIM IMSI
 *
 * Only valid when radio state is "RADIO_STATE_SIM_READY"
 *
 * "data" is NULL
 * "response" is a const char * containing the IMSI
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */

#define RIL_REQUEST_GET_IMSI 11

/**
 * RIL_REQUEST_HANGUP
 *
 * Hang up a specific line (like AT+CHLD=1x)
 *
 * "data" is an int * 
 * (int *)data)[0] contains GSM call index (value of 'x' in CHLD above)
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */

#define RIL_REQUEST_HANGUP 12

/**
 * RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND
 *
 * Hang up waiting or held (like AT+CHLD=0)
 *
 * "data" is NULL
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */

#define RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND 13

/**
 * RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND
 *
 * Hang up waiting or held (like AT+CHLD=1)
 *
 * "data" is NULL
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */

#define RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND 14

/**
 * RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE
 *
 * Switch waiting or holding call and active call (like AT+CHLD=2)
 *
 * State transitions should be is follows:
 *
 * If call 1 is waiting and call 2 is active, then if this re
 *
 *   BEFORE                               AFTER
 * Call 1   Call 2                 Call 1       Call 2
 * ACTIVE   HOLDING                HOLDING     ACTIVE
 * ACTIVE   WAITING                HOLDING     ACTIVE
 * HOLDING  WAITING                HOLDING     ACTIVE
 * ACTIVE   IDLE                   HOLDING     IDLE
 * IDLE     IDLE                   IDLE        IDLE
 *
 * "data" is NULL
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */

#define RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE 15
#define RIL_REQUEST_SWITCH_HOLDING_AND_ACTIVE 15

/**
 * RIL_REQUEST_CONFERENCE
 *
 * Conference holding and active (like AT+CHLD=3)

 * "data" is NULL
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_CONFERENCE 16

/**
 * RIL_REQUEST_UDUB
 *
 * Send UDUB (user determined used busy) to ringing or 
 * waiting call answer)(RIL_BasicRequest r);
 *
 * "data" is NULL
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_UDUB 17

/**
 * RIL_REQUEST_LAST_CALL_FAIL_CAUSE
 *
 * Requests the failure cause code for the most recently terminated call
 *
 * "data" is NULL
 * "response" is a "int *"
 * ((int *)response)[0] is an integer cause code defined in TS 24.008
 *   Annex H or close approximation
 *
 * The implementation should return CALL_FAIL_ERROR_UNSPECIFIED for blocked
 * MO calls by restricted state (See RIL_UNSOL_RESTRICTED_STATE_CHANGED)
 *
 * If the implementation does not have access to the exact cause codes,
 * then it should return one of the values listed in RIL_LastCallFailCause,
 * as the UI layer needs to distinguish these cases for tone generation or
 * error notification.
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 * See also: RIL_REQUEST_LAST_PDP_FAIL_CAUSE
 */
#define RIL_REQUEST_LAST_CALL_FAIL_CAUSE 18

/**
 * RIL_REQUEST_SIGNAL_STRENGTH
 *
 * Requests current signal strength and bit error rate
 *
 * Must succeed if radio is on.
 *
 * "data" is NULL
 * "response" is an "int *"
 * ((int *)response)[0] is received signal strength (0-31, 99)
 * ((int *)response)[1] is bit error rate (0-7, 99)
 *  as defined in TS 27.007 8.5
 *  Other values (eg -1) are not legal
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 */

#define RIL_REQUEST_SIGNAL_STRENGTH 19
/**
 * RIL_REQUEST_REGISTRATION_STATE
 *
 * Request current registration state
 *
 * "data" is NULL
 * "response" is a "char **"
 * ((const char **)response)[0] is registration state 0-5 from TS 27.007 7.2
 * ((const char **)response)[1] is LAC if registered or NULL if not
 * ((const char **)response)[2] is CID if registered or NULL if not
 *
 * LAC and CID are in hexadecimal format.
 * valid LAC are 0x0000 - 0xffff
 * valid CID are 0x00000000 - 0x0fffffff
 *     In GSM, CID is Cell ID (as described in TS 27.007) in 16 bits
 *     In UMTS, CID is UMTS Cell Identity (as described in TS 25.331) in 28 bits
 * 
 * Please note that registration state 4 ("unknown") is treated 
 * as "out of service" in the Android telephony system
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_REGISTRATION_STATE 20

/**
 * RIL_REQUEST_GPRS_REGISTRATION_STATE
 *
 * Request current GPRS registration state
 *
 * "data" is NULL
 * "response" is a "char **"
 * ((const char **)response)[0] is registration state 0-5 from TS 27.007 7.2
 * ((const char **)response)[1] is LAC if registered or NULL if not
 * ((const char **)response)[2] is CID if registered or NULL if not
 * ((const char **)response)[3] indicates the available radio technology, where:
 *      0 == unknown
 *      1 == GPRS only
 *      2 == EDGE
 *      3 == UMTS
 *
 * LAC and CID are in hexadecimal format.
 * valid LAC are 0x0000 - 0xffff
 * valid CID are 0x00000000 - 0x0fffffff
 * 
 * Please note that registration state 4 ("unknown") is treated 
 * as "out of service" in the Android telephony system
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_GPRS_REGISTRATION_STATE 21

/**
 * RIL_REQUEST_OPERATOR
 *
 * Request current operator ONS or EONS
 *
 * "data" is NULL
 * "response" is a "const char **"
 * ((const char **)response)[0] is long alpha ONS or EONS 
 *                                  or NULL if unregistered
 *
 * ((const char **)response)[1] is short alpha ONS or EONS 
 *                                  or NULL if unregistered
 * ((const char **)response)[2] is 5 or 6 digit numeric code (MCC + MNC)
 *                                  or NULL if unregistered
 *                                  
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_OPERATOR 22

/**
 * RIL_REQUEST_RADIO_POWER
 *
 * Toggle radio on and off (for "airplane" mode)
 * "data" is int *
 * ((int *)data)[0] is > 0 for "Radio On"
 * ((int *)data)[0] is == 0 for "Radio Off"
 *
 * "response" is NULL
 *
 * Turn radio on if "on" > 0
 * Turn radio off if "on" == 0
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_RADIO_POWER 23

/**
 * RIL_REQUEST_DTMF
 *
 * Send a DTMF tone
 *
 * If the implementation is currently playing a tone requested via
 * RIL_REQUEST_DTMF_START, that tone should be cancelled and the new tone
 * should be played instead
 *
 * "data" is a char *
 * ((char *)data)[0] is a single character with one of 12 values: 0-9,*,#
 * ((char *)data)[1] is a single character with one of 3 values:
 *    'S' -- tone should be played for a short time
 *    'L' -- tone should be played for a long time
 * "response" is NULL
 * 
 * FIXME should this block/mute microphone?
 * How does this interact with local DTMF feedback?
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 * See also: RIL_REQUEST_DTMF_STOP, RIL_REQUEST_DTMF_START
 *
 */
#define RIL_REQUEST_DTMF 24

/**
 * RIL_REQUEST_SEND_SMS
 * 
 * Send an SMS message
 *
 * "data" is const char **
 * ((const char **)data)[0] is SMSC address in GSM BCD format prefixed
 *      by a length byte (as expected by TS 27.005) or NULL for default SMSC
 * ((const char **)data)[1] is SMS in PDU format as an ASCII hex string
 *      less the SMSC address
 *      TP-Layer-Length is be "strlen(((const char **)data)[1])/2"
 *
 * "response" is a const RIL_SMS_Response *
 *
 * Based on the return error, caller decides to resend if sending sms
 * fails. SMS_SEND_FAIL_RETRY means retry (i.e. error cause is 332) 
 * and GENERIC_FAILURE means no retry (i.e. error cause is 500)
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  SMS_SEND_FAIL_RETRY
 *  GENERIC_FAILURE
 *
 * FIXME how do we specify TP-Message-Reference if we need to resend?
 */
#define RIL_REQUEST_SEND_SMS 25


/**
 * RIL_REQUEST_SEND_SMS_EXPECT_MORE
 * 
 * Send an SMS message. Identical to RIL_REQUEST_SEND_SMS,
 * except that more messages are expected to be sent soon. If possible,
 * keep SMS relay protocol link open (eg TS 27.005 AT+CMMS command)
 *
 * "data" is const char **
 * ((const char **)data)[0] is SMSC address in GSM BCD format prefixed
 *      by a length byte (as expected by TS 27.005) or NULL for default SMSC
 * ((const char **)data)[1] is SMS in PDU format as an ASCII hex string
 *      less the SMSC address
 *      TP-Layer-Length is be "strlen(((const char **)data)[1])/2"
 *
 * "response" is a const RIL_SMS_Response *
 *
 * Based on the return error, caller decides to resend if sending sms
 * fails. SMS_SEND_FAIL_RETRY means retry (i.e. error cause is 332) 
 * and GENERIC_FAILURE means no retry (i.e. error cause is 500)
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  SMS_SEND_FAIL_RETRY
 *  GENERIC_FAILURE
 *
 */
#define RIL_REQUEST_SEND_SMS_EXPECT_MORE 26


/**
 * RIL_REQUEST_SETUP_DEFAULT_PDP
 *
 * Configure and activate PDP context (CID 1) for default IP connection 
 *
 * Android Telephony layer will start up pppd process on specified
 * tty when this request responded to.
 *
 * "data" is a const char **
 * ((const char **)data)[0] is the APN to connect to
 * ((const char **)data)[1] is the username, or NULL
 * ((const char **)data)[2] is the password, or NULL
 *
 * "response" is a char **
 * ((char **)response)[0] indicating PDP CID, which is generated by RIL
 * ((char **)response)[1] indicating the network interface name
 * ((char **)response)[2] indicating the IP address for this interface
 *
 * FIXME may need way to configure QoS settings
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 * See also: RIL_REQUEST_DEACTIVATE_DEFAULT_PDP
 */

#define RIL_REQUEST_SETUP_DEFAULT_PDP 27



/**
 * RIL_REQUEST_SIM_IO
 *
 * Request SIM I/O operation.
 * This is similar to the TS 27.007 "restricted SIM" operation
 * where it assumes all of the EF selection will be done by the
 * callee.
 *
 * "data" is a const RIL_SIM_IO *
 * Please note that RIL_SIM_IO has a "PIN2" field which may be NULL,
 * or may specify a PIN2 for operations that require a PIN2 (eg
 * updating FDN records)
 *
 * "response" is a const RIL_SIM_IO_Response *
 *
 * Arguments and responses that are unused for certain
 * values of "command" should be ignored or set to NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *  SIM_PIN2
 *  SIM_PUK2
 */
#define RIL_REQUEST_SIM_IO 28

/**
 * RIL_REQUEST_SEND_USSD
 *
 * Send a USSD message
 *
 * If a USSD session already exists, the message should be sent in the
 * context of that session. Otherwise, a new session should be created.
 *
 * The network reply should be reported via RIL_UNSOL_ON_USSD
 *
 * Only one USSD session may exist at a time, and the session is assumed
 * to exist until:
 *   a) The android system invokes RIL_REQUEST_CANCEL_USSD
 *   b) The implementation sends a RIL_UNSOL_ON_USSD with a type code
 *      of "0" (USSD-Notify/no further action) or "2" (session terminated)
 *
 * "data" is a const char * containing the USSD request in UTF-8 format
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 * See also: RIL_REQUEST_CANCEL_USSD, RIL_UNSOL_ON_USSD
 */

#define RIL_REQUEST_SEND_USSD 29

/**
 * RIL_REQUEST_CANCEL_USSD
 * 
 * Cancel the current USSD session if one exists
 *
 * "data" is null
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE 
 */

#define RIL_REQUEST_CANCEL_USSD 30

/**  
 * RIL_REQUEST_GET_CLIR
 *
 * Gets current CLIR status
 * "data" is NULL
 * "response" is int *
 * ((int *)data)[0] is "n" parameter from TS 27.007 7.7
 * ((int *)data)[1] is "m" parameter from TS 27.007 7.7
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_GET_CLIR 31

/**
 * RIL_REQUEST_SET_CLIR
 *
 * "data" is int *
 * ((int *)data)[0] is "n" parameter from TS 27.007 7.7
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_SET_CLIR 32

/**
 * RIL_REQUEST_QUERY_CALL_FORWARD_STATUS
 *
 * "data" is const RIL_CallForwardInfo *
 *
 * "response" is const RIL_CallForwardInfo **
 * "response" points to an array of RIL_CallForwardInfo *'s, one for
 * each distinct registered phone number.
 *
 * For example, if data is forwarded to +18005551212 and voice is forwarded
 * to +18005559999, then two separate RIL_CallForwardInfo's should be returned
 * 
 * If, however, both data and voice are forwarded to +18005551212, then
 * a single RIL_CallForwardInfo can be returned with the service class
 * set to "data + voice = 3")
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_QUERY_CALL_FORWARD_STATUS 33


/**
 * RIL_REQUEST_SET_CALL_FORWARD
 *
 * Configure call forward rule
 *
 * "data" is const RIL_CallForwardInfo *
 * "response" is NULL
 *  
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_SET_CALL_FORWARD 34


/**
 * RIL_REQUEST_QUERY_CALL_WAITING
 *
 * Query current call waiting state
 *
 * "data" is const int *
 * ((const int *)data)[0] is the TS 27.007 service class to query.
 * "response" is a const int *
 * ((const int *)response)[0] is 0 for "disabled" and 1 for "enabled"
 *
 * If ((const int *)response)[0] is = 1, then ((const int *)response)[1]
 * must follow, with the TS 27.007 service class bit vector of services
 * for which call waiting is enabled.
 *
 * For example, if ((const int *)response)[0]  is 1 and 
 * ((const int *)response)[1] is 3, then call waiting is enabled for data
 * and voice and disabled for everything else
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_QUERY_CALL_WAITING 35


/**
 * RIL_REQUEST_SET_CALL_WAITING
 *
 * Configure current call waiting state
 *
 * "data" is const int *
 * ((const int *)data)[0] is 0 for "disabled" and 1 for "enabled"
 * ((const int *)data)[1] is the TS 27.007 service class bit vector of
 *                           services to modify
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_SET_CALL_WAITING 36

/**
 * RIL_REQUEST_SMS_ACKNOWLEDGE
 *
 * Acknowledge successful or failed receipt of SMS previously indicated
 * via RIL_UNSOL_RESPONSE_NEW_SMS 
 *
 * "data" is int *
 * ((int *)data)[0] is "1" on successful receipt 
 *                  (basically, AT+CNMA=1 from TS 27.005
 * ((int *)data)[0] is "0" on failed receipt 
 *                  (basically, AT+CNMA=2 from TS 27.005)
 *
 * "response" is NULL
 *
 * FIXME would like request that specified RP-ACK/RP-ERROR PDU
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_SMS_ACKNOWLEDGE  37

/**
 * RIL_REQUEST_GET_IMEI
 *
 * Get the device IMEI, including check digit
 *
 * Valid when RadioState is not RADIO_STATE_UNAVAILABLE
 *
 * "data" is NULL
 * "response" is a const char * containing the IMEI
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */

#define RIL_REQUEST_GET_IMEI 38

/**
 * RIL_REQUEST_GET_IMEISV
 *
 * Get the device IMEISV, which should be two decimal digits
 *
 * Valid when RadioState is not RADIO_STATE_UNAVAILABLE
 *
 * "data" is NULL
 * "response" is a const char * containing the IMEISV
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */

#define RIL_REQUEST_GET_IMEISV 39


/**
 * RIL_REQUEST_ANSWER
 *
 * Answer incoming call
 *
 * Will not be called for WAITING calls.
 * RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE will be used in this case
 * instead
 *
 * "data" is NULL
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */

#define RIL_REQUEST_ANSWER 40

/**
 * RIL_REQUEST_DEACTIVATE_DEFAULT_PDP
 *
 * Deactivate PDP context created by RIL_REQUEST_SETUP_DEFAULT_PDP
 *
 * "data" is const char **
 * ((char**)data)[0] indicating PDP CID
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 * See also: RIL_REQUEST_SETUP_DEFAULT_PDP
 */

#define RIL_REQUEST_DEACTIVATE_DEFAULT_PDP 41

/**
 * RIL_REQUEST_QUERY_FACILITY_LOCK
 *
 * Query the status of a facility lock state
 *
 * "data" is const char **
 * ((const char **)data)[0] is the facility string code from TS 27.007 7.4  
 *                      (eg "AO" for BAOC, "SC" for SIM lock)
 * ((const char **)data)[1] is the password, or "" if not required
 * ((const char **)data)[2] is the TS 27.007 service class bit vector of
 *                           services to query
 *
 * "response" is an int *
 * ((const int *)response) 0 is the TS 27.007 service class bit vector of
 *                           services for which the specified barring facility 
 *                           is active. "0" means "disabled for all"
 * 
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 */
#define RIL_REQUEST_QUERY_FACILITY_LOCK 42

/**
 * RIL_REQUEST_SET_FACILITY_LOCK
 *
 * Enable/disable one facility lock
 *
 * "data" is const char **
 *
 * ((const char **)data)[0] = facility string code from TS 27.007 7.4
 * (eg "AO" for BAOC)
 * ((const char **)data)[1] = "0" for "unlock" and "1" for "lock"
 * ((const char **)data)[2] = password
 * ((const char **)data)[3] = string representation of decimal TS 27.007
 *                            service class bit vector. Eg, the string
 *                            "1" means "set this facility for voice services"
 *
 * "response" is NULL 
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 */
#define RIL_REQUEST_SET_FACILITY_LOCK 43

/**
 * RIL_REQUEST_CHANGE_BARRING_PASSWORD
 *
 * Change call barring facility password
 *
 * "data" is const char **
 *
 * ((const char **)data)[0] = facility string code from TS 27.007 7.4
 * (eg "AO" for BAOC)
 * ((const char **)data)[1] = old password
 * ((const char **)data)[2] = new password
 *
 * "response" is NULL 
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 */
#define RIL_REQUEST_CHANGE_BARRING_PASSWORD 44

/**
 * RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE
 *
 * Query current network selectin mode
 *
 * "data" is NULL
 *
 * "response" is int *
 * ((const int *)response)[0] is
 *     0 for automatic selection
 *     1 for manual selection
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 */
#define RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE 45

/**
 * RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC
 *
 * Specify that the network should be selected automatically
 *
 * "data" is NULL
 * "response" is NULL
 *
 * This request must not respond until the new operator is selected 
 * and registered
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 */
#define RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC 46

/**
 * RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL
 *
 * Manually select a specified network.
 *
 * The radio baseband/RIL implementation is expected to fall back to 
 * automatic selection mode if the manually selected network should go
 * out of range in the future.
 *
 * "data" is const char * specifying MCCMNC of network to select (eg "310170")
 * "response" is NULL
 *
 * This request must not respond until the new operator is selected 
 * and registered
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 */
#define RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL 47

/**
 * RIL_REQUEST_QUERY_AVAILABLE_NETWORKS
 *
 * Scans for available networks
 *
 * "data" is NULL
 * "response" is const char ** that should be an array of n*4 strings, where
 *    n is the number of available networks
 * For each available network:
 *
 * ((const char **)response)[n+0] is long alpha ONS or EONS 
 * ((const char **)response)[n+1] is short alpha ONS or EONS 
 * ((const char **)response)[n+2] is 5 or 6 digit numeric code (MCC + MNC)
 * ((const char **)response)[n+3] is a string value of the status:
 *           "unknown"
 *           "available"
 *           "current"
 *           "forbidden"
 *
 * This request must not respond until the new operator is selected 
 * and registered
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 */
#define RIL_REQUEST_QUERY_AVAILABLE_NETWORKS 48

/**
 * RIL_REQUEST_DTMF_START
 *
 * Start playing a DTMF tone. Continue playing DTMF tone until 
 * RIL_REQUEST_DTMF_STOP is received 
 *
 * If a RIL_REQUEST_DTMF_START is received while a tone is currently playing,
 * it should cancel the previous tone and play the new one.
 * 
 * "data" is a char *
 * ((char *)data)[0] is a single character with one of 12 values: 0-9,*,#
 * "response" is NULL
 * 
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 * See also: RIL_REQUEST_DTMF, RIL_REQUEST_DTMF_STOP
 */
#define RIL_REQUEST_DTMF_START 49

/**
 * RIL_REQUEST_DTMF_STOP
 *
 * Stop playing a currently playing DTMF tone.
 * 
 * "data" is NULL
 * "response" is NULL
 * 
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 * See also: RIL_REQUEST_DTMF, RIL_REQUEST_DTMF_START
 */
#define RIL_REQUEST_DTMF_STOP 50

/**
 * RIL_REQUEST_BASEBAND_VERSION
 *
 * Return string value indicating baseband version, eg
 * response from AT+CGMR
 * 
 * "data" is NULL
 * "response" is const char * containing version string for log reporting
 * 
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 */
#define RIL_REQUEST_BASEBAND_VERSION 51

/**
 * RIL_REQUEST_SEPARATE_CONNECTION
 *
 * Separate a party from a multiparty call placing the multiparty call
 * (less the specified party) on hold and leaving the specified party 
 * as the only other member of the current (active) call
 *
 * Like AT+CHLD=2x
 *
 * See TS 22.084 1.3.8.2 (iii)
 * TS 22.030 6.5.5 "Entering "2X followed by send"
 * TS 27.007 "AT+CHLD=2x"
 * 
 * "data" is an int * 
 * (int *)data)[0] contains GSM call index (value of 'x' in CHLD above)
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_SEPARATE_CONNECTION 52


/**
 * RIL_REQUEST_SET_MUTE
 *
 * Turn on or off uplink (microphone) mute.
 *
 * Will only be sent while voice call is active.
 * Will always be reset to "disable mute" when a new voice call is initiated
 *
 * "data" is an int *
 * (int *)data)[0] is 1 for "enable mute" and 0 for "disable mute"
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */

#define RIL_REQUEST_SET_MUTE 53

/**
 * RIL_REQUEST_GET_MUTE
 *
 * Queries the current state of the uplink mute setting
 *
 * "data" is NULL
 * "response" is an int *
 * (int *)response)[0] is 1 for "mute enabled" and 0 for "mute disabled"
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */

#define RIL_REQUEST_GET_MUTE 54

/**
 * RIL_REQUEST_QUERY_CLIP
 *
 * Queries the status of the CLIP supplementary service
 *
 * (for MMI code "*#30#")
 *
 * "data" is NULL
 * "response" is an int *
 * (int *)response)[0] is 1 for "CLIP provisioned" 
 *                           and 0 for "CLIP not provisioned"
 *                           and 2 for "unknown, e.g. no network etc" 
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */

#define RIL_REQUEST_QUERY_CLIP 55

/**
 * RIL_REQUEST_LAST_PDP_FAIL_CAUSE
 * 
 * Requests the failure cause code for the most recently failed PDP 
 * context activate
 *
 * "data" is NULL
 *
 * "response" is a "int *"
 * ((int *)response)[0] is an integer cause code defined in TS 24.008
 *   section 6.1.3.1.3 or close approximation
 *
 * If the implementation does not have access to the exact cause codes,
 * then it should return one of the values listed in 
 * RIL_LastPDPActivateFailCause, as the UI layer needs to distinguish these 
 * cases for error notification
 * and potential retries.
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 * See also: RIL_REQUEST_LAST_CALL_FAIL_CAUSE
 *  
 */ 

#define RIL_REQUEST_LAST_PDP_FAIL_CAUSE 56

/**
 * RIL_REQUEST_PDP_CONTEXT_LIST
 *
 * Queries the status of PDP contexts, returning for each
 * its CID, whether or not it is active, and its PDP type,
 * APN, and PDP adddress.
 *
 * "data" is NULL
 * "response" is an array of RIL_PDP_Context_Response
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */

#define RIL_REQUEST_PDP_CONTEXT_LIST 57

/**
 * RIL_REQUEST_RESET_RADIO
 *
 * Request a radio reset. The RIL implementation may postpone
 * the reset until after this request is responded to if the baseband
 * is presently busy.
 *
 * "data" is NULL
 * "response" is NULL
 *
 * The reset action could be delayed for a while
 * in case baseband modem is just busy.
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */

#define RIL_REQUEST_RESET_RADIO 58

/**
 * RIL_REQUEST_OEM_HOOK_RAW
 *
 * This request reserved for OEM-specific uses. It passes raw byte arrays
 * back and forth.
 *
 * It can be invoked on the Java side from 
 * com.android.internal.telephony.Phone.invokeOemRilRequestRaw()
 *
 * "data" is a char * of bytes copied from the byte[] data argument in java
 * "response" is a char * of bytes that will returned via the
 * caller's "response" Message here: 
 * (byte[])(((AsyncResult)response.obj).result)
 *
 * An error response here will result in 
 * (((AsyncResult)response.obj).result) == null and 
 * (((AsyncResult)response.obj).exception) being an instance of
 * com.android.internal.telephony.gsm.CommandException
 *
 * Valid errors:
 *  All
 */

#define RIL_REQUEST_OEM_HOOK_RAW 59

/**
 * RIL_REQUEST_OEM_HOOK_STRINGS
 *
 * This request reserved for OEM-specific uses. It passes strings
 * back and forth.
 *
 * It can be invoked on the Java side from 
 * com.android.internal.telephony.Phone.invokeOemRilRequestStrings()
 *
 * "data" is a const char **, representing an array of null-terminated UTF-8
 * strings copied from the "String[] strings" argument to
 * invokeOemRilRequestStrings()
 *
 * "response" is a const char **, representing an array of null-terminated UTF-8
 * stings that will be returned via the caller's response message here:
 *
 * (String[])(((AsyncResult)response.obj).result)
 *
 * An error response here will result in 
 * (((AsyncResult)response.obj).result) == null and 
 * (((AsyncResult)response.obj).exception) being an instance of
 * com.android.internal.telephony.gsm.CommandException
 *
 * Valid errors:
 *  All
 */

#define RIL_REQUEST_OEM_HOOK_STRINGS 60

/**
 * RIL_REQUEST_SCREEN_STATE
 *
 * Indicates the current state of the screen.  When the screen is off, the
 * RIL should notify the baseband to suppress certain notifications (eg,
 * signal strength and changes in LAC or CID) in an effort to conserve power.
 * These notifications should resume when the screen is on.
 *
 * "data" is int *
 * ((int *)data)[0] is == 1 for "Screen On"
 * ((int *)data)[0] is == 0 for "Screen Off"
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_SCREEN_STATE 61


/**
 * RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION
 *
 * Enables/disables supplementary service related notifications
 * from the network.
 *
 * Notifications are reported via RIL_UNSOL_SUPP_SVC_NOTIFICATION.
 *
 * "data" is int *
 * ((int *)data)[0] is == 1 for notifications enabled
 * ((int *)data)[0] is == 0 for notifications disabled
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 * See also: RIL_UNSOL_SUPP_SVC_NOTIFICATION.
 */
#define RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION 62

/**
 * RIL_REQUEST_WRITE_SMS_TO_SIM
 *
 * Stores a SMS message to SIM memory.
 *
 * "data" is RIL_SMS_WriteArgs *
 *
 * "response" is int *
 * ((const int *)response)[0] is the record index where the message is stored.
 *
 * Valid errors:
 *  SUCCESS
 *  GENERIC_FAILURE
 *
 */
#define RIL_REQUEST_WRITE_SMS_TO_SIM 63

/**
 * RIL_REQUEST_DELETE_SMS_ON_SIM
 *
 * Deletes a SMS message from SIM memory.
 *
 * "data" is int  *
 * ((int *)data)[0] is the record index of the message to delete.
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  GENERIC_FAILURE
 *
 */
#define RIL_REQUEST_DELETE_SMS_ON_SIM 64

/**
 * RIL_REQUEST_SET_BAND_MODE
 *
 * Assign a specified band for RF configuration.
 *
 * "data" is int *
 * ((int *)data)[0] is == 0 for "unspecified" (selected by baseband automatically)
 * ((int *)data)[0] is == 1 for "EURO band" (GSM-900 / DCS-1800 / WCDMA-IMT-2000)
 * ((int *)data)[0] is == 2 for "US band" (GSM-850 / PCS-1900 / WCDMA-850 / WCDMA-PCS-1900)
 * ((int *)data)[0] is == 3 for "JPN band" (WCDMA-800 / WCDMA-IMT-2000)
 * ((int *)data)[0] is == 4 for "AUS band" (GSM-900 / DCS-1800 / WCDMA-850 / WCDMA-IMT-2000)
 * ((int *)data)[0] is == 5 for "AUS band 2" (GSM-900 / DCS-1800 / WCDMA-850)
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_SET_BAND_MODE 65

/**
 * RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE
 *
 * Query the list of band mode supported by RF.
 *
 * "data" is NULL
 *
 * "response" is int *
 * "response" points to an array of int's, the int[0] is the size of array, reset is one for
 * each available band mode.
 *
 *  0 for "unspecified" (selected by baseband automatically)
 *  1 for "EURO band" (GSM-900 / DCS-1800 / WCDMA-IMT-2000)
 *  2 for "US band" (GSM-850 / PCS-1900 / WCDMA-850 / WCDMA-PCS-1900)
 *  3 for "JPN band" (WCDMA-800 / WCDMA-IMT-2000)
 *  4 for "AUS band" (GSM-900 / DCS-1800 / WCDMA-850 / WCDMA-IMT-2000)
 *  5 for "AUS band 2" (GSM-900 / DCS-1800 / WCDMA-850)
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 * See also: RIL_REQUEST_SET_BAND_MODE
 */
#define RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE 66

/**
 * RIL_REQUEST_STK_GET_PROFILE
 *
 * Requests the profile of SIM tool kit.
 * The profile indicates the SAT/USAT features supported by ME.
 * The SAT/USAT features refer to 3GPP TS 11.14 and 3GPP TS 31.111
 *
 * "data" is NULL
 *
 * "response" is a const char * containing SAT/USAT profile
 * in hexadecimal format string starting with first byte of terminal profile
 *
 * Valid errors:
 *  RIL_E_SUCCESS
 *  RIL_E_RADIO_NOT_AVAILABLE (radio resetting)
 *  RIL_E_GENERIC_FAILURE
 */
#define RIL_REQUEST_STK_GET_PROFILE 67

/**
 * RIL_REQUEST_STK_SET_PROFILE
 *
 * Download the STK terminal profile as part of SIM initialization
 * procedure
 *
 * "data" is a const char * containing SAT/USAT profile
 * in hexadecimal format string starting with first byte of terminal profile
 *
 * "response" is NULL
 *
 * Valid errors:
 *  RIL_E_SUCCESS
 *  RIL_E_RADIO_NOT_AVAILABLE (radio resetting)
 *  RIL_E_GENERIC_FAILURE
 */
#define RIL_REQUEST_STK_SET_PROFILE 68

/**
 * RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND
 *
 * Requests to send a SAT/USAT envelope command to SIM.
 * The SAT/USAT envelope command refers to 3GPP TS 11.14 and 3GPP TS 31.111
 *
 * "data" is a const char * containing SAT/USAT command
 * in hexadecimal format string starting with command tag
 *
 * "response" is a const char * containing SAT/USAT response
 * in hexadecimal format string starting with first byte of response
 * (May be NULL)
 *
 * Valid errors:
 *  RIL_E_SUCCESS
 *  RIL_E_RADIO_NOT_AVAILABLE (radio resetting)
 *  RIL_E_GENERIC_FAILURE
 */
#define RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND 69

/**
 * RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE
 *
 * Requests to send a terminal response to SIM for a received
 * proactive command
 *
 * "data" is a const char * containing SAT/USAT response
 * in hexadecimal format string starting with first byte of response data
 *
 * "response" is NULL
 *
 * Valid errors:
 *  RIL_E_SUCCESS
 *  RIL_E_RADIO_NOT_AVAILABLE (radio resetting)
 *  RIL_E_GENERIC_FAILURE
 */
#define RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE 70

/**
 * RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM
 *
 * When STK application gets RIL_UNSOL_STK_CALL_SETUP, the call actually has
 * been initialized by ME already. (We could see the call has been in the 'call
 * list') So, STK application needs to accept/reject the call according as user
 * operations.
 *
 * "data" is int *
 * ((int *)data)[0] is > 0 for "accept" the call setup
 * ((int *)data)[0] is == 0 for "reject" the call setup
 *
 * "response" is NULL
 *
 * Valid errors:
 *  RIL_E_SUCCESS
 *  RIL_E_RADIO_NOT_AVAILABLE (radio resetting)
 *  RIL_E_GENERIC_FAILURE
 */
#define RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM 71

/**
 * RIL_REQUEST_EXPLICIT_CALL_TRANSFER
 *
 * Connects the two calls and disconnects the subscriber from both calls.
 * 
 * "data" is NULL
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS 
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_EXPLICIT_CALL_TRANSFER 72

/**
 * RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE
 *
 * Requests to set the preferred network type for searching and registering
 * (CS/PS domain, RAT, and operation mode)
 *
 * "data" is int *
 * ((int *)data)[0] is == 0 for WCDMA preferred (auto mode)
 * ((int *)data)[0] is == 1 for GSM only
 * ((int *)data)[0] is == 2 for WCDMA only
 *
 * "response" is NULL
 *
 * Valid errors:
 *  RIL_E_SUCCESS
 *  RIL_E_RADIO_NOT_AVAILABLE (radio resetting)
 *  RIL_E_GENERIC_FAILURE
 */
#define RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE 73

/**
 * RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE
 *
 * Query the preferred network type (CS/PS domain, RAT, and operation mode)
 * for searching and registering
 *
 * "data" is NULL
 *
 * "response" is int *
 * ((int *)response)[0] is == 0 for WCDMA preferred (auto mode)
 * ((int *)response)[0] is == 1 for GSM only
 * ((int *)response)[0] is == 2 for WCDMA only
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 * See also: RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE
 */
#define RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE 74

/**
 * RIL_REQUEST_NEIGHBORING_CELL_IDS
 *
 * Request neighboring cell id in GSM network
 *
 * "data" is NULL
 * "response" must be a " const RIL_NeighboringCell** "
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
#define RIL_REQUEST_GET_NEIGHBORING_CELL_IDS 75

/**
 * RIL_REQUEST_SET_LOCATION_UPDATES
 *
 * Enables/disables network state change notifications due to changes in
 * LAC and/or CID (basically, +CREG=2 vs. +CREG=1).  
 *
 * Note:  The RIL implementation should default to "updates enabled"
 * when the screen is on and "updates disabled" when the screen is off.
 *
 * "data" is int *
 * ((int *)data)[0] is == 1 for updates enabled (+CREG=2)
 * ((int *)data)[0] is == 0 for updates disabled (+CREG=1)
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 * See also: RIL_REQUEST_SCREEN_STATE, RIL_UNSOL_RESPONSE_NETWORK_STATE_CHANGED
 */
#define RIL_REQUEST_SET_LOCATION_UPDATES 76

/***********************************************************************/

#define RIL_UNSOL_RESPONSE_BASE 1000

/**
 * RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED
 *
 * Indicate when value of RIL_RadioState has changed.
 *
 * Callee will invoke RIL_RadioStateRequest method on main thread
 *
 * "data" is NULL
 */

#define RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED 1000


/**
 * RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED
 *
 * Indicate when call state has changed
 *
 * Callee will invoke RIL_REQUEST_GET_CURRENT_CALLS on main thread
 *
 * "data" is NULL
 *
 * Response should be invoked on, for example, 
 * "RING", "BUSY", "NO CARRIER", and also call state
 * transitions (DIALING->ALERTING ALERTING->ACTIVE)
 *
 * Redundent or extraneous invocations are tolerated
 */
#define RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED 1001


/**
 * RIL_UNSOL_RESPONSE_NETWORK_STATE_CHANGED 
 *
 * Called when network state, operator name, or GPRS state has changed
 * Basically on, +CREG and +CGREG
 *
 * Callee will invoke the following requests on main thread:
 *
 * RIL_REQUEST_REGISTRATION_STATE
 * RIL_REQUEST_GPRS_REGISTRATION_STATE
 * RIL_REQUEST_OPERATOR
 *
 * "data" is NULL
 *
 * FIXME should this happen when SIM records are loaded? (eg, for
 * EONS)
 */
#define RIL_UNSOL_RESPONSE_NETWORK_STATE_CHANGED 1002

/**
 * RIL_UNSOL_RESPONSE_NEW_SMS
 *
 * Called when new SMS is received.
 * 
 * "data" is const char *
 * This is a pointer to a string containing the PDU of an SMS-DELIVER
 * as an ascii string of hex digits. The PDU starts with the SMSC address
 * per TS 27.005 (+CMT:)
 *
 * Callee will subsequently confirm the receipt of thei SMS with a
 * RIL_REQUEST_SMS_ACKNOWLEDGE
 *
 * No new RIL_UNSOL_RESPONSE_NEW_SMS 
 * or RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT messages should be sent until a
 * RIL_REQUEST_SMS_ACKNOWLEDGE has been received
 */

#define RIL_UNSOL_RESPONSE_NEW_SMS 1003

/**
 * RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT
 *
 * Called when new SMS Status Report is received.
 * 
 * "data" is const char *
 * This is a pointer to a string containing the PDU of an SMS-STATUS-REPORT
 * as an ascii string of hex digits. The PDU starts with the SMSC address
 * per TS 27.005 (+CDS:).
 *
 * Callee will subsequently confirm the receipt of the SMS with a
 * RIL_REQUEST_SMS_ACKNOWLEDGE
 *
 * No new RIL_UNSOL_RESPONSE_NEW_SMS 
 * or RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT messages should be sent until a
 * RIL_REQUEST_SMS_ACKNOWLEDGE has been received
 */

#define RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT 1004

/**
 * RIL_UNSOL_RESPONSE_NEW_SMS_ON_SIM
 *
 * Called when new SMS has been stored on SIM card
 * 
 * "data" is const int *
 * ((const int *)data)[0] contains the slot index on the SIM that contains
 * the new message
 */

#define RIL_UNSOL_RESPONSE_NEW_SMS_ON_SIM 1005

/**
 * RIL_UNSOL_ON_USSD
 *
 * Called when a new USSD message is received.
 *
 * "data" is const char **
 * ((const char **)data)[0] points to a type code, which is 
 *  one of these string values:
 *      "0"   USSD-Notify -- text in ((const char **)data)[1]
 *      "1"   USSD-Request -- text in ((const char **)data)[1]
 *      "2"   Session terminated by network
 *      "3"   other local client (eg, SIM Toolkit) has responded
 *      "4"   Operation not supported
 *      "5"   Network timeout
 *
 * The USSD session is assumed to persist if the type code is "1", otherwise
 * the current session (if any) is assumed to have terminated.
 *
 * ((const char **)data)[1] points to a message string if applicable, which
 * should always be in UTF-8.
 */
#define RIL_UNSOL_ON_USSD 1006
/* Previously #define RIL_UNSOL_ON_USSD_NOTIFY 1006   */

/**
 * RIL_UNSOL_ON_USSD_REQUEST
 *
 * Obsolete. Send via RIL_UNSOL_ON_USSD
 */
#define RIL_UNSOL_ON_USSD_REQUEST 1007 


/**
 * RIL_UNSOL_NITZ_TIME_RECEIVED
 *
 * Called when radio has received a NITZ time message
 *
 * "data" is const char * pointing to NITZ time string
 * in the form "yy/mm/dd,hh:mm:ss(+/-)tz,dt"
 */
#define RIL_UNSOL_NITZ_TIME_RECEIVED  1008

/**
 * RIL_UNSOL_SIGNAL_STRENGTH
 *
 * Radio may report signal strength rather han have it polled.
 *
 * "data" is an "int *"
 * ((int *)response)[0] is received signal strength (0-31, 99)
 * ((int *)response)[1] is bit error rate (0-7, 99)
 *  as defined in TS 27.007 8.5
 *  Other values (eg -1) are not legal
 */
#define RIL_UNSOL_SIGNAL_STRENGTH  1009


/**
 * RIL_UNSOL_PDP_CONTEXT_LIST_CHANGED
 *
 * Indicate a PDP context state has changed, or a new context
 * has been activated or deactivated
 *
 * "data" is an array of RIL_PDP_Context_Response identical to that
 * returned by RIL_REQUEST_PDP_CONTEXT_LIST
 *
 * See also: RIL_REQUEST_PDP_CONTEXT_LIST
 */

#define RIL_UNSOL_PDP_CONTEXT_LIST_CHANGED 1010

/**
 * RIL_UNSOL_SUPP_SVC_NOTIFICATION
 *
 * Reports supplementary service related notification from the network.
 *
 * "data" is a const RIL_SuppSvcNotification *
 *
 */

#define RIL_UNSOL_SUPP_SVC_NOTIFICATION 1011

/**
 * RIL_UNSOL_STK_SESSION_END
 *
 * Indicate when STK session is terminated by SIM.
 *
 * "data" is NULL
 */
#define RIL_UNSOL_STK_SESSION_END 1012

/**
 * RIL_UNSOL_STK_PROACTIVE_COMMAND
 *
 * Indicate when SIM issue a STK proactive command to applications
 *
 * "data" is a const char * containing SAT/USAT proactive command
 * in hexadecimal format string starting with command tag
 *
 */
#define RIL_UNSOL_STK_PROACTIVE_COMMAND 1013

/**
 * RIL_UNSOL_STK_EVENT_NOTIFY
 *
 * Indicate when SIM notifies applcations some event happens.
 * Generally, application does not need to have any feedback to
 * SIM but shall be able to indicate appropriate messages to users.
 *
 * "data" is a const char * containing SAT/USAT commands or responses
 * sent by ME to SIM or commands handled by ME, in hexadecimal format string
 * starting with first byte of response data or command tag
 *
 */
#define RIL_UNSOL_STK_EVENT_NOTIFY 1014

/**
 * RIL_UNSOL_STK_CALL_SETUP
 *
 * Indicate when SIM wants application to setup a voice call.
 *
 * "data" is const int *
 * ((const int *)data)[0] contains timeout value (in milliseconds)
 */
#define RIL_UNSOL_STK_CALL_SETUP 1015

/**
 * RIL_UNSOL_SIM_SMS_STORAGE_FULL
 *
 * Indicates that SMS storage on the SIM is full.  Sent when the network
 * attempts to deliver a new SMS message.  Messages cannot be saved on the
 * SIM until space is freed.  In particular, incoming Class 2 messages
 * cannot be stored.
 *
 * "data" is null
 *
 */
#define RIL_UNSOL_SIM_SMS_STORAGE_FULL 1016

/**
 * RIL_UNSOL_SIM_REFRESH
 *
 * Indicates that file(s) on the SIM have been updated, or the SIM
 * has been reinitialized.
 *
 * "data" is an int *
 * ((int *)data)[0] is a RIL_SimRefreshResult.
 * ((int *)data)[1] is the EFID of the updated file if the result is
 * SIM_FILE_UPDATE or NULL for any other result.
 *
 * Note: If the radio state changes as a result of the SIM refresh (eg,
 * SIM_READY -> SIM_LOCKED_OR_ABSENT), RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED
 * should be sent.
 */
#define RIL_UNSOL_SIM_REFRESH 1017

/**
 * RIL_UNSOL_CALL_RING
 *
 * Ring indication for an incoming call (eg, RING or CRING event).
 *
 * "data" is null
 */
#define RIL_UNSOL_CALL_RING 1018


/*
 * Import four CDMA notifications 1019 - 1022. Need fix when do real CDMA merge.
 */

/**
 * RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED
 *
 * Indicates that SIM state changes.
 *
 * Callee will invoke RIL_REQUEST_GET_SIM_STATUS on main thread
 * "data" is null
 */
#define RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED 1019

/**
 * RIL_UNSOL_RESPONSE_CDMA_NEW_SMS
 *
 * Called when new CDMA SMS is received
 *
 * "data" is const RIL_CDMA_SMS_Message *
 *
 * Callee will subsequently confirm the receipt of the SMS with
 * a RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE
 *
 * No new RIL_UNSOL_RESPONSE_CDMA_NEW_SMS should be sent until
 * RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE has been received
 */
#define RIL_UNSOL_RESPONSE_CDMA_NEW_SMS 1020

/**
 * RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS
 *
 * Called when new Broadcast SMS is received
 *
 * "data" is const char * of 88 bytes which indicates each page
 * of a CBS Message sent to the MS by the BTS as coded in 3GPP
 * 23.041 Section 9.4.1.1
 */
#define RIL_UNSOL_RESPONSE_NEW_BROADCAST_SMS 1021
	
/**
 * RIL_UNSOL_CDMA_RUIM_SMS_STORAGE_FULL
 *
 * Indicates that SMS storage on the RUIM is full.  Messages
 * cannot be saved on the RUIM until space is freed.
 *
 * "data" is null
 */
#define RIL_UNSOL_CDMA_RUIM_SMS_STORAGE_FULL 1022	
	
/**
 * RIL_UNSOL_RESTRICTED_STATE_CHANGED
 *
 * Indicates a restricted state change (eg, for Domain Specific Access Control).
 *
 * Radio need send this msg after radio off/on cycle no matter it is changed or not.
 *
 * "data" is an int *
 * ((int *)data)[0] contains a bitmask of RIL_RESTRICTED_STATE_* values.
 */
#define RIL_UNSOL_RESTRICTED_STATE_CHANGED 1023



/***********************************************************************/


/**
 * RIL_Request Function pointer
 *
 * @param request is one of RIL_REQUEST_*
 * @param data is pointer to data defined for that RIL_REQUEST_*
 *        data is owned by caller, and should not be modified or freed by callee
 * @param t should be used in subsequent call to RIL_onResponse
 * @param datalen the length of data
 *
 */
typedef void (*RIL_RequestFunc) (int request, void *data, 
                                    size_t datalen, RIL_Token t);

/**
 * This function should return the current radio state synchronously
 */
typedef RIL_RadioState (*RIL_RadioStateRequest)();

/**
 * This function returns "1" if the specified RIL_REQUEST code is
 * supported and 0 if it is not
 *
 * @param requestCode is one of RIL_REQUEST codes
 */

typedef int (*RIL_Supports)(int requestCode);

/**
 * This function is called from a separate thread--not the 
 * thread that calls RIL_RequestFunc--and indicates that a pending
 * request should be cancelled.
 * 
 * On cancel, the callee should do its best to abandon the request and
 * call RIL_onRequestComplete with RIL_Errno CANCELLED at some later point.
 *
 * Subsequent calls to  RIL_onRequestComplete for this request with
 * other results will be tolerated but ignored. (That is, it is valid
 * to ignore the cancellation request)
 *
 * RIL_Cancel calls should return immediately, and not wait for cancellation
 *
 * Please see ITU v.250 5.6.1 for how one might implement this on a TS 27.007 
 * interface
 *
 * @param t token wants to be canceled
 */

typedef void (*RIL_Cancel)(RIL_Token t);

typedef void (*RIL_TimedCallback) (void *param);

/**
 * Return a version string for your RIL implementation
 */
typedef const char * (*RIL_GetVersion) (void);

typedef struct {
    int version;        /* set to RIL_VERSION */
    RIL_RequestFunc onRequest;
    RIL_RadioStateRequest onStateRequest;
    RIL_Supports supports;
    RIL_Cancel onCancel;
    RIL_GetVersion getVersion;
} RIL_RadioFunctions;

#ifdef RIL_SHLIB
struct RIL_Env {
    /**
     * "t" is parameter passed in on previous call to RIL_Notification
     * routine.
     *
     * If "e" != SUCCESS, then response can be null/is ignored
     *
     * "response" is owned by caller, and should not be modified or 
     * freed by callee
     *
     * RIL_onRequestComplete will return as soon as possible
     */
    void (*OnRequestComplete)(RIL_Token t, RIL_Errno e, 
                           void *response, size_t responselen);

    /**
     * "unsolResponse" is one of RIL_UNSOL_RESPONSE_*
     * "data" is pointer to data defined for that RIL_UNSOL_RESPONSE_*
     *
     * "data" is owned by caller, and should not be modified or freed by callee
     */

    void (*OnUnsolicitedResponse)(int unsolResponse, const void *data, 
                                    size_t datalen);

    /**
     * Call user-specifed "callback" function on on the same thread that 
     * RIL_RequestFunc is called. If "relativeTime" is specified, then it specifies
     * a relative time value at which the callback is invoked. If relativeTime is
     * NULL or points to a 0-filled structure, the callback will be invoked as
     * soon as possible
     */

    void (*RequestTimedCallback) (RIL_TimedCallback callback, 
                                   void *param, const struct timeval *relativeTime);   
};


/** 
 *  RIL implementations must defined RIL_Init 
 *  argc and argv will be command line arguments intended for the RIL implementation
 *  Return NULL on error
 *
 * @param env is environment point defined as RIL_Env
 * @param argc number of arguments
 * @param argv list fo arguments
 *
 */
const RIL_RadioFunctions *RIL_Init(const struct RIL_Env *env, int argc, char **argv);

#else /* RIL_SHLIB */

/**
 * Call this once at startup to register notification routine
 *
 * @param callbacks user-specifed callback function
 */
void RIL_register (const RIL_RadioFunctions *callbacks);


/**
 *
 * RIL_onRequestComplete will return as soon as possible
 *
 * @param t is parameter passed in on previous call to RIL_Notification
 *          routine.
 * @param e error code
 *          if "e" != SUCCESS, then response can be null/is ignored
 * @param response is owned by caller, and should not be modified or
 *                 freed by callee
 * @param responselen the length of response in byte
 */
void RIL_onRequestComplete(RIL_Token t, RIL_Errno e, 
                           void *response, size_t responselen);

/**
 * @param unsolResponse is one of RIL_UNSOL_RESPONSE_*
 * @param data is pointer to data defined for that RIL_UNSOL_RESPONSE_*
 *     "data" is owned by caller, and should not be modified or freed by callee
 * @param datalen the length of data in byte
 */

void RIL_onUnsolicitedResponse(int unsolResponse, const void *data, 
                                size_t datalen);


/**
 * Call user-specifed "callback" function on on the same thread that 
 * RIL_RequestFunc is called. If "relativeTime" is specified, then it specifies
 * a relative time value at which the callback is invoked. If relativeTime is
 * NULL or points to a 0-filled structure, the callback will be invoked as
 * soon as possible
 *
 * @param callback user-specifed callback function
 * @param param parameter list
 * @param relativeTime a relative time value at which the callback is invoked
 */

void RIL_requestTimedCallback (RIL_TimedCallback callback, 
                               void *param, const struct timeval *relativeTime);


#endif /* RIL_SHLIB */

#ifdef __cplusplus
}
#endif

#endif /*ANDROID_RIL_H*/

