/*
 * Copyright (C) 2013 The Android Open Source Project
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
#ifndef ANDROID_IMS_H
#define ANDROID_IMS_H 1

#define IMS_VERSION 1     /* Current version */


typedef enum {
    IMS_CALLTYPE_VOICECALL = 1,
    IMS_CALLTYPE_VIDEOCALL
} IMS_CallType;

typedef enum {
    IMS_RATTYPE_LTE = 1,
    IMS_RATTYPE_WIFIONLY,
    IMS_RATTYPE_WIFIPREFERRED,
    IMS_RATTYPE_BESTEFFORT
} IMS_RATType;

typedef enum {
    IMS_CALLSTATE_ACTIVE = 1,
    IMS_CALLSTATE_HOLDING,
    IMS_CALLSTATE_DIALING,
    IMS_CALLSTATE_ALERTING,
    IMS_CALLSTATE_INCOMING,
    IMS_CALLSTATE_WAITING,
    IMS_CALLSTATE_PENDING  // SRVCC moved call to RIL. Pending hangup from IMS
} IMS_CallState;

typedef enum {
    IMS_REGISTERED = 1,
    IMS_NOTREGISTERED
} IMS_Registration;

typedef enum {
    IMS_CALL_FAIL_UNOBTAINABLE_NUMBER = 1,
    IMS_CALL_FAIL_NORMAL = 16,
    IMS_CALL_FAIL_BUSY = 17,
    /* TODO add a lot more fail causes */
    IMS_CALL_FAIL_ERROR_UNSPECIFIED = 0xffff
} IMS_LastCallFailCause;

typedef enum {
    TX = 1,
    RX,
    TX_RX
} IMS_CallDirection;

typedef struct {
    char *         impi;  /* Subscriber IMS Private Identity */
    char **        impu;  /* Subscriber IMS Public Identities */
    int            nbrOfImpu /* Size of impu */
    char *         displayName;  /* Subscriber Display Name */
} IMS_SubscriberInfo_v1;

typedef struct {
    char*          number; /* Number to dial */
    IMS_CallType   callType; /* Voicecall or videocall */
    IMS_RATType    ratType; /* LTE / WiFi / BestEffort */
    IMS_CallDirection direction /* RX, TX, TX_RX */
} IMS_Dial_v1;

typedef struct {
    char*          number; /* Number  */
    IMS_CallType   callType; /* Voicecall or videocall */
    IMS_RATType    ratType; /* LTE / WiFi / BestEffort */
    IMS_CallDirection direction /* RX, TX, TX_RX */
    IMS_CallState  callState; /* Dialing / Ongoing / Handover (for SRVCC) */
} IMS_CurrentCallsInfo_v1;

typedef struct {
    char*          sipUri; /* user adress */
    char*          mimeType;
    char*          message;
    char*          icsi; /* service identifier*/
    char*          iari; /* application identifier */
} IMS_Message;

typedef struct {
    IMS_Registration* state; /* REGISTERED, NOTREGISTERED */
    char*             services; /*NULL if NOTREGISTERED,
                                  else a comma seperated
                                  list of current services*/
} IMS_RegistrationState;

typedef struct {
/* TODO */
} IMS_SMS_Message

typedef struct {
    int messageRef;
    char *ackPDU;
    int errorCode;
} IMS_SMS_Response;

/**
 * IMS_REQUEST_GET_SUBSCRIBER_INFO
 *
 * Requests IMS subscriber information
 *
 * "data" is NULL
 *
 * "response" is const IMS_SubscriberInfo_v1 *
 *
 * Valid errors:
 */
#define IMS_REQUEST_GET_SUBSCRIBER_INFO 1

/**
 * IMS_REQUEST_DIAL
 *
 * Make a call
 *
 * "data" is IMS_Dial_v1
 *
 * "response" is NULL
 *
 * Valid errors:
 *
 */

#define IMS_REQUEST_DIAL 2

