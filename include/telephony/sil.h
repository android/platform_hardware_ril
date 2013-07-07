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
#ifndef ANDROID_SIL_H
#define ANDROID_SIL_H 1

#define SIL_VERSION 1     /* Current version */

#define SIL_MAX_CARD_APPS         8
#define SIL_MAX_PHYSICAL_CARDS    4


typedef enum {
    SIL_PERSOSUBSTATE_UNKNOWN                   = 0, /* initial state */
    SIL_PERSOSUBSTATE_IN_PROGRESS               = 1, /* in between each lock transition */
    SIL_PERSOSUBSTATE_READY                     = 2, /* when either SIM or RUIM Perso is finished
                                                        since each app can only have 1 active perso
                                                        involved */
    SIL_PERSOSUBSTATE_SIM_NETWORK               = 3,
    SIL_PERSOSUBSTATE_SIM_NETWORK_SUBSET        = 4,
    SIL_PERSOSUBSTATE_SIM_CORPORATE             = 5,
    SIL_PERSOSUBSTATE_SIM_SERVICE_PROVIDER      = 6,
    SIL_PERSOSUBSTATE_SIM_SIM                   = 7,
    SIL_PERSOSUBSTATE_SIM_NETWORK_PUK           = 8, /* The corresponding perso lock is blocked */
    SIL_PERSOSUBSTATE_SIM_NETWORK_SUBSET_PUK    = 9,
    SIL_PERSOSUBSTATE_SIM_CORPORATE_PUK         = 10,
    SIL_PERSOSUBSTATE_SIM_SERVICE_PROVIDER_PUK  = 11,
    SIL_PERSOSUBSTATE_SIM_SIM_PUK               = 12,
    SIL_PERSOSUBSTATE_RUIM_NETWORK1             = 13,
    SIL_PERSOSUBSTATE_RUIM_NETWORK2             = 14,
    SIL_PERSOSUBSTATE_RUIM_HRPD                 = 15,
    SIL_PERSOSUBSTATE_RUIM_CORPORATE            = 16,
    SIL_PERSOSUBSTATE_RUIM_SERVICE_PROVIDER     = 17,
    SIL_PERSOSUBSTATE_RUIM_RUIM                 = 18,
    SIL_PERSOSUBSTATE_RUIM_NETWORK1_PUK         = 19, /* The corresponding perso lock is blocked */
    SIL_PERSOSUBSTATE_RUIM_NETWORK2_PUK         = 20,
    SIL_PERSOSUBSTATE_RUIM_HRPD_PUK             = 21,
    SIL_PERSOSUBSTATE_RUIM_CORPORATE_PUK        = 22,
    SIL_PERSOSUBSTATE_RUIM_SERVICE_PROVIDER_PUK = 23,
    SIL_PERSOSUBSTATE_RUIM_RUIM_PUK             = 24
} SIL_PersoSubstate;

typedef enum {
    SIL_APPSTATE_UNKNOWN               = 0,
    SIL_APPSTATE_DETECTED              = 1,
    SIL_APPSTATE_PIN                   = 2, /* If PIN1 or UPin is required */
    SIL_APPSTATE_PUK                   = 3, /* If PUK1 or Puk for UPin is required */
    SIL_APPSTATE_SUBSCRIPTION_PERSO    = 4, /* perso_substate should be look at
                                               when app_state is assigned to this value */
    SIL_APPSTATE_READY                 = 5
} SIL_AppState;

typedef enum {
    SIL_PINSTATE_UNKNOWN              = 0,
    SIL_PINSTATE_ENABLED_NOT_VERIFIED = 1,
    SIL_PINSTATE_ENABLED_VERIFIED     = 2,
    SIL_PINSTATE_DISABLED             = 3,
    SIL_PINSTATE_ENABLED_BLOCKED      = 4,
    SIL_PINSTATE_ENABLED_PERM_BLOCKED = 5
} SIL_PinState;

typedef enum {
  SIL_APPTYPE_UNKNOWN = 0,
  SIL_APPTYPE_SIM     = 1,
  SIL_APPTYPE_USIM    = 2,
  SIL_APPTYPE_RUIM    = 3,
  SIL_APPTYPE_CSIM    = 4,
  SIL_APPTYPE_ISIM    = 5
} SIL_AppType;

typedef struct {
    char *iccid; // IccID of this card. IMSI in case of no IccId
} SIL_Card;

typedef struct {
    SIL_Card *card; //Card that this sim IO request should go to
    int command;    /* one of the commands listed for TS 27.007 +CRSM*/
    int fileid;     /* EF id */
    char *path;     /* "pathid" from TS 27.007 +CRSM command.
                       Path is in hex asciii format eg "7f205f70"
                       Path must always be provided.
                     */
    int p1;
    int p2;
    int p3;
    char *data;     /* May be NULL*/
    char *pin2;     /* May be NULL*/
    char *aidPtr;   /* AID value, See ETSI 102.221 8.1 and 101.220 4, NULL if no value. */
} SIL_SIM_IO_v7;

typedef struct {
    int sw1;
    int sw2;
    char *simResponse;  /* In hex string format ([a-fA-F0-9]*). */
} SIL_SIM_IO_Response;

typedef enum {
    SIL_CARDSTATE_ABSENT   = 0,
    SIL_CARDSTATE_PRESENT  = 1,
    SIL_CARDSTATE_ERROR    = 2
} SIL_CardState;

typedef struct
{
  SIL_AppType      app_type;
  SIL_AppState     app_state;
  SIL_PersoSubstate perso_substate; /* applicable only if app_state ==
                                       SIL_APPSTATE_SUBSCRIPTION_PERSO */
  char             *aid_ptr;        /* null terminated string, e.g., from 0xA0, 0x00 -> 0x41,
                                       0x30, 0x30, 0x30 */
  char             *app_label_ptr;  /* null terminated string */
  int              pin1_replaced;   /* applicable to USIM, CSIM & ISIM */
  SIL_PinState     pin1;
  SIL_PinState     pin2;
} SIL_AppStatus;

typedef struct
{
  SIL_CardState card_state;
  SIL_PinState  universal_pin_state;             /* applicable to USIM and CSIM: SIL_PINSTATE_xxx */
  int           gsm_umts_subscription_app_index; /* value < SIL_MAX_CARD_APPS, -1 if none */
  int           cdma_subscription_app_index;     /* value < SIL_MAX_CARD_APPS, -1 if none */
  int           ims_subscription_app_index;      /* value < SIL_MAX_CARD_APPS, -1 if none */
  int           num_applications;                /* value <= SIL_MAX_CARD_APPS */
  SIL_AppStatus applications[SIL_MAX_CARD_APPS];
} SIL_CardStatus_v6;

/** Used by SIL_REQUEST_WRITE_SMS_TO_SIM */
typedef struct {
    SIL_Card *card; /* IccID */
    int status;     /* Status of message.  See TS 27.005 3.1, "<stat>": */
                    /*      0 = "REC UNREAD"    */
                    /*      1 = "REC READ"      */
                    /*      2 = "STO UNSENT"    */
                    /*      3 = "STO SENT"      */
    char * pdu;     /* PDU of message to write, as an ASCII hex string less the SMSC address,
                       the TP-layer length is "strlen(pdu)/2". */
    char * smsc;    /* SMSC address in GSM BCD format prefixed by a length byte
                       (as expected by TS 27.005) or NULL for default SMSC */
} SIL_SMS_WriteArgs;

/** The result of a SIM refresh, returned in data[0] of SIL_UNSOL_SIM_REFRESH
 *      or as part of SIL_SimRefreshResponse_v7
 */