/**
 * IMS_REQUEST_GET_CURRENT_CALLS
 *
 * Fetches the current ongoing calls
 *
 * "data" is NULL
 *
 * "response" is "const IMS_CurrentCallsInfo_v1 **"
 *
 * Valid errors:
 *
 */

#define IMS_REQUEST_GET_CURRENT_CALLS 3

/**
 * IMS_REQUEST_HANGUP
 *
 * Hang up a specific call
 *
 * "data" is int*
 * ((int *)data)[0] contains Connection index
 *
 * "response" is NULL
 *
 * Valid errors:
 *
 */

#define IMS_REQUEST_HANGUP 4

/**
 * IMS_REQUEST_HANGUP_WAITING_OR_BACKGROUND
 *
 * Hangup a waiting or held call
 *
 * "data" is NULL
 *
 * "response" is NULL
 *
 * Valid errors:
 *
 */

#define IMS_REQUEST_HANGUP_WAITING_OR_BACKGROUND 5

/**
 * IMS_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND
 *
 * Hangup active call, resume waiting or held call
 *
 * "data" is NULL
 *
 * "response" is NULL
 *
 * Valid errors:
 *
 */

#define IMS_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND 6

/**
 * IMS_REQUEST_SWITCH_HOLDING_AND_ACTIVE
 *
 * Switch waiting/holding and active call
 *
 * "data" is NULL
 *
 * "response" is NULL
 *
 * Valid errors:
 *
 */

#define IMS_REQUEST_SWITCH_HOLDING_AND_ACTIVE 7

/**
 * IMS_REQUEST_CONFERENCE
 *
 * Conference holding and active
 *
 * "data" is NULL
 *
 * "response" is NULL
 *
 * Valid errors:
 *
 */

#define IMS_REQUEST_CONFERENCE 8

/**
 * IMS_REQUEST_UDUB
 *
 * Send UDUB (user determined user busy) to ringing or waiting call
 * answer)
 *
 * "data" is NULL
 *
 * "response" is NULL
 *
 * Valid errors:
 *
 */

#define IMS_REQUEST_UDUB 9

/**
 * IMS_REQUEST_LAST_CALL_FAIL_CAUSE
 *
 * Requests the failure cause code for the most recently terminated call
 *
 * "data" is NULL
 *
 * "response" is int *
 * ((int*)response)[0] is IMS_LastCallFailCause
 *
 * Valid errors:
 *
 */

#define IMS_REQUEST_LAST_CALL_FAIL_CAUSE 10

/**
 * IMS_REQUEST_IMS_REGISTRATION_STATE
 *
 * Request current registration state for IMS service
 *
 * "data" is NULL
 *
 * "response" is IMS_RegistrationState
 *
 * Valid errors:
 *
 */

#define IMS_REQUEST_IMS_REGISTRATION_STATE 11

/**
 * IMS_REQUEST_DTMF
 *
 * Send a DTMF tone
 *
 * "data" is a char * containing a single character with one of 12 values: 0-9,*,#
 *
 * "response" is NULL
 *
 * Valid errors:
 *
 */

#define IMS_REQUEST_DTMF 12

/**
 * IMS_REQUEST_SEND_USSD
 *
 * Send a USSD message
 *
 * "data" is char * containing the USSD request
 *
 * "response" is NULL
 *
 * Valid errors:
 *
 */

#define IMS_REQUEST_SEND_USSD 13

/**
 * IMS_REQUEST_CANCEL_USSD
 *
 * Cancel the current USSD session
 *
 * "data" is NULL
 *
 * "response" is NULL
 *
 * Valid errors:
 *
 */

#define IMS_REQUEST_CANCEL_USSD 14

/**
 * IMS_REQUEST_ANSWER
 *
 * Answer incoming call
 *
 * "data" is NULL
 *
 * "response" is NULL
 *
 * Valid errors:
 *
 */

#define IMS_REQUEST_ANSWER 15

/**
 * IMS_REQUEST_DTMF_START
 *
 * Start playing a DTMF tone
 *
 * "data" is a char *
 * Single character with one of 12 values: 0-9,*,#
 *
 * "response" is NULL
 *
 * Valid errors:
 *
 */