typedef enum {
    /* A file on SIM has been updated.  data[1] contains the EFID. */
    SIM_FILE_UPDATE = 0,
    /* SIM initialized.  All files should be re-read. */
    SIM_INIT = 1,
    /* SIM reset.  SIM power required, SIM may be locked and all files should be re-read. */
    SIM_RESET = 2
} SIL_SimRefreshResult;

typedef struct {
    SIL_SimRefreshResult result;
    int                  ef_id; /* is the EFID of the updated file if the result is */
                                /* SIM_FILE_UPDATE or 0 for any other result. */
    char *               aid;   /* is AID(application ID) of the card application */
                                /* See ETSI 102.221 8.1 and 101.220 4 */
                                /*     For SIM_FILE_UPDATE result it can be set to AID of */
                                /*         application in which updated EF resides or it can be */
                                /*         NULL if EF is outside of an application. */
                                /*     For SIM_INIT result this field is set to AID of */
                                /*         application that caused REFRESH */
                                /*     For SIM_RESET result it is NULL. */
} SIL_SimRefreshResponse_v7;

typedef struct {
    SIL_Card             iccid;
    SIL_SimRefreshResult result;
    int                  ef_id; /* is the EFID of the updated file if the result is */
                                /* SIM_FILE_UPDATE or 0 for any other result. */
    char *               aid;   /* is AID(application ID) of the card application */
                                /* See ETSI 102.221 8.1 and 101.220 4 */
                                /*     For SIM_FILE_UPDATE result it can be set to AID of */
                                /*         application in which updated EF resides or it can be */
                                /*         NULL if EF is outside of an application. */
                                /*     For SIM_INIT result this field is set to AID of */
                                /*         application that caused REFRESH */
                                /*     For SIM_RESET result it is NULL. */
} SIL_SimRefreshResponse_v8;

/**
 * RIL_REQUEST_GET_CARDS
 *
 * Get the IccIDs of all present cards, or IMSI in the case of no IccID
 *
 * "response" is a SIL_Card[SIL_MAX_PHYSICAL_CARDS] containing the IccIDs of all
 * present cards. NULL is allowed on cards not present.
 *
 * Valid errors:
 *  Must never fail
 */
#define SIL_REQUEST_GET_ICCIDS 1

/**
 * SIL_REQUEST_GET_SIM_STATUS
 *
 * Requests status of the SIM interface and the SIM card
 *
 * "data" is SIL_Card*
 *
 * "response" is const SIL_CardStatus_v6 *
 *
 * Valid errors:
 *  Must never fail
 */
#define SIL_REQUEST_GET_SIM_STATUS 2

/**
 * SIL_REQUEST_ENTER_SIM_PIN
 *
 * Supplies SIM PIN. Only called if SIL_CardStatus has SIL_APPSTATE_PIN state
 *
 * "data" is const char **
 * ((const char **)data)[0] is PIN value
 * ((const char **)data)[1] is AID value, See ETSI 102.221 8.1 and 101.220 4, NULL if no value.
 * ((const char **)data)[2] is SIL_Card * or null.
 *
 * "response" is int *
 * ((int *)response)[0] is the number of retries remaining, or -1 if unknown
 *
 * Valid errors:
 *
 * SUCCESS
 * RADIO_NOT_AVAILABLE (radio resetting)
 * GENERIC_FAILURE
 * PASSWORD_INCORRECT
 */

#define SIL_REQUEST_ENTER_SIM_PIN 3


/**
 * SIL_REQUEST_ENTER_SIM_PUK
 *
 * Supplies SIM PUK and new PIN.
 *
 * "data" is const char **
 * ((const char **)data)[0] is PUK value
 * ((const char **)data)[1] is new PIN value
 * ((const char **)data)[2] is AID value, See ETSI 102.221 8.1 and 101.220 4, NULL if no value.
 * ((const char **)data)[3] is SIL_Card * or null.
 *
 * "response" is int *
 * ((int *)response)[0] is the number of retries remaining, or -1 if unknown
 *
 * Valid errors:
 *
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 *  PASSWORD_INCORRECT
 *     (PUK is invalid)
 */