#define IMS_REQUEST_DTMF_START 16

/**
 * IMS_REQUEST_DTMF_STOP
 *
 * Stops playing a currently playing DTMF tone.
 *
 * "data" is IMS_Dial_v1
 *
 * "response" is NULL
 *
 * Valid errors:
 *
 */

#define IMS_REQUEST_DTMF_STOP 17

/**
 * IMS_REQUEST_SEND_SMS
 *
 * Send a SMS over IMS
 *
 * "data" is IMS_SMS_Message
 *
 * "response" is IMS_SMS_Response
 */
#define IMS_REQUEST_SEND_SMS 18

/**
 * IMS_REQUEST_SMS_ACKNOWLEDGE
 *
 * Acknowledge successful or failed receipt of SMS previously indicated
 * via IMS_UNSOL_RESPONSE_NEW_SMS
 *
 * "data" is int *
 *
 * "response" is NULL
 *
 *
 * Valid errors:
 */
#define IMS_REQUEST_SMS_ACKNOWLEDGE  19

/**
 * IMS_REQUEST_ASSIGN_ICCID
 *
 * Sets the expected ICCID that this socket is always going to refer to.
 * First call that must be made on a new socket.
 * This should only be needed in case of multi SIM and by extension, multiple ims stacks
 *
 * "data" is char* containing IccID
 *
 * Valid errors:
 *
 *  SUCCESS
 *  INVALID_CARD
 */
#define IMS_REQUEST_ASSIGN_ICCID 20

/**
 *
 * IMS_REGISTER_SERVICE
 *
 * Trigger the core to do SIP registration with
 * selected services as supplied. It is expect that this
 * will later lead to an IMS_UNSOL_RESPONSE_IMS_REGISTRATION_STATE_CHANGED
 *
 * "data" is a char* of a comma seperated list of ICSIs
 * Example: VoLTE, ViLTE and RCS services could be in this list
 *
 * "response" is NULL
 *
 */
#define IMS_REGISTER_SERVICE 21

/**
 *
 * Get the capabilities of a specific user
 *
 * "data" is a char* containing the SIP Address of the user
 *
 * "response" is a char* containing a comma seperated list of supported services
 *
 */
#define IMS_GET_OPTIONS 22

/**
 * Send a specific message to a user
 *
 * "data" is a IMS_Message*
 *
 * "response" is a char *. null in case the status is OK.
 *    in case of fail, the string contains the reason.
 */
#define IMS_MESSAGE 23

/**
 * Invite a user to a conversation
 *
 * "data" is TODO
 *
 * "response is TODO
 */
#define IMS_INVITE 24

/**
 * IMS_UNSOL_RESPONSE_IMS_REGISTRATION_STATE_CHANGED
 *
 * IMS Registration state has changed
 *
 * "data" is NULL
 *
 * Valid errors:
 *
 */
#define IMS_UNSOL_RESPONSE_IMS_REGISTRATION_STATE_CHANGED 201

/**
 * IMS_UNSOL_RESPONSE_CALL_STATE_CHANGED
 *
 * Call state has changed
 *
 * "data" is NULL
 *
 * Valid errors:
 *
 */
#define IMS_UNSOL_RESPONSE_CALL_STATE_CHANGED 202

/**
 * IMS_UNSOL_ON_USSD
 *
 * New USSD message is received
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
 *
 * Valid errors:
 *
 */

#define IMS_UNSOL_ON_USSD 203

/**
 * IMS_UNSOL_CALL_RING
 *
 * Ring indication of incoming call
 *
 * "data" is NULL
 *
 * Valid errors:
 *
 */

#define IMS_UNSOL_CALL_RING 204

/**
 * RIL_UNSOL_RESPONSE_NEW_SMS
 *
 * Called when new SMS is received.
 *
 */
#define IMS_UNSOL_RESPONSE_NEW_SMS 205

/* TODO
 * 1. Presence
 * 2. complete Generic SIP API
 */

#endif /*ANDROID_IMS_H*/