#define SIL_REQUEST_ENTER_SIM_PUK 4

/**
 * SIL_REQUEST_ENTER_SIM_PIN2
 *
 * Supplies SIM PIN2. Only called following operation where SIM_PIN2 was
 * returned as a a failure from a previous operation.
 *
 * "data" is const char **
 * ((const char **)data)[0] is PIN2 value
 * ((const char **)data)[1] is AID value, See ETSI 102.221 8.1 and 101.220 4, NULL if no value.
 * ((const char **)data)[2] is SIL_Card * or null.
 *
 * "response" is int *
 * ((int *)response)[0] is the number of retries remaining, or -1 if unknown
 *
 * Valid errors:
 *
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 *  PASSWORD_INCORRECT
 */

#define SIL_REQUEST_ENTER_SIM_PIN2 5

/**
 * SIL_REQUEST_ENTER_SIM_PUK2
 *
 * Supplies SIM PUK2 and new PIN2.
 *
 * "data" is const char **
 * ((const char **)data)[0] is PUK2 value
 * ((const char **)data)[1] is new PIN2 value
 * ((const char **)data)[2] is AID value, See ETSI 102.221 8.1 and 101.220 4, NULL if no value.
 * ((const char **)data)[3] is SIL_Card * or null.
 *
 * "response" is int *
 * ((int *)response)[0] is the number of retries remaining, or -1 if unknown
 *
 * Valid errors:
 *
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 *  PASSWORD_INCORRECT
 *     (PUK2 is invalid)
 */

#define SIL_REQUEST_ENTER_SIM_PUK2 6

/**
 * SIL_REQUEST_CHANGE_SIM_PIN
 *
 * Supplies old SIM PIN and new PIN.
 *
 * "data" is const char **
 * ((const char **)data)[0] is old PIN value
 * ((const char **)data)[1] is new PIN value
 * ((const char **)data)[2] is AID value, See ETSI 102.221 8.1 and 101.220 4, NULL if no value.
 * ((const char **)data)[2] is SIL_Card * or null.
 *
 * "response" is int *
 * ((int *)response)[0] is the number of retries remaining, or -1 if unknown
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
#define SIL_REQUEST_CHANGE_SIM_PIN 7


/**
 * SIL_REQUEST_CHANGE_SIM_PIN2
 *
 * Supplies old SIM PIN2 and new PIN2.
 *
 * "data" is const char **
 * ((const char **)data)[0] is old PIN2 value
 * ((const char **)data)[1] is new PIN2 value
 * ((const char **)data)[2] is AID value, See ETSI 102.221 8.1 and 101.220 4, NULL if no value.
 * ((const char **)data)[3] is SIL_Card * or null.
 *
 * "response" is int *
 * ((int *)response)[0] is the number of retries remaining, or -1 if unknown
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

#define SIL_REQUEST_CHANGE_SIM_PIN2 8

/**
 * SIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION
 *
 * Requests that network personlization be deactivated
 *
 * "data" is const char **
 * ((const char **)(data))[0]] is network depersonlization code
 * ((const char **)data)[1] is SIL_Card * or null.
 *
 * "response" is int *
 * ((int *)response)[0] is the number of retries remaining, or -1 if unknown
 *
 * Valid errors:
 *
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 *  PASSWORD_INCORRECT
 *     (code is invalid)
 */
#define SIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION 9

/**
 * SIL_REQUEST_SIM_IO
 *
 * Request SIM I/O operation.
 * This is similar to the TS 27.007 "restricted SIM" operation
 * where it assumes all of the EF selection will be done by the
 * callee.
 *
 * "data" is a const SIL_SIM_IO_v7 *
 * Please note that SIL_SIM_IO has a "PIN2" field which may be NULL,
 * or may specify a PIN2 for operations that require a PIN2 (eg
 * updating FDN records)
 *
 * "response" is a const SIL_SIM_IO_Response *
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
#define SIL_REQUEST_SIM_IO 10

/**
 * SIL_REQUEST_WRITE_SMS_TO_SIM
 *
 * Stores a SMS message to SIM memory.
 *
 * "data" is SIL_SMS_WriteArgs *
 *
 * "response" is int *
 * ((const int *)response)[0] is the record index where the message is stored.
 *
 * Valid errors:
 *  SUCCESS
 *  GENERIC_FAILURE
 *
 *
 */
#define SIL_REQUEST_WRITE_SMS_TO_SIM 11

/**
 * SIL_REQUEST_DELETE_SMS_ON_SIM
 *
 * Deletes a SMS message from SIM memory.
 * "data" is const char **
 * ((const char **)data)[0] is the record index of the message to delete.
 * ((const char **)data)[1] is SIL_Card * or null.
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  GENERIC_FAILURE
 *
 */
#define SIL_REQUEST_DELETE_SMS_ON_SIM 12

/**
 * RIL_REQUEST_GET_IMSI
 *
 * Get the SIM IMSI
 *
 * "data" is const char **
 * ((const char **)data)[0] is AID value, See ETSI 102.221 8.1 and 101.220 4, NULL if no value.
 * ((const char **)data)[1] is SIL_Card * or null.
 * "response" is a const char * containing the IMSI
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE (radio resetting)
 *  GENERIC_FAILURE
 */

#define RIL_REQUEST_GET_IMSI 13

/**
 * SIL_REQUEST_STK_GET_PROFILE
 *
 * Requests the profile of SIM tool kit.
 * The profile indicates the SAT/USAT features supported by ME.
 * The SAT/USAT features refer to 3GPP TS 11.14 and 3GPP TS 31.111
 *
 * "data" is SIL_Card *
 *
 * "response" is a const char * containing SAT/USAT profile
 * in hexadecimal format string starting with first byte of terminal profile
 *
 * Valid errors:
 *  SIL_E_SUCCESS
 *  SIL_E_RADIO_NOT_AVAILABLE (radio resetting)
 *  SIL_E_GENERIC_FAILURE
 */
#define SIL_REQUEST_STK_GET_PROFILE 14

/**
 * SIL_REQUEST_STK_SET_PROFILE
 *
 * Download the STK terminal profile as part of SIM initialization
 * procedure
 *
 * "data" is const char **
 * ((const char **)data)[0] is containing SAT/USAT profile
 * in hexadecimal format string starting with first byte of terminal profile
 * ((const char **)data)[1] is SIL_Card * or null.
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SIL_E_SUCCESS
 *  SIL_E_RADIO_NOT_AVAILABLE (radio resetting)
 *  SIL_E_GENERIC_FAILURE
 */
#define SIL_REQUEST_STK_SET_PROFILE 15

/**
 * SIL_REQUEST_STK_SEND_ENVELOPE_COMMAND
 *
 * Requests to send a SAT/USAT envelope command to SIM.
 * The SAT/USAT envelope command refers to 3GPP TS 11.14 and 3GPP TS 31.111
 *
 * "data" is const char **
 * ((const char **)data)[0] containing SAT/USAT command
 * in hexadecimal format string starting with command tag
 * ((const char **)data)[1] is SIL_Card * or null.
 *
 * "response" is a const char * containing SAT/USAT response
 * in hexadecimal format string starting with first byte of response
 * (May be NULL)
 *
 * Valid errors:
 *  SIL_E_SUCCESS
 *  SIL_E_RADIO_NOT_AVAILABLE (radio resetting)
 *  SIL_E_GENERIC_FAILURE
 */
#define SIL_REQUEST_STK_SEND_ENVELOPE_COMMAND 16

/**
 * SIL_REQUEST_STK_SEND_TERMINAL_RESPONSE
 *
 * Requests to send a terminal response to SIM for a received
 * proactive command
 *
 * "data" is const char **
 * ((const char **)data)[0] IS containing SAT/USAT response
 * in hexadecimal format string starting with first byte of response data
 * ((const char **)data)[1] is SIL_Card * or null.
 *
 * "response" is NULL
 *
 * Valid errors:
 *  SIL_E_SUCCESS
 *  SIL_E_RADIO_NOT_AVAILABLE (radio resetting)
 *  SIL_E_GENERIC_FAILURE
 */
#define SIL_REQUEST_STK_SEND_TERMINAL_RESPONSE 17

/**
 * SIL_REQUEST_DEVICE_IDENTITY
 *
 * Request the device ESN / MEID / IMEI / IMEISV.
 *
 * The request is always allowed and contains GSM and CDMA device identity;
 * it substitutes the deprecated requests SIL_REQUEST_GET_IMEI and
 * SIL_REQUEST_GET_IMEISV.
 *
 * If a NULL value is returned for any of the device id, it means that error
 * accessing the device.
 *
 * When CDMA subscription is changed the ESN/MEID may change.  The application
 * layer should re-issue the request to update the device identity in this case.
 *
 * "response" is const char **
 * ((const char **)response)[0] is IMEI if GSM subscription is available
 * ((const char **)response)[1] is IMEISV if GSM subscription is available
 * ((const char **)response)[2] is ESN if CDMA subscription is available
 * ((const char **)response)[3] is MEID if CDMA subscription is available
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
#define SIL_REQUEST_DEVICE_IDENTITY 18

/**
 * SIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING
 *
 * Indicates that the StkSerivce is running and is
 * ready to receive SIL_UNSOL_STK_XXXXX commands.
 *
 * "data" is SIL_Card *
 * "response" is NULL
 *
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 *
 */
#define SIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING 19

/**
 * SIL_REQUEST_ISIM_AUTHENTICATION
 *
 * Request the ISIM application on the UICC to perform AKA
 * challenge/response algorithm for IMS authentication
 *
 * "data" is a const char **
 * ((const char **)data)[0] contains the challenge string in Base64 format
 * ((const char **)data)[1] is SIL_Card * or null.
 *
 * "response" is a const char * containing the response in Base64 format
 * Valid errors:
 *  SUCCESS
 *  RADIO_NOT_AVAILABLE
 *  GENERIC_FAILURE
 */
#define SIL_REQUEST_ISIM_AUTHENTICATION 22

/**
 * SIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS
 *
 * Requests to send a SAT/USAT envelope command to SIM.
 * The SAT/USAT envelope command refers to 3GPP TS 11.14 and 3GPP TS 31.111.
 *
 * This request has one difference from SIL_REQUEST_STK_SEND_ENVELOPE_COMMAND:
 * the SW1 and SW2 status bytes from the UICC response are returned along with
 * the response data, using the same structure as SIL_REQUEST_SIM_IO.
 *
 * The SIL implementation shall perform the normal processing of a '91XX'
 * response in SW1/SW2 to retrieve the pending proactive command and send it
 * as an unsolicited response, as SIL_REQUEST_STK_SEND_ENVELOPE_COMMAND does.
 *
 * "data" is a const char **
 * ((const char **)data)[0] contains the SAT/USAT command
 * in hexadecimal format starting with command tag
 * ((const char **)data)[1] is SIL_Card * or null.
 *
 * "response" is a const SIL_SIM_IO_Response *
 *
 * Valid errors:
 *  SIL_E_SUCCESS
 *  SIL_E_RADIO_NOT_AVAILABLE (radio resetting)
 *  SIL_E_GENERIC_FAILURE
 */
#define SIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS 23

/**
 * SIL_UNSOL_RESPONSE_CARD_STATUS_CHANGED
 *
 * Indicates number of physical SIM cards have changed.
 *
 * Callee will invoke SIL_REQUEST_GET_ICCIDS on main thread

 * "data" is null
 */
#define SIL_UNSOL_RESPONSE_CARD_STATUS_CHANGED 201

/**
 * SIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED
 *
 * Indicates that SIM state changes.
 *
 * Callee will invoke SIL_REQUEST_GET_SIM_STATUS on main thread

 * "data" is SIL_Card
 */
#define SIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED 202

/**
 * SIL_UNSOL_SIM_REFRESH
 *
 * Indicates that file(s) on the SIM have been updated, or the SIM
 * has been reinitialized.
 *
 * In the case where RIL is version 6 or older:
 * "data" is an int *
 * ((int *)data)[0] is a SIL_SimRefreshResult.
 * ((int *)data)[1] is the EFID of the updated file if the result is
 * SIM_FILE_UPDATE or NULL for any other result.
 *
 * In the case where RIL is version 7:
 * "data" is a SIL_SimRefreshResponse_v7 *
 *
 * In the case where ril is version 9:
 * "data" is a SIL_SimRefreshResponse_v8 *
 *
 * Note: If the SIM state changes as a result of the SIM refresh (eg,
 * SIM_READY -> SIM_LOCKED_OR_ABSENT), SIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED
 * should be sent.
 */
#define SIL_UNSOL_SIM_REFRESH 203

/**
 * SIL_UNSOL_STK_SESSION_END
 *
 * Indicate when STK session is terminated by SIM.
 *
 * "data" is a SIL_Card*
 */
#define SIL_UNSOL_STK_SESSION_END 204

/**
 * SIL_UNSOL_STK_PROACTIVE_COMMAND
 *
 * Indicate when SIM issue a STK proactive command to applications
 *
 * "data" is a const char **
 * ((const char **)data)[0] contains SAT/USAT proactive command
 * in hexadecimal format string starting with command tag
 * ((const char **)data)[1] is SIL_Card * or null.
 *
 */
#define SIL_UNSOL_STK_PROACTIVE_COMMAND 205

/**
 * SIL_UNSOL_STK_EVENT_NOTIFY
 *
 * Indicate when SIM notifies applcations some event happens.
 * Generally, application does not need to have any feedback to
 * SIM but shall be able to indicate appropriate messages to users.
 *
 * "data" is a const char **
 * ((const char **)data)[0] contains SAT/USAT commands or responses
 * sent by ME to SIM or commands handled by ME, in hexadecimal format string
 * starting with first byte of response data or command tag
 * ((const char **)data)[1] is SIL_Card * or null.
 *
 */
#define SIL_UNSOL_STK_EVENT_NOTIFY 206

/**
 * SIL_UNSOL_STK_CALL_SETUP
 *
 * Indicate when SIM wants application to setup a voice call.
 *
 * "data" is const char **
 * ((const char *)data)[0] contains timeout value (in milliseconds)
 * ((const char *)data)[1] contains SIL_Card *
 */
#define SIL_UNSOL_STK_CALL_SETUP 207

/**
 * SIL_UNSOL_SIM_SMS_STORAGE_FULL
 *
 * Indicates that SMS storage on the SIM is full.  Sent when the network
 * attempts to deliver a new SMS message.  Messages cannot be saved on the
 * SIM until space is freed.  In particular, incoming Class 2 messages
 * cannot be stored.
 *
 * "data" is SIL_Card *
 *
 */
#define SIL_UNSOL_SIM_SMS_STORAGE_FULL 208

/**
 * SIL_UNSOL_RESPONSE_NEW_SMS_ON_SIM
 *
 * Called when new SMS has been stored on SIM card
 *
 * "data" is const char **
 * ((const char *)data)[0] contains the slot index on the SIM that contains
 * the new message
 * ((const char *)data)[1] contains SIL_Card *
 */
#define SIL_UNSOL_RESPONSE_NEW_SMS_ON_SIM 209

#endif /*ANDROID_SIL_H*/
