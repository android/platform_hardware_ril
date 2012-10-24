/* //device/system/reference-ril/reference-ril.c
**
** Copyright 2006, The Android Open Source Project
** Copyright (c) 2010-2012, Code Aurora Forum. All rights reserved.
** Not a Contribution, Apache license notifications and license are retained
** for attribution purposes only.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <telephony/ril.h>
#include <telephony/ril_cdma_sms.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <alloca.h>
#include "atchannel.h"
#include "at_tok.h"
#include "misc.h"
#include <getopt.h>
#include <sys/socket.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>
#include <termios.h>
#include <sys/system_properties.h>

#include "ril.h"
#include "hardware/qemu_pipe.h"

#define LOG_TAG "RIL"
#define LOG_NDEBUG 0
#define LOG_NDDEBUG 0
#define LOG_NIDEBUG 0
#include <utils/Log.h>

#define MAX_AT_RESPONSE 0x1000

/* pathname returned from RIL_REQUEST_SETUP_DATA_CALL / RIL_REQUEST_SETUP_DEFAULT_PDP */
#define PPP_TTY_PATH "eth0"

#ifdef USE_TI_COMMANDS

// Enable a workaround
// 1) Make incoming call, do not answer
// 2) Hangup remote end
// Expected: call should disappear from CLCC line
// Actual: Call shows as "ACTIVE" before disappearing
#define WORKAROUND_ERRONEOUS_ANSWER 1

// Some varients of the TI stack do not support the +CGEV unsolicited
// response. However, they seem to send an unsolicited +CME ERROR: 150
#define WORKAROUND_FAKE_CGEV 1
#endif

typedef enum {
    SIM_ABSENT = 0,
    SIM_NOT_READY = 1,
    SIM_READY = 2, /* SIM_READY means the radio state is RADIO_STATE_SIM_READY */
    SIM_PIN = 3,
    SIM_PUK = 4,
    SIM_NETWORK_PERSONALIZATION = 5,
    RUIM_ABSENT = 6,
    RUIM_NOT_READY = 7,
    RUIM_READY = 8,
    RUIM_PIN = 9,
    RUIM_PUK = 10,
    RUIM_NETWORK_PERSONALIZATION = 11
} SIM_Status;

/* Reference RIL Instance IDs */
enum {
    REF_RIL_FIRST_INSTANCE_ID = 0,
    REF_RIL_SECOND_INSTANCE_ID = 1,
    REF_RIL_MAX_INSTANCE_ID
};

/* Callback functions for instance 0 */
static void onRequest_func0 (int request, void *data, size_t datalen, RIL_Token t);
static RIL_RadioState currentState_func0();
static int onSupports_func0 (int requestCode);
static void onCancel_func0 (RIL_Token t);
static const char *getVersion_func0();

/* Callback functions for instance 1 */
static void onRequest_func1 (int request, void *data, size_t datalen, RIL_Token t);
static RIL_RadioState currentState_func1();
static int onSupports_func1 (int requestCode);
static void onCancel_func1 (RIL_Token t);
static const char *getVersion_func1();

static int isRadioOn();
static SIM_Status getSIMStatus(int inst_id);
static int getCardStatus(int inst_id, RIL_CardStatus_v6 **pp_card_status);
static void freeCardStatus(RIL_CardStatus_v6 *p_card_status);
static void onDataCallListChanged(void *param);

extern const char * requestToString(int request);

static int isMultiSimEnabled();

/*** Static Variables ***/
static const RIL_RadioFunctions s_callbacks [REF_RIL_MAX_INSTANCE_ID] = {
{
    RIL_VERSION,
    onRequest_func0,
    currentState_func0,
    onSupports_func0,
    onCancel_func0,
    getVersion_func0
},
{
    RIL_VERSION,
    onRequest_func1,
    currentState_func1,
    onSupports_func1,
    onCancel_func1,
    getVersion_func1
}
};

int line_sms = 0;
int mo_call = 0;
int mt_ring_counter = 0;

#ifdef RIL_SHLIB
static const struct RIL_Env *s_rilenv[REF_RIL_MAX_INSTANCE_ID];

#define RIL_onRequestComplete(inst, t, e, response, responselen) do { \
    if(!isMultiSimEnabled() && inst != REF_RIL_FIRST_INSTANCE_ID) {\
        ALOGD(" RIL_onRequestComplete: Error: dsds not enabled and inst != REF_RIL_FIRST_INSTANCE_ID"); \
        break; \
    } \
    ALOGD(" RIL_onRequestComplete: inst = %d", inst); \
    s_rilenv[inst]->OnRequestComplete(t,e, response, responselen); \
} while (0)

#define RIL_onUnsolicitedResponse(inst, a,b,c) do { \
    if(!isMultiSimEnabled() && inst != REF_RIL_FIRST_INSTANCE_ID) {\
        ALOGD(" RIL_onUnsolicitedResponse: Error: dsds not enabled and inst != REF_RIL_FIRST_INSTANCE_ID"); \
        break; \
    } \
    ALOGD(" RIL_onUnsolicitedResponse: inst = %d", inst); \
    s_rilenv[inst]->OnUnsolicitedResponse(a,b,c); \
} while (0)

#define RIL_requestTimedCallback(inst, a,b,c) do { \
    if(!isMultiSimEnabled() && inst != REF_RIL_FIRST_INSTANCE_ID) {\
        ALOGD(" RIL_requestTimedCallback: Error: dsds not enabled and inst != REF_RIL_FIRST_INSTANCE_ID"); \
        break; \
    } \
    ALOGD(" RIL_requestTimedCallback: inst = %d", inst); \
    s_rilenv[inst]->RequestTimedCallback(a,b,c); \
} while (0)

#endif

static RIL_RadioState sState[REF_RIL_MAX_INSTANCE_ID] = {RADIO_STATE_UNAVAILABLE, RADIO_STATE_UNAVAILABLE};

static pthread_mutex_t s_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_state_cond = PTHREAD_COND_INITIALIZER;

static int s_port = -1;
static const char * s_device_path = NULL;
static int          s_device_socket = 0;

/* trigger change to this with s_state_cond */
static int s_closed = 0;

static int sFD;     /* file desc of AT channel */
static char sATBuffer[MAX_AT_RESPONSE+1];
static char *sATBufferCur = NULL;

static const struct timeval TIMEVAL_SIMPOLL = {1,0};
static const struct timeval TIMEVAL_CALLSTATEPOLL = {0,500000};
static const struct timeval TIMEVAL_0 = {0,0};

#ifdef WORKAROUND_ERRONEOUS_ANSWER
// Max number of times we'll try to repoll when we think
// we have a AT+CLCC race condition
#define REPOLL_CALLS_COUNT_MAX 4

// Line index that was incoming or waiting at last poll, or -1 for none
static int s_incomingOrWaitingLine = -1;
// Number of times we've asked for a repoll of AT+CLCC
static int s_repollCallsCount = 0;
// Should we expect a call to be answered in the next CLCC?
static int s_expectAnswer = 0;
#endif /* WORKAROUND_ERRONEOUS_ANSWER */

static void pollSIMState (void *param);
//Need to check all below 5 functions
static void setRadioState (int inst_id, RIL_RadioState newState);
//static void setRadioTechnology(int inst_id, ModemInfo *mdm, int newtech);
//static int techFamilyFromModemType(int mdmtype);

/*jiju*/
static int mySimState[2] = {SIM_READY, SIM_READY};
static int myDds = -1;

static int clccStateToRILState(int state, RIL_CallState *p_state)

{
    switch(state) {
        case 0: *p_state = RIL_CALL_ACTIVE;   return 0;
        case 1: *p_state = RIL_CALL_HOLDING;  return 0;
        case 2: *p_state = RIL_CALL_DIALING;  return 0;
        case 3: *p_state = RIL_CALL_ALERTING; return 0;
        case 4: *p_state = RIL_CALL_INCOMING; return 0;
        case 5: *p_state = RIL_CALL_WAITING;  return 0;
        default: return -1;
    }
}

/**
 * Note: directly modified line and has *p_call point directly into
 * modified line
 */
static int callFromCLCCLine(char *line, RIL_Call *p_call)
{
        //+CLCC: 1,0,2,0,0,\"+18005551212\",145
        //     index,isMT,state,mode,isMpty(,number,TOA)?

    int err;
    int state;
    int mode;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(p_call->index));
    if (err < 0) goto error;

    err = at_tok_nextbool(&line, &(p_call->isMT));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &state);
    if (err < 0) goto error;

    err = clccStateToRILState(state, &(p_call->state));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &mode);
    if (err < 0) goto error;

    p_call->isVoice = (mode == 0);

    err = at_tok_nextbool(&line, &(p_call->isMpty));
    if (err < 0) goto error;

    if (at_tok_hasmore(&line)) {
        err = at_tok_nextstr(&line, &(p_call->number));

        /* tolerate null here */
        if (err < 0) return 0;

        // Some lame implementations return strings
        // like "NOT AVAILABLE" in the CLCC line
        if (p_call->number != NULL
            && 0 == strspn(p_call->number, "+0123456789")
        ) {
            p_call->number = NULL;
        }

        err = at_tok_nextint(&line, &p_call->toa);
        if (err < 0) goto error;
    }

    p_call->uusInfo = NULL;

    return 0;

error:
    ALOGE("invalid CLCC line\n");
    return -1;
}


/** do post-AT+CFUN=1 initialization */
static void onRadioPowerOn(int inst_id)
{
    ALOGD("onRadioPowerOn(%d)", inst_id);
#ifdef USE_TI_COMMANDS
    /*  Must be after CFUN=1 */
    /*  TI specific -- notifications for CPHS things such */
    /*  as CPHS message waiting indicator */

    at_send_command("AT%CPHS=1", NULL);

    /*  TI specific -- enable NITZ unsol notifs */
    at_send_command("AT%CTZV=1", NULL);
#endif

    pollSIMState((void *)inst_id);
}

/** do post- SIM ready initialization */
static void onSIMReady()
{
    at_send_command_singleline("AT+CSMS=1", "+CSMS:", NULL);
    /*
     * Always send SMS messages directly to the TE
     *
     * mode = 1 // discard when link is reserved (link should never be
     *             reserved)
     * mt = 2   // most messages routed to TE
     * bm = 2   // new cell BM's routed to TE
     * ds = 1   // Status reports routed to TE
     * bfr = 1  // flush buffer
     */
    at_send_command("AT+CNMI=1,2,2,1,1", NULL);
}

static void requestRadioPower(int inst_id, void *data, size_t datalen, RIL_Token t)
{
    int onOff;

    int err;
    ATResponse *p_response = NULL;

    assert (datalen >= sizeof(int *));
    onOff = ((int *)data)[0];

    ALOGD("onOff: %d, sState: %d\n", onOff, sState[inst_id]);
    if (onOff == 0 && sState[inst_id] != RADIO_STATE_OFF) {
        err = at_send_command("AT+CFUN=0", &p_response);
       if (err < 0 || p_response->success == 0) goto error;
        setRadioState (inst_id, RADIO_STATE_OFF);
    } else if (onOff > 0 && sState[inst_id] == RADIO_STATE_OFF) {
        err = at_send_command("AT+CFUN=1", &p_response);
        if (err < 0|| p_response->success == 0) {
            // Some stacks return an error when there is no SIM,
            // but they really turn the RF portion on
            // So, if we get an error, let's check to see if it
            // turned on anyway

            if (isRadioOn() != 1) {
                goto error;
            }
        }
        setRadioState (inst_id, RADIO_STATE_ON);
    }

    at_response_free(p_response);
    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
    return;
error:
    at_response_free(p_response);
    RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

static void requestOrSendDataCallList(int inst_id, RIL_Token *t);

static void onDataCallListChanged(void *param)
{
    int inst_id = (int)param;
    requestOrSendDataCallList(inst_id, NULL);
}

static void requestDataCallList(int inst_id, void *data, size_t datalen, RIL_Token t)
{
    requestOrSendDataCallList(inst_id, &t);
}

static void requestOrSendDataCallList(int inst_id, RIL_Token *t)
{
    ATResponse *p_response;
    ATLine *p_cur;
    int err;
    int n = 0;
    char *out;

    err = at_send_command_multiline ("AT+CGACT?", "+CGACT:", &p_response);
    if (err != 0 || p_response->success == 0) {
        if (t != NULL)
            RIL_onRequestComplete(inst_id, *t, RIL_E_GENERIC_FAILURE, NULL, 0);
        else
            RIL_onUnsolicitedResponse(inst_id, RIL_UNSOL_DATA_CALL_LIST_CHANGED,
                                      NULL, 0);
        return;
    }

    for (p_cur = p_response->p_intermediates; p_cur != NULL;
         p_cur = p_cur->p_next)
        n++;

    RIL_Data_Call_Response_v6 *responses =
        alloca(n * sizeof(RIL_Data_Call_Response_v6));

    int i;
    for (i = 0; i < n; i++) {
        responses[i].status = -1;
        responses[i].suggestedRetryTime = -1;
        responses[i].cid = -1;
        responses[i].active = -1;
        responses[i].type = "";
        responses[i].ifname = "";
        responses[i].addresses = "";
        responses[i].dnses = "";
        responses[i].gateways = "";
    }

    RIL_Data_Call_Response_v6 *response = responses;
    for (p_cur = p_response->p_intermediates; p_cur != NULL;
         p_cur = p_cur->p_next) {
        char *line = p_cur->line;

        err = at_tok_start(&line);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &response->cid);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &response->active);
        if (err < 0)
            goto error;

        response++;
    }

    at_response_free(p_response);

    err = at_send_command_multiline ("AT+CGDCONT?", "+CGDCONT:", &p_response);
    if (err != 0 || p_response->success == 0) {
        if (t != NULL)
            RIL_onRequestComplete(inst_id, *t, RIL_E_GENERIC_FAILURE, NULL, 0);
        else
            RIL_onUnsolicitedResponse(inst_id, RIL_UNSOL_DATA_CALL_LIST_CHANGED,
                                      NULL, 0);
        return;
    }

    for (p_cur = p_response->p_intermediates; p_cur != NULL;
         p_cur = p_cur->p_next) {
        char *line = p_cur->line;
        int cid;

        err = at_tok_start(&line);
        if (err < 0)
            goto error;

        err = at_tok_nextint(&line, &cid);
        if (err < 0)
            goto error;

        for (i = 0; i < n; i++) {
            if (responses[i].cid == cid)
                break;
        }

        if (i >= n) {
            /* details for a context we didn't hear about in the last request */
            continue;
        }

        // Assume no error
        responses[i].status = 0;

        // type
        err = at_tok_nextstr(&line, &out);
        if (err < 0)
            goto error;
        responses[i].type = alloca(strlen(out) + 1);
        strcpy(responses[i].type, out);

        // APN ignored for v5
        err = at_tok_nextstr(&line, &out);
        if (err < 0)
            goto error;

        responses[i].ifname = alloca(strlen(PPP_TTY_PATH) + 1);
        strcpy(responses[i].ifname, PPP_TTY_PATH);

        err = at_tok_nextstr(&line, &out);
        if (err < 0)
            goto error;

        responses[i].addresses = alloca(strlen(out) + 1);
        strcpy(responses[i].addresses, out);

        {
            char  propValue[PROP_VALUE_MAX];

            if (__system_property_get("ro.kernel.qemu", propValue) != 0) {
                /* We are in the emulator - the dns servers are listed
                 * by the following system properties, setup in
                 * /system/etc/init.goldfish.sh:
                 *  - net.eth0.dns1
                 *  - net.eth0.dns2
                 *  - net.eth0.dns3
                 *  - net.eth0.dns4
                 */
                const int   dnslist_sz = 128;
                char*       dnslist = alloca(dnslist_sz);
                const char* separator = "";
                int         nn;

                dnslist[0] = 0;
                for (nn = 1; nn <= 4; nn++) {
                    /* Probe net.eth0.dns<n> */
                    char  propName[PROP_NAME_MAX];
                    snprintf(propName, sizeof propName, "net.eth0.dns%d", nn);

                    /* Ignore if undefined */
                    if (__system_property_get(propName, propValue) == 0) {
                        continue;
                    }

                    /* Append the DNS IP address */
                    strlcat(dnslist, separator, dnslist_sz);
                    strlcat(dnslist, propValue, dnslist_sz);
                    separator = " ";
                }
                responses[i].dnses = dnslist;

                /* There is only on gateway in the emulator */
                responses[i].gateways = "10.0.2.2";
            }
            else {
                /* I don't know where we are, so use the public Google DNS
                 * servers by default and no gateway.
                 */
                responses[i].dnses = "8.8.8.8 8.8.4.4";
                responses[i].gateways = "";
            }
        }
    }

    at_response_free(p_response);

    if (t != NULL)
        RIL_onRequestComplete(inst_id, *t, RIL_E_SUCCESS, responses,
                              n * sizeof(RIL_Data_Call_Response_v6));
    else
        RIL_onUnsolicitedResponse(inst_id, RIL_UNSOL_DATA_CALL_LIST_CHANGED,
                                  responses,
                                  n * sizeof(RIL_Data_Call_Response_v6));

    return;

error:
    if (t != NULL)
        RIL_onRequestComplete(inst_id, *t, RIL_E_GENERIC_FAILURE, NULL, 0);
    else
        RIL_onUnsolicitedResponse(inst_id, RIL_UNSOL_DATA_CALL_LIST_CHANGED,
                                  NULL, 0);

    at_response_free(p_response);
}

static void requestQueryNetworkSelectionMode(
                int inst_id, void *data, size_t datalen, RIL_Token t)
{
    int err;
    ATResponse *p_response = NULL;
    int response = 0;
    char *line;

    err = at_send_command_singleline("AT+COPS?", "+COPS:", &p_response);

    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);

    if (err < 0) {
        goto error;
    }

    err = at_tok_nextint(&line, &response);

    if (err < 0) {
        goto error;
    }

    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, &response, sizeof(int));
    at_response_free(p_response);
    return;
error:
    at_response_free(p_response);
    ALOGE("requestQueryNetworkSelectionMode must never return error when radio is on");
    RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
}

static void sendCallStateChanged(void *param)
{
    int inst_id = (int)param;

    ALOGD("sendCallStateChanged () : inst_id = %d", inst_id);

    RIL_onUnsolicitedResponse (
        inst_id,
        RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED,
        NULL, 0);
}

static void requestGetCurrentCalls(int inst_id, void *data, size_t datalen, RIL_Token t)
{
    int err;
    ATResponse *p_response;
    ATLine *p_cur;
    int countCalls;
    int countValidCalls;
    RIL_Call *p_calls;
    RIL_Call **pp_calls;
    int i;
    int needRepoll = 0;

#ifdef WORKAROUND_ERRONEOUS_ANSWER
    int prevIncomingOrWaitingLine;

    prevIncomingOrWaitingLine = s_incomingOrWaitingLine;
    s_incomingOrWaitingLine = -1;
#endif /*WORKAROUND_ERRONEOUS_ANSWER*/

    err = at_send_command_multiline ("AT+CLCC", "+CLCC:", &p_response);

    if (err != 0 || p_response->success == 0) {
        RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
        return;
    }

    /* count the calls */
    for (countCalls = 0, p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next
    ) {
        countCalls++;
    }

    /* yes, there's an array of pointers and then an array of structures */

    pp_calls = (RIL_Call **)alloca(countCalls * sizeof(RIL_Call *));
    p_calls = (RIL_Call *)alloca(countCalls * sizeof(RIL_Call));
    memset (p_calls, 0, countCalls * sizeof(RIL_Call));

    /* init the pointer array */
    for(i = 0; i < countCalls ; i++) {
        pp_calls[i] = &(p_calls[i]);
    }

    for (countValidCalls = 0, p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next
    ) {
        err = callFromCLCCLine(p_cur->line, p_calls + countValidCalls);

        if (err != 0) {
            continue;
        }

#ifdef WORKAROUND_ERRONEOUS_ANSWER
        if (p_calls[countValidCalls].state == RIL_CALL_INCOMING
            || p_calls[countValidCalls].state == RIL_CALL_WAITING
        ) {
            s_incomingOrWaitingLine = p_calls[countValidCalls].index;
        }
#endif /*WORKAROUND_ERRONEOUS_ANSWER*/

        if (p_calls[countValidCalls].state != RIL_CALL_ACTIVE
            && p_calls[countValidCalls].state != RIL_CALL_HOLDING
        ) {
            needRepoll = 1;
        }

        countValidCalls++;
    }

#ifdef WORKAROUND_ERRONEOUS_ANSWER
    // Basically:
    // A call was incoming or waiting
    // Now it's marked as active
    // But we never answered it
    //
    // This is probably a bug, and the call will probably
    // disappear from the call list in the next poll
    if (prevIncomingOrWaitingLine >= 0
            && s_incomingOrWaitingLine < 0
            && s_expectAnswer == 0
    ) {
        for (i = 0; i < countValidCalls ; i++) {

            if (p_calls[i].index == prevIncomingOrWaitingLine
                    && p_calls[i].state == RIL_CALL_ACTIVE
                    && s_repollCallsCount < REPOLL_CALLS_COUNT_MAX
            ) {
                ALOGI(
                    "Hit WORKAROUND_ERRONOUS_ANSWER case."
                    " Repoll count: %d\n", s_repollCallsCount);
                s_repollCallsCount++;
                goto error;
            }
        }
    }

    s_expectAnswer = 0;
    s_repollCallsCount = 0;
#endif /*WORKAROUND_ERRONEOUS_ANSWER*/

    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, pp_calls,
            countValidCalls * sizeof (RIL_Call *));

    at_response_free(p_response);

#ifdef POLL_CALL_STATE
    if (countValidCalls) {  // We don't seem to get a "NO CARRIER" message from
                            // smd, so we're forced to poll until the call ends.
#else
    if (needRepoll) {
#endif
        RIL_requestTimedCallback (inst_id, sendCallStateChanged, (void *)inst_id, &TIMEVAL_CALLSTATEPOLL);
    }

    return;
error:
    RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static RIL_SelectUiccSub currentSelectedSub[2] = {
    {-1, -1, RIL_SUBSCRIPTION_1, RIL_UICC_SUBSCRIPTION_DEACTIVATE },
    {-1, -1, RIL_SUBSCRIPTION_1, RIL_UICC_SUBSCRIPTION_DEACTIVATE }
};

static int isAppReady(int slot, int app_index) {
    int i = 0;

    for (i=0; i<2; i++) {
        if (currentSelectedSub[i].act_status == RIL_UICC_SUBSCRIPTION_ACTIVATE
                && currentSelectedSub[i].slot == slot
                && currentSelectedSub[i].app_index == app_index) {
            ALOGD("isAppReady : slot = %d app_index = %d is READY", slot, app_index);
            return 1;
        }
    }
    ALOGD("isAppReady : slot = %d app_index = %d is NOT READY", slot, app_index);
    return 0;
}

static void setUiccSubscriptionSource(int inst_id, int request, void *data, size_t datalen, RIL_Token t)
{
    RIL_SelectUiccSub *uiccSubscrInfo;
    uiccSubscrInfo = (RIL_SelectUiccSub *)data;
    int response = 0;
    int slot = 0;

    ALOGD("setUiccSubscriptionSource() : inst_id = %d", inst_id);
    slot = uiccSubscrInfo->slot;
    currentSelectedSub[slot].slot = uiccSubscrInfo->slot;
    currentSelectedSub[slot].app_index = uiccSubscrInfo->app_index;
    currentSelectedSub[slot].sub_type = uiccSubscrInfo->sub_type;
    currentSelectedSub[slot].act_status = uiccSubscrInfo->act_status;

    // TODO: DSDS: Need to implement this.
    // workarround: send success for now.
    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);

    if (uiccSubscrInfo->act_status == RIL_UICC_SUBSCRIPTION_ACTIVATE) {
        if (currentSelectedSub[slot].slot == 0
                && mySimState[0] == SIM_NOT_READY) {
            mySimState[0] = SIM_READY;
        }
        ALOGD("setUiccSubscriptionSource() : Activate Request: sending SUBSCRIPTION_STATUS_CHANGED");
        response = 1; // ACTIVATED
        RIL_onUnsolicitedResponse (
            inst_id,
            RIL_UNSOL_UICC_SUBSCRIPTION_STATUS_CHANGED,
            &response, sizeof(response));
    } else {
        ALOGD("setUiccSubscriptionSource() : Deactivate Request");
    }
    RIL_onUnsolicitedResponse(uiccSubscrInfo->slot, RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED, NULL, 0);
}

static void setSubscriptionMode(int instance_id, int request, void *data, size_t datalen, RIL_Token t)
{
    ALOGD("getUiccSubscriptionSource() : instance_id = %d", instance_id);
    // TODO: DSDS: Need to implement this.
    // workarround: send success for now.
    RIL_onRequestComplete(instance_id, t, RIL_E_SUCCESS, NULL, 0);
}


static void getUiccSubscriptionSource(int inst_id, int request, void *data, size_t datalen, RIL_Token t)
{
    ALOGD("getUiccSubscriptionSource() : inst_id = %d", inst_id);
    // TODO: DSDS: Need to implement this.
    // workarround: send success for now.
    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
}

static void setDataSubscriptionSource(int inst_id, int request, void *data, size_t datalen, RIL_Token t)
{
    ALOGD("setDataSubscriptionSource() : inst_id = %d", inst_id) ;
    myDds = inst_id;
    // TODO: DSDS: Need to implement this.
    // workarround: send success for now.
    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);

    inst_id = (inst_id == 0) ? 1 : 0;
    ALOGD("setDataSubscriptionSource() : sending all data disconnected on SUB: = %d", inst_id);
    RIL_onUnsolicitedResponse(inst_id, RIL_UNSOL_DATA_CALL_LIST_CHANGED,
                              NULL, 0);
}

static void getDataSubscriptionSource(int inst_id, int request, void *data, size_t datalen, RIL_Token t)
{
    ALOGD("getDataSubscriptionSource() : inst_id = %d", inst_id);
    // TODO: DSDS: Need to implement this.
    // workarround: send success for now.
    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
}


static void requestDial(int inst_id, void *data, size_t datalen, RIL_Token t)
{
    RIL_Dial *p_dial;
    char *cmd;
    const char *clir;
    int ret;

    ALOGD("requestDial() : inst_id = %d", inst_id);
    mo_call = 1;

    p_dial = (RIL_Dial *)data;

    switch (p_dial->clir) {
        case 1: clir = "I"; break;  /*invocation*/
        case 2: clir = "i"; break;  /*suppression*/
        default:
        case 0: clir = ""; break;   /*subscription default*/
    }

    asprintf(&cmd, "ATD%s%s;", p_dial->address, clir);

    ret = at_send_command(cmd, NULL);

    free(cmd);

    /* success or failure is ignored by the upper layer here.
       it will call GET_CURRENT_CALLS and determine success that way */
    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
}

static void requestWriteSmsToSim(int inst_id, void *data, size_t datalen, RIL_Token t)
{
    RIL_SMS_WriteArgs *p_args;
    char *cmd;
    int length;
    int err;
    ATResponse *p_response = NULL;

    p_args = (RIL_SMS_WriteArgs *)data;

    length = strlen(p_args->pdu)/2;
    asprintf(&cmd, "AT+CMGW=%d,%d", length, p_args->status);

    err = at_send_command_sms(cmd, p_args->pdu, "+CMGW:", &p_response);

    if (err != 0 || p_response->success == 0) goto error;

    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);

    return;
error:
    RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static void requestHangup(int inst_id, void *data, size_t datalen, RIL_Token t)
{
    int *p_line;

    int ret;
    char *cmd;

    ALOGD("requestHangup() : inst_id = %d", inst_id);
    mo_call = 0;

    p_line = (int *)data;

    // 3GPP 22.030 6.5.5
    // "Releases a specific active call X"
    asprintf(&cmd, "AT+CHLD=1%d", p_line[0]);

    ret = at_send_command(cmd, NULL);

    free(cmd);

    /* success or failure is ignored by the upper layer here.
       it will call GET_CURRENT_CALLS and determine success that way */
    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
}

static void requestSignalStrength(int inst_id, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    char *line;
    int count =0;
    int numofElements=sizeof(RIL_SignalStrength_v6)/sizeof(int);
    int response[numofElements];

    err = at_send_command_singleline("AT+CSQ", "+CSQ:", &p_response);

    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    for (count =0; count < numofElements; count ++) {
        err = at_tok_nextint(&line, &(response[count]));
        if (err < 0) goto error;
    }

    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, response, sizeof(response));

    at_response_free(p_response);
    return;

error:
    ALOGE("requestSignalStrength must never return an error when radio is on");
    RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static void requestRegistrationState(int inst_id, int request, void *data,
                                        size_t datalen, RIL_Token t)
{
    int err;
    int response[4];
    char * responseStr[4];
    ATResponse *p_response = NULL;
    const char *cmd;
    const char *prefix;
    char *line, *p;
    int commas;
    int skip;
    int count = 3;


    if (request == RIL_REQUEST_VOICE_REGISTRATION_STATE) {
        cmd = "AT+CREG?";
        prefix = "+CREG:";
    } else if (request == RIL_REQUEST_DATA_REGISTRATION_STATE) {
        cmd = "AT+CGREG?";
        prefix = "+CGREG:";
    } else {
        assert(0);
        goto error;
    }

    err = at_send_command_singleline(cmd, prefix, &p_response);

    if (err != 0) goto error;

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    /* Ok you have to be careful here
     * The solicited version of the CREG response is
     * +CREG: n, stat, [lac, cid]
     * and the unsolicited version is
     * +CREG: stat, [lac, cid]
     * The <n> parameter is basically "is unsolicited creg on?"
     * which it should always be
     *
     * Now we should normally get the solicited version here,
     * but the unsolicited version could have snuck in
     * so we have to handle both
     *
     * Also since the LAC and CID are only reported when registered,
     * we can have 1, 2, 3, or 4 arguments here
     *
     * finally, a +CGREG: answer may have a fifth value that corresponds
     * to the network type, as in;
     *
     *   +CGREG: n, stat [,lac, cid [,networkType]]
     */

    /* count number of commas */
    commas = 0;
    for (p = line ; *p != '\0' ;p++) {
        if (*p == ',') commas++;
    }

    switch (commas) {
        case 0: /* +CREG: <stat> */
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            response[1] = -1;
            response[2] = -1;
        break;

        case 1: /* +CREG: <n>, <stat> */
            err = at_tok_nextint(&line, &skip);
            if (err < 0) goto error;
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            response[1] = -1;
            response[2] = -1;
            if (err < 0) goto error;
        break;

        case 2: /* +CREG: <stat>, <lac>, <cid> */
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[1]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[2]);
            if (err < 0) goto error;
        break;
        case 3: /* +CREG: <n>, <stat>, <lac>, <cid> */
            err = at_tok_nextint(&line, &skip);
            if (err < 0) goto error;
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[1]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[2]);
            if (err < 0) goto error;
        break;
        /* special case for CGREG, there is a fourth parameter
         * that is the network type (unknown/gprs/edge/umts)
         */
        case 4: /* +CGREG: <n>, <stat>, <lac>, <cid>, <networkType> */
            err = at_tok_nextint(&line, &skip);
            if (err < 0) goto error;
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[1]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[2]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[3]);
            if (err < 0) goto error;
            count = 4;
        break;
        default:
            goto error;
    }

    asprintf(&responseStr[0], "%d", response[0]);
    asprintf(&responseStr[1], "%x", response[1]);
    asprintf(&responseStr[2], "%x", response[2]);

    if (count > 3)
        asprintf(&responseStr[3], "%d", response[3]);

    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, responseStr, count*sizeof(char*));
    at_response_free(p_response);

    return;
error:
    ALOGE("requestRegistrationState must never return an error when radio is on");
    RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static void requestOperator(int inst_id, void *data, size_t datalen, RIL_Token t)
{
    int err;
    int i;
    int skip;
    ATLine *p_cur;
    char *response[3];

    memset(response, 0, sizeof(response));

    ATResponse *p_response = NULL;

    ALOGD("requestOperator(): inst_id = %d", inst_id);

    err = at_send_command_multiline(
        "AT+COPS=3,0;+COPS?;+COPS=3,1;+COPS?;+COPS=3,2;+COPS?",
        "+COPS:", &p_response);

    /* we expect 3 lines here:
     * +COPS: 0,0,"T - Mobile"
     * +COPS: 0,1,"TMO"
     * +COPS: 0,2,"310170"
     */

    if (err != 0) goto error;

    for (i = 0, p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next, i++
    ) {
        char *line = p_cur->line;

        err = at_tok_start(&line);
        if (err < 0) goto error;

        err = at_tok_nextint(&line, &skip);
        if (err < 0) goto error;

        // If we're unregistered, we may just get
        // a "+COPS: 0" response
        if (!at_tok_hasmore(&line)) {
            response[i] = NULL;
            continue;
        }

        err = at_tok_nextint(&line, &skip);
        if (err < 0) goto error;

        // a "+COPS: 0, n" response is also possible
        if (!at_tok_hasmore(&line)) {
            response[i] = NULL;
            continue;
        }

        err = at_tok_nextstr(&line, &(response[i]));
        if (err < 0) goto error;
    }

    if (i != 3) {
        /* expect 3 lines exactly */
        goto error;
    }

    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, response, sizeof(response));
    at_response_free(p_response);

    return;
error:
    ALOGE("requestOperator must not return error when radio is on");
    RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static void requestSendSMS(int inst_id, void *data, size_t datalen, RIL_Token t)
{
    int err;
    const char *smsc;
    const char *pdu;
    int tpLayerLength;
    char *cmd1, *cmd2;
    RIL_SMS_Response response;
    ATResponse *p_response = NULL;

    ALOGD("requestSendSMS datalen =%d", datalen);
    smsc = ((const char **)data)[0];
    pdu = ((const char **)data)[1];

    tpLayerLength = strlen(pdu)/2;

    ALOGD("requestSendSMS(): inst_id = %d", inst_id);

    // "NULL for default SMSC"
    if (smsc == NULL) {
        smsc= "00";
    }
    ALOGD("smsc=%s, pdu=%s", smsc, pdu);

    asprintf(&cmd1, "AT+CMGS=%d", tpLayerLength);
    asprintf(&cmd2, "%s%s", smsc, pdu);

    err = at_send_command_sms(cmd1, cmd2, "+CMGS:", &p_response);

    if (err != 0 || p_response->success == 0) goto error;

    memset(&response, 0, sizeof(response));

    /* FIXME fill in messageRef and ackPDU */

    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, &response, sizeof(response));
    at_response_free(p_response);

    return;
error:
    RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

static void requestSetupDataCall(int inst_id, void *data, size_t datalen, RIL_Token t)
{
    const char *apn;
    char *cmd;
    int err;
    ATResponse *p_response = NULL;

    apn = ((const char **)data)[2];

#ifdef USE_TI_COMMANDS
    // Config for multislot class 10 (probably default anyway eh?)
    err = at_send_command("AT%CPRIM=\"GMM\",\"CONFIG MULTISLOT_CLASS=<10>\"",
                        NULL);

    err = at_send_command("AT%DATA=2,\"UART\",1,,\"SER\",\"UART\",0", NULL);
#endif /* USE_TI_COMMANDS */

    int fd, qmistatus;
    size_t cur = 0;
    size_t len;
    ssize_t written, rlen;
    char status[32] = {0};
    int retry = 10;
    const char *pdp_type;

    ALOGD("requesting data connection to APN '%s'", apn);

    fd = open ("/dev/qmi", O_RDWR);
    if (fd >= 0) { /* the device doesn't exist on the emulator */

        ALOGD("opened the qmi device\n");
        asprintf(&cmd, "up:%s", apn);
        len = strlen(cmd);

        while (cur < len) {
            do {
                written = write (fd, cmd + cur, len - cur);
            } while (written < 0 && errno == EINTR);

            if (written < 0) {
                ALOGE("### ERROR writing to /dev/qmi");
                close(fd);
                goto error;
            }

            cur += written;
        }

        // wait for interface to come online

        do {
            sleep(1);
            do {
                rlen = read(fd, status, 31);
            } while (rlen < 0 && errno == EINTR);

            if (rlen < 0) {
                ALOGE("### ERROR reading from /dev/qmi");
                close(fd);
                goto error;
            } else {
                status[rlen] = '\0';
                ALOGD("### status: %s", status);
            }
        } while (strncmp(status, "STATE=up", 8) && strcmp(status, "online") && --retry);

        close(fd);

        if (retry == 0) {
            ALOGE("### Failed to get data connection up\n");
            goto error;
        }

        qmistatus = system("netcfg rmnet0 dhcp");

        ALOGD("netcfg rmnet0 dhcp: status %d\n", qmistatus);

        if (qmistatus < 0) goto error;

    } else {

        if (datalen > 6 * sizeof(char *)) {
            pdp_type = ((const char **)data)[6];
        } else {
            pdp_type = "IP";
        }

        asprintf(&cmd, "AT+CGDCONT=1,\"%s\",\"%s\",,0,0", pdp_type, apn);
        //FIXME check for error here
        err = at_send_command(cmd, NULL);
        free(cmd);

        // Set required QoS params to default
        err = at_send_command("AT+CGQREQ=1", NULL);

        // Set minimum QoS params to default
        err = at_send_command("AT+CGQMIN=1", NULL);

        // packet-domain event reporting
        err = at_send_command("AT+CGEREP=1,0", NULL);

        // Hangup anything that's happening there now
        err = at_send_command("AT+CGACT=1,0", NULL);

        // Start data on PDP context 1
        err = at_send_command("ATD*99***1#", &p_response);

        if (err < 0 || p_response->success == 0) {
            goto error;
        }
    }

    requestOrSendDataCallList(inst_id, &t);

    at_response_free(p_response);

    return;
error:
    RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);

}

static void requestSMSAcknowledge(int inst_id, void *data, size_t datalen, RIL_Token t)
{
    int ackSuccess;
    int err;

    ackSuccess = ((int *)data)[0];

    if (ackSuccess == 1) {
        err = at_send_command("AT+CNMA=1", NULL);
    } else if (ackSuccess == 0)  {
        err = at_send_command("AT+CNMA=2", NULL);
    } else {
        ALOGE("unsupported arg to RIL_REQUEST_SMS_ACKNOWLEDGE\n");
        goto error;
    }

    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
error:
    RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);

}

static void  requestSIM_IO(int inst_id, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    RIL_SIM_IO_Response sr;
    int err;
    char *cmd = NULL;
    RIL_SIM_IO_v6 *p_args;
    char *line;

    memset(&sr, 0, sizeof(sr));

    p_args = (RIL_SIM_IO_v6 *)data;

    ALOGD("requestSIM_IO(): inst_id = %d fileid = %d", inst_id, p_args->fileid);

    /* FIXME handle pin2 */

    if (p_args->data == NULL) {
        asprintf(&cmd, "AT+CRSM=%d,%d,%d,%d,%d",
                    p_args->command, p_args->fileid,
                    p_args->p1, p_args->p2, p_args->p3);
    } else {
        asprintf(&cmd, "AT+CRSM=%d,%d,%d,%d,%d,%s",
                    p_args->command, p_args->fileid,
                    p_args->p1, p_args->p2, p_args->p3, p_args->data);
    }

    err = at_send_command_singleline(cmd, "+CRSM:", &p_response);

    if (err < 0 || p_response->success == 0) {
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(sr.sw1));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(sr.sw2));
    if (err < 0) goto error;

    if (at_tok_hasmore(&line)) {
        err = at_tok_nextstr(&line, &(sr.simResponse));
        if (err < 0) goto error;
    }

    if (p_args->fileid == 0x2FE2) {
        if (p_args->command == 0xb0) {
            if (inst_id == REF_RIL_SECOND_INSTANCE_ID) {
                sr.simResponse = "88004433112288557700";
            }
            ALOGD("requestSIM_IO(): Read Request for ICCID on inst_id - %d. ICCID = %s",
                    inst_id, sr.simResponse);
        }
    }


    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, &sr, sizeof(sr));
    at_response_free(p_response);
    free(cmd);

    return;
error:
    RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    free(cmd);

}

static void  requestEnterSimPin(int inst_id, void*  data, size_t  datalen, RIL_Token  t)
{
    ATResponse   *p_response = NULL;
    int           err;
    char*         cmd = NULL;
    const char**  strings = (const char**)data;;

    if ( datalen == sizeof(char*) ) {
        asprintf(&cmd, "AT+CPIN=%s", strings[0]);
    } else if ( datalen == 2*sizeof(char*) || datalen == 3 * sizeof(char*)) {
        asprintf(&cmd, "AT+CPIN=%s,%s", strings[0], strings[1]);
    } else {
        goto error;
    }

    err = at_send_command_singleline(cmd, "+CPIN:", &p_response);
    free(cmd);

    if (err < 0 || p_response->success == 0) {
error:
        RIL_onRequestComplete(inst_id, t, RIL_E_PASSWORD_INCORRECT, NULL, 0);
    } else {
        RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
}


static void  requestSendUSSD(int inst_id, void *data, size_t datalen, RIL_Token t)
{
    const char *ussdRequest;

    ussdRequest = (char *)(data);


    RIL_onRequestComplete(inst_id, t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);

// @@@ TODO

}


/*** Callback methods from the RIL library to us ***/

/**
 * Call from RIL to us to make a RIL_REQUEST
 *
 * Must be completed with a call to RIL_onRequestComplete()
 *
 * RIL_onRequestComplete() may be called from any thread, before or after
 * this function returns.
 *
 * Will always be called from the same thread, so returning here implies
 * that the radio is ready to process another command (whether or not
 * the previous command has completed).
 */
static void
onRequest (int inst_id, int request, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response;
    int err;

    ALOGD("onRequest: %s, inst_id = %d", requestToString(request), inst_id);

    /* Ignore all requests except RIL_REQUEST_GET_SIM_STATUS
     * when RADIO_STATE_UNAVAILABLE.
     */
    if (sState[inst_id] == RADIO_STATE_UNAVAILABLE
        && !(request == RIL_REQUEST_GET_SIM_STATUS)
    ) {
        RIL_onRequestComplete(inst_id, t, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
        return;
    }

    /* Ignore all non-power requests when RADIO_STATE_OFF
     * (except RIL_REQUEST_GET_SIM_STATUS)
     */
    if (sState[inst_id] == RADIO_STATE_OFF
        && !(request == RIL_REQUEST_RADIO_POWER
            || request == RIL_REQUEST_GET_SIM_STATUS)
    ) {
        RIL_onRequestComplete(inst_id, t, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
        return;
    }

    switch (request) {
        case RIL_REQUEST_GET_SIM_STATUS: {
            RIL_CardStatus_v6 *p_card_status;
            char *p_buffer;
            int buffer_size;

            int result = getCardStatus(inst_id, &p_card_status);
            if (result == RIL_E_SUCCESS) {
                p_buffer = (char *)p_card_status;
                buffer_size = sizeof(*p_card_status);
            } else {
                p_buffer = NULL;
                buffer_size = 0;
            }
            RIL_onRequestComplete(inst_id, t, result, p_buffer, buffer_size);
            freeCardStatus(p_card_status);
            break;
        }
        case RIL_REQUEST_GET_CURRENT_CALLS:
            requestGetCurrentCalls(inst_id, data, datalen, t);
            break;
        case RIL_REQUEST_DIAL:
            requestDial(inst_id, data, datalen, t);
            break;
        case RIL_REQUEST_HANGUP:
            requestHangup(inst_id, data, datalen, t);
            break;
        case RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND:
            // 3GPP 22.030 6.5.5
            // "Releases all held calls or sets User Determined User Busy
            //  (UDUB) for a waiting call."
            at_send_command("AT+CHLD=0", NULL);

            /* success or failure is ignored by the upper layer here.
               it will call GET_CURRENT_CALLS and determine success that way */
            RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
            break;
        case RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND:
            // 3GPP 22.030 6.5.5
            // "Releases all active calls (if any exist) and accepts
            //  the other (held or waiting) call."
            at_send_command("AT+CHLD=1", NULL);
            mo_call = 0;

            /* success or failure is ignored by the upper layer here.
               it will call GET_CURRENT_CALLS and determine success that way */
            RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
            break;
        case RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE:
            // 3GPP 22.030 6.5.5
            // "Places all active calls (if any exist) on hold and accepts
            //  the other (held or waiting) call."
            at_send_command("AT+CHLD=2", NULL);

#ifdef WORKAROUND_ERRONEOUS_ANSWER
            s_expectAnswer = 1;
#endif /* WORKAROUND_ERRONEOUS_ANSWER */

            /* success or failure is ignored by the upper layer here.
               it will call GET_CURRENT_CALLS and determine success that way */
            RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
            break;
        case RIL_REQUEST_ANSWER:
            at_send_command("ATA", NULL);

#ifdef WORKAROUND_ERRONEOUS_ANSWER
            s_expectAnswer = 1;
#endif /* WORKAROUND_ERRONEOUS_ANSWER */

            /* success or failure is ignored by the upper layer here.
               it will call GET_CURRENT_CALLS and determine success that way */
            RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
            break;
        case RIL_REQUEST_CONFERENCE:
            // 3GPP 22.030 6.5.5
            // "Adds a held call to the conversation"
            at_send_command("AT+CHLD=3", NULL);

            /* success or failure is ignored by the upper layer here.
               it will call GET_CURRENT_CALLS and determine success that way */
            RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
            break;
        case RIL_REQUEST_UDUB:
            /* user determined user busy */
            /* sometimes used: ATH */
            at_send_command("ATH", NULL);

            /* success or failure is ignored by the upper layer here.
               it will call GET_CURRENT_CALLS and determine success that way */
            RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
            break;

        case RIL_REQUEST_SEPARATE_CONNECTION:
            {
                char  cmd[12];
                int   party = ((int*)data)[0];

                // Make sure that party is in a valid range.
                // (Note: The Telephony middle layer imposes a range of 1 to 7.
                // It's sufficient for us to just make sure it's single digit.)
                if (party > 0 && party < 10) {
                    sprintf(cmd, "AT+CHLD=2%d", party);
                    at_send_command(cmd, NULL);
                    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
                } else {
                    RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
                }
            }
            break;

        case RIL_REQUEST_SIGNAL_STRENGTH:
            requestSignalStrength(inst_id, data, datalen, t);
            break;
        case RIL_REQUEST_VOICE_REGISTRATION_STATE:
        case RIL_REQUEST_DATA_REGISTRATION_STATE:
            requestRegistrationState(inst_id, request, data, datalen, t);
            break;
        case RIL_REQUEST_OPERATOR:
            requestOperator(inst_id, data, datalen, t);
            break;
        case RIL_REQUEST_RADIO_POWER:
            requestRadioPower(inst_id, data, datalen, t);
            break;
        case RIL_REQUEST_DTMF: {
            char c = ((char *)data)[0];
            char *cmd;
            asprintf(&cmd, "AT+VTS=%c", (int)c);
            at_send_command(cmd, NULL);
            free(cmd);
            RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
            break;
        }
        case RIL_REQUEST_SEND_SMS:
            requestSendSMS(inst_id, data, datalen, t);
            break;
        case RIL_REQUEST_SETUP_DATA_CALL:
            requestSetupDataCall(inst_id, data, datalen, t);
            break;
        case RIL_REQUEST_SMS_ACKNOWLEDGE:
            requestSMSAcknowledge(inst_id, data, datalen, t);
            break;

        case RIL_REQUEST_GET_IMSI:
            p_response = NULL;
            err = at_send_command_numeric("AT+CIMI", &p_response);

            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS,
                    p_response->p_intermediates->line, sizeof(char *));
            }
            at_response_free(p_response);
            break;

        case RIL_REQUEST_GET_IMEI:
            ALOGD("RIL_REQUEST_GET_IMEI");
            p_response = NULL;
            err = at_send_command_numeric("AT+CGSN", &p_response);

            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS,
                    p_response->p_intermediates->line, sizeof(char *));
            }
            at_response_free(p_response);
            break;

        case RIL_REQUEST_SIM_IO:
            requestSIM_IO(inst_id, data,datalen,t);
            break;

        case RIL_REQUEST_SEND_USSD:
            requestSendUSSD(inst_id, data, datalen, t);
            break;

        case RIL_REQUEST_CANCEL_USSD:
            p_response = NULL;
            err = at_send_command_numeric("AT+CUSD=2", &p_response);

            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS,
                    p_response->p_intermediates->line, sizeof(char *));
            }
            at_response_free(p_response);
            break;

        case RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC:
            at_send_command("AT+COPS=0", NULL);
            break;

        case RIL_REQUEST_DATA_CALL_LIST:
            requestDataCallList(inst_id, data, datalen, t);
            break;

        case RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE:
            requestQueryNetworkSelectionMode(inst_id, data, datalen, t);
            break;

        case RIL_REQUEST_OEM_HOOK_RAW:
            // echo back data
            RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, data, datalen);
            break;


        case RIL_REQUEST_OEM_HOOK_STRINGS: {
            int i;
            const char ** cur;

            ALOGD("got OEM_HOOK_STRINGS: 0x%8p %lu", data, (long)datalen);


            for (i = (datalen / sizeof (char *)), cur = (const char **)data ;
                    i > 0 ; cur++, i --) {
                ALOGD("> '%s'", *cur);
            }

            // echo back strings
            RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, data, datalen);
            break;
        }

        case RIL_REQUEST_WRITE_SMS_TO_SIM:
            requestWriteSmsToSim(inst_id, data, datalen, t);
            break;

        case RIL_REQUEST_DELETE_SMS_ON_SIM: {
            char * cmd;
            p_response = NULL;
            asprintf(&cmd, "AT+CMGD=%d", ((int *)data)[0]);
            err = at_send_command(cmd, &p_response);
            free(cmd);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
            } else {
                RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, NULL, 0);
            }
            at_response_free(p_response);
            break;
        }

        case RIL_REQUEST_ENTER_SIM_PIN:
        case RIL_REQUEST_ENTER_SIM_PUK:
        case RIL_REQUEST_ENTER_SIM_PIN2:
        case RIL_REQUEST_ENTER_SIM_PUK2:
        case RIL_REQUEST_CHANGE_SIM_PIN:
        case RIL_REQUEST_CHANGE_SIM_PIN2:
            requestEnterSimPin(inst_id, data, datalen, t);
            break;

/* TODO:       case RIL_REQUEST_VOICE_RADIO_TECH:
            {
                int techfam = techFamilyFromModemType(TECH(sMdmInfo));
                if (techfam < 0 )
                    RIL_onRequestComplete(inst_id, t, RIL_E_GENERIC_FAILURE, NULL, 0);
                else
                    RIL_onRequestComplete(inst_id, t, RIL_E_SUCCESS, &techfam, sizeof(&techfam));
            }
            break;*/

        case RIL_REQUEST_SET_UICC_SUBSCRIPTION:
            setUiccSubscriptionSource(inst_id, request, data, datalen, t);
            break;
        case RIL_REQUEST_SET_DATA_SUBSCRIPTION:
            setDataSubscriptionSource(inst_id, request, data, datalen, t);
            break;
        case RIL_REQUEST_GET_UICC_SUBSCRIPTION:
            getUiccSubscriptionSource(inst_id, request, data, datalen, t);
            break;
        case RIL_REQUEST_GET_DATA_SUBSCRIPTION:
            getDataSubscriptionSource(inst_id, request, data, datalen, t);
            break;
        case RIL_REQUEST_SET_SUBSCRIPTION_MODE:
            setSubscriptionMode(inst_id, request, data, datalen, t);
            break;
        default:
            ALOGD("Request not supported. Tech:" );
            RIL_onRequestComplete(inst_id, t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}

/**
 * Call from RIL instance 0 to us to make a RIL_REQUEST
 *
 * Must be completed with a call to RIL_onRequestComplete()
 *
 * RIL_onRequestComplete() may be called from any thread, before or after
 * this function returns.
 *
 * Will always be called from the same thread, so returning here implies
 * that the radio is ready to process another command (whether or not
 * the previous command has completed).
 */
static void
onRequest_func0 (int request, void *data, size_t datalen, RIL_Token t)
{
    onRequest (REF_RIL_FIRST_INSTANCE_ID, request, data, datalen, t);
}

/**
 * Call from RIL instance 1 to us to make a RIL_REQUEST
 *
 * Must be completed with a call to RIL_onRequestComplete()
 *
 * RIL_onRequestComplete() may be called from any thread, before or after
 * this function returns.
 *
 * Will always be called from the same thread, so returning here implies
 * that the radio is ready to process another command (whether or not
 * the previous command has completed).
 */
static void
onRequest_func1 (int request, void *data, size_t datalen, RIL_Token t)
{
    onRequest (REF_RIL_SECOND_INSTANCE_ID, request, data, datalen, t);
}


/**
 * Synchronous call from the RIL to us to return current radio state.
 * RADIO_STATE_UNAVAILABLE should be the initial state.
 */
static RIL_RadioState
currentState(int inst_id)
{
    if (inst_id == REF_RIL_FIRST_INSTANCE_ID) {
        return sState[REF_RIL_FIRST_INSTANCE_ID];
    } else {
        return sState[REF_RIL_SECOND_INSTANCE_ID];
    }
}

/**
 * Call from RIL instance 0 for currentState
 */
static RIL_RadioState
currentState_func0()
{
    ALOGD("currentState_func0");
    return currentState(REF_RIL_FIRST_INSTANCE_ID);
}

/**
 * Call from RIL instance 1 for currentState
 */
static RIL_RadioState
currentState_func1()
{
    ALOGD("currentState_func1");
    return currentState(REF_RIL_SECOND_INSTANCE_ID);
}

/**
 * Call from RIL to us to find out whether a specific request code
 * is supported by this implementation.
 *
 * Return 1 for "supported" and 0 for "unsupported"
 */

static int
onSupports (int inst_id, int requestCode)
{
    //@@@ todo

    return 1;
}

/**
 * Call from RIL instance 0 to us to find out whether a specific request code
 * is supported by this implementation.
 *
 * Return 1 for "supported" and 0 for "unsupported"
 */
static int
onSupports_func0(int requestCode)
{
    return onSupports(REF_RIL_FIRST_INSTANCE_ID, requestCode);
}

/**
 * Call from RIL instance 1 to us to find out whether a specific request code
 * is supported by this implementation.
 *
 * Return 1 for "supported" and 0 for "unsupported"
 */
static int
onSupports_func1(int requestCode)
{
    return onSupports(REF_RIL_SECOND_INSTANCE_ID, requestCode);
}


static void onCancel (int instance, RIL_Token t)
{
    //@@@todo

}

/**
 * Call from RIL instance 2 for onCancel
 */
static void onCancel_func0 (RIL_Token t)
{
    onCancel(REF_RIL_FIRST_INSTANCE_ID, t);
}

/**
 * Call from RIL instance 1 for onCancel
 */
static void onCancel_func1 (RIL_Token t)
{
    onCancel(REF_RIL_SECOND_INSTANCE_ID, t);
}

static const char * getVersion(int inst_id)
{
    return "android reference-ril 1.0";
}

/**
 * Call from RIL instance 0 for getVersion
 */
static const char * getVersion_func0(void)
{
    return getVersion(REF_RIL_FIRST_INSTANCE_ID);
}

/**
 * Call from RIL instance 1 for getVersion
 */
static const char * getVersion_func1(void)
{
    return getVersion(REF_RIL_SECOND_INSTANCE_ID);
}

/*static void
setRadioTechnology(int inst_id, ModemInfo *mdm, int newtech)
{
    ALOGD("setRadioTechnology(%d, %d)", inst_id, newtech);

    int oldtech = TECH(mdm);

    if (newtech != oldtech) {
        ALOGD("Tech change (%d => %d)", oldtech, newtech);
        TECH(mdm) = newtech;
        if (techFamilyFromModemType(newtech) != techFamilyFromModemType(oldtech)) {
            RIL_onUnsolicitedResponse(inst_id, RIL_UNSOL_VOICE_RADIO_TECH_CHANGED, NULL, 0);
        }
    }
}
*/

static void
setRadioState (int inst_id, RIL_RadioState newState)
{
    ALOGD("setRadioState (%d, %d)", inst_id, newState);
    RIL_RadioState oldState;

    pthread_mutex_lock(&s_state_mutex);

    oldState = sState[inst_id];

    if (s_closed > 0) {
        // If we're closed, the only reasonable state is
        // RADIO_STATE_UNAVAILABLE
        // This is here because things on the main thread
        // may attempt to change the radio state after the closed
        // event happened in another thread
        newState = RADIO_STATE_UNAVAILABLE;
    }

    if (sState[inst_id] != newState || s_closed > 0) {
        sState[inst_id] = newState;
        pthread_cond_broadcast (&s_state_cond);
    }

    pthread_mutex_unlock(&s_state_mutex);


    /* do these outside of the mutex */
    if (sState[inst_id] != oldState) {
        RIL_onUnsolicitedResponse (inst_id, RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED,
                                    NULL, 0);

         //TODO: suresh check this is proper ?
        /* FIXME onSimReady() and onRadioPowerOn() cannot be called
         * from the AT reader thread
         * Currently, this doesn't happen, but if that changes then these
         * will need to be dispatched on the request thread
         */
     //   if (sState[inst_id] == RADIO_STATE_SIM_READY) {
     //       onSIMReady();
     //   } else
        if (sState[inst_id] == RADIO_STATE_ON) {
            onRadioPowerOn(inst_id);
        }
    }
}

/** Returns SIM_NOT_READY on error */
static SIM_Status
getSIMStatus(int inst_id)
{
    ATResponse *p_response = NULL;
    int err;
    int ret;
    char *cpinLine;
    char *cpinResult;

    ALOGD("getSIMStatus(). sState[%d]: %d",inst_id, sState[inst_id]);
    if (sState[inst_id] == RADIO_STATE_OFF || sState[inst_id] == RADIO_STATE_UNAVAILABLE) {
        ret = SIM_NOT_READY;
        goto done;
    }

    err = at_send_command_singleline("AT+CPIN?", "+CPIN:", &p_response);

    if (err != 0) {
        ret = SIM_NOT_READY;
        goto done;
    }

    switch (at_get_cme_error(p_response)) {
        case CME_SUCCESS:
            break;

        case CME_SIM_NOT_INSERTED:
            ret = SIM_ABSENT;
            goto done;

        default:
            ret = SIM_NOT_READY;
            goto done;
    }

    /* CPIN? has succeeded, now look at the result */

    cpinLine = p_response->p_intermediates->line;
    err = at_tok_start (&cpinLine);

    if (err < 0) {
        ret = SIM_NOT_READY;
        goto done;
    }

    err = at_tok_nextstr(&cpinLine, &cpinResult);

    if (err < 0) {
        ret = SIM_NOT_READY;
        goto done;
    }

    if (0 == strcmp (cpinResult, "SIM PIN")) {
        ret = SIM_PIN;
        goto done;
    } else if (0 == strcmp (cpinResult, "SIM PUK")) {
        ret = SIM_PUK;
        goto done;
    } else if (0 == strcmp (cpinResult, "PH-NET PIN")) {
        return SIM_NETWORK_PERSONALIZATION;
    } else if (0 != strcmp (cpinResult, "READY"))  {
        /* we're treating unsupported lock types as "sim absent" */
        ret = SIM_ABSENT;
        goto done;
    }

    at_response_free(p_response);
    p_response = NULL;
    cpinResult = NULL;

    //ret = SIM_READY;
    ret = mySimState[inst_id];

done:
    at_response_free(p_response);
    return ret;
}


/**
 * Get the current card status.
 *
 * This must be freed using freeCardStatus.
 * @return: On success returns RIL_E_SUCCESS
 */
static int getCardStatus(int inst_id, RIL_CardStatus_v6 **pp_card_status) {
    static RIL_AppStatus app_status_array[] = {
        // SIM_ABSENT = 0
        { RIL_APPTYPE_UNKNOWN, RIL_APPSTATE_UNKNOWN, RIL_PERSOSUBSTATE_UNKNOWN,
          NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_UNKNOWN },
        // SIM_NOT_READY = 1
        { RIL_APPTYPE_SIM, RIL_APPSTATE_DETECTED, RIL_PERSOSUBSTATE_UNKNOWN,
          NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_UNKNOWN },
        // SIM_READY = 2
        { RIL_APPTYPE_SIM, RIL_APPSTATE_READY, RIL_PERSOSUBSTATE_READY,
          NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_UNKNOWN },
        // SIM_PIN = 3
        { RIL_APPTYPE_SIM, RIL_APPSTATE_PIN, RIL_PERSOSUBSTATE_UNKNOWN,
          NULL, NULL, 0, RIL_PINSTATE_ENABLED_NOT_VERIFIED, RIL_PINSTATE_UNKNOWN },
        // SIM_PUK = 4
        { RIL_APPTYPE_SIM, RIL_APPSTATE_PUK, RIL_PERSOSUBSTATE_UNKNOWN,
          NULL, NULL, 0, RIL_PINSTATE_ENABLED_BLOCKED, RIL_PINSTATE_UNKNOWN },
        // SIM_NETWORK_PERSONALIZATION = 5
        { RIL_APPTYPE_SIM, RIL_APPSTATE_SUBSCRIPTION_PERSO, RIL_PERSOSUBSTATE_SIM_NETWORK,
          NULL, NULL, 0, RIL_PINSTATE_ENABLED_NOT_VERIFIED, RIL_PINSTATE_UNKNOWN },
        // RUIM_ABSENT = 6
        { RIL_APPTYPE_UNKNOWN, RIL_APPSTATE_UNKNOWN, RIL_PERSOSUBSTATE_UNKNOWN,
          NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_UNKNOWN },
        // RUIM_NOT_READY = 7
        { RIL_APPTYPE_RUIM, RIL_APPSTATE_DETECTED, RIL_PERSOSUBSTATE_UNKNOWN,
          NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_UNKNOWN },
        // RUIM_READY = 8
        { RIL_APPTYPE_RUIM, RIL_APPSTATE_READY, RIL_PERSOSUBSTATE_READY,
          NULL, NULL, 0, RIL_PINSTATE_UNKNOWN, RIL_PINSTATE_UNKNOWN },
        // RUIM_PIN = 9
        { RIL_APPTYPE_RUIM, RIL_APPSTATE_PIN, RIL_PERSOSUBSTATE_UNKNOWN,
          NULL, NULL, 0, RIL_PINSTATE_ENABLED_NOT_VERIFIED, RIL_PINSTATE_UNKNOWN },
        // RUIM_PUK = 10
        { RIL_APPTYPE_RUIM, RIL_APPSTATE_PUK, RIL_PERSOSUBSTATE_UNKNOWN,
          NULL, NULL, 0, RIL_PINSTATE_ENABLED_BLOCKED, RIL_PINSTATE_UNKNOWN },
        // RUIM_NETWORK_PERSONALIZATION = 11
        { RIL_APPTYPE_RUIM, RIL_APPSTATE_SUBSCRIPTION_PERSO, RIL_PERSOSUBSTATE_SIM_NETWORK,
           NULL, NULL, 0, RIL_PINSTATE_ENABLED_NOT_VERIFIED, RIL_PINSTATE_UNKNOWN }
    };
    RIL_CardState card_state;
    int num_apps;

    int sim_status = getSIMStatus(inst_id);

    if (sim_status == SIM_ABSENT) {
        card_state = RIL_CARDSTATE_ABSENT;
        num_apps = 0;
    } else {
        card_state = RIL_CARDSTATE_PRESENT;
        // Only one app here. get sim status for app index : 0.
        if (isAppReady(inst_id, 0) == 0) {
            sim_status = SIM_NOT_READY;
        }
        num_apps = 1;
    }

    // Allocate and initialize base card status.
    RIL_CardStatus_v6 *p_card_status = malloc(sizeof(RIL_CardStatus_v6));
    p_card_status->card_state = card_state;
    p_card_status->universal_pin_state = RIL_PINSTATE_UNKNOWN;
    p_card_status->gsm_umts_subscription_app_index = RIL_CARD_MAX_APPS;
    p_card_status->cdma_subscription_app_index = RIL_CARD_MAX_APPS;
    p_card_status->ims_subscription_app_index = RIL_CARD_MAX_APPS;
    p_card_status->num_applications = num_apps;

    // Initialize application status
    int i;
    for (i = 0; i < RIL_CARD_MAX_APPS; i++) {
        p_card_status->applications[i] = app_status_array[SIM_ABSENT];
    }

    // Pickup the appropriate application status
    // that reflects sim_status for gsm.
    if (num_apps != 0) {
        // Only support one app, gsm
        p_card_status->num_applications = 1;
//        TODO: Multiple application support
        p_card_status->gsm_umts_subscription_app_index = 0;

        // Get the correct app status
        p_card_status->applications[0] = app_status_array[sim_status];
    }

    *pp_card_status = p_card_status;
    return RIL_E_SUCCESS;
}

/**
 * Free the card status returned by getCardStatus
 */
static void freeCardStatus(RIL_CardStatus_v6 *p_card_status) {
    free(p_card_status);
}

/**
 * SIM ready means any commands that access the SIM will work, including:
 *  AT+CPIN, AT+CSMS, AT+CNMI, AT+CRSM
 *  (all SMS-related commands)
 */

static void pollSIMState (void *param)
{
    ATResponse *p_response;
    int ret;
    int inst_id = (int)param;

    if (sState[inst_id] != RADIO_STATE_SIM_NOT_READY) {
        // no longer valid to poll
        return;
    }

    switch(getSIMStatus(inst_id)) {
        case SIM_ABSENT:
        case SIM_PIN:
        case SIM_PUK:
        case SIM_NETWORK_PERSONALIZATION:
        default:
            setRadioState(inst_id, RADIO_STATE_SIM_LOCKED_OR_ABSENT);
        return;

        case SIM_NOT_READY:
            RIL_requestTimedCallback (inst_id, pollSIMState, NULL, &TIMEVAL_SIMPOLL);
        return;

        case SIM_READY:
            setRadioState(inst_id, RADIO_STATE_SIM_READY);
        return;
    }
}

/** returns 1 if on, 0 if off, and -1 on error */
static int isRadioOn()
{
    ATResponse *p_response = NULL;
    int err;
    char *line;
    char ret;

    err = at_send_command_singleline("AT+CFUN?", "+CFUN:", &p_response);

    if (err < 0 || p_response->success == 0) {
        // assume radio is off
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextbool(&line, &ret);
    if (err < 0) goto error;

    at_response_free(p_response);

    return (int)ret;

error:

    at_response_free(p_response);
    return -1;
}

/**
 * Initialize everything that can be configured while we're still in
 * AT+CFUN=0
 */
static void initializeCallback(void *param)
{
    ATResponse *p_response = NULL;
    int err;

    int inst_id = (int)param;

    ALOGD("initializeCallback(%d)", inst_id);

    setRadioState (inst_id, RADIO_STATE_OFF);

    at_handshake();

    /* note: we don't check errors here. Everything important will
       be handled in onATTimeout and onATReaderClosed */

    /*  atchannel is tolerant of echo but it must */
    /*  have verbose result codes */
    at_send_command("ATE0Q0V1", NULL);

    /*  No auto-answer */
    at_send_command("ATS0=0", NULL);

    /*  Extended errors */
    at_send_command("AT+CMEE=1", NULL);

    /*  Network registration events */
    err = at_send_command("AT+CREG=2", &p_response);

    /* some handsets -- in tethered mode -- don't support CREG=2 */
    if (err < 0 || p_response->success == 0) {
        at_send_command("AT+CREG=1", NULL);
    }

    at_response_free(p_response);

    /*  GPRS registration events */
    at_send_command("AT+CGREG=1", NULL);

    /*  Call Waiting notifications */
    at_send_command("AT+CCWA=1", NULL);

    /*  Alternating voice/data off */
    at_send_command("AT+CMOD=0", NULL);

    /*  Not muted */
    at_send_command("AT+CMUT=0", NULL);

    /*  +CSSU unsolicited supp service notifications */
    at_send_command("AT+CSSN=0,1", NULL);

    /*  no connected line identification */
    at_send_command("AT+COLP=0", NULL);

    /*  HEX character set */
    at_send_command("AT+CSCS=\"HEX\"", NULL);

    /*  USSD unsolicited */
    at_send_command("AT+CUSD=1", NULL);

    /*  Enable +CGEV GPRS event notifications, but don't buffer */
    at_send_command("AT+CGEREP=1,0", NULL);

    /*  SMS PDU mode */
    at_send_command("AT+CMGF=0", NULL);

#ifdef USE_TI_COMMANDS

    at_send_command("AT%CPI=3", NULL);

    /*  TI specific -- notifications when SMS is ready (currently ignored) */
    at_send_command("AT%CSTAT=1", NULL);

#endif /* USE_TI_COMMANDS */


    /* assume radio is off on error */
    if (isRadioOn() > 0) {
        setRadioState (inst_id, RADIO_STATE_ON);
    }
}

static void waitForClose()
{
    pthread_mutex_lock(&s_state_mutex);

    while (s_closed == 0) {
        pthread_cond_wait(&s_state_cond, &s_state_mutex);
    }

    pthread_mutex_unlock(&s_state_mutex);
}

static void handleMyUnsolProactiveCommands(char *myUnsol) {
    char *unsolCmd;
    int err = at_tok_start(&myUnsol);
    if (err < 0) {
        ALOGE("Error ::: bailing out %d\n ", err);
    }
    at_tok_nextstr(&myUnsol, &unsolCmd);
    ALOGD("--jiju--: unsolCmd = %s", unsolCmd);

    if (strcmp(unsolCmd, "MY_UNSOL_CARD_REMOVED_0") == 0) {
        ALOGD("--jiju--: MY_UNSOL: Card removed : 0");
        mySimState[0] = SIM_ABSENT;
        RIL_onUnsolicitedResponse(0, RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED, NULL, 0);
    } else if (strcmp(unsolCmd, "MY_UNSOL_CARD_INSERTED_0") == 0) {
        ALOGD("--jiju--: MY_UNSOL: Card inserted : 0");
        //mySimState[0] = SIM_READY;
        mySimState[0] = SIM_NOT_READY;
        RIL_onUnsolicitedResponse(0, RIL_UNSOL_RESPONSE_SIM_STATUS_CHANGED, NULL, 0);
    } else if (strcmp(unsolCmd, "MY_UNSOL_SUB_DEACTIVATED_0") == 0) {
        ALOGD("--jiju--: MY_UNSOL: Subscription deactivated : 0");
        int response = 0; // DEACTIVATED
        RIL_onUnsolicitedResponse (
            0, RIL_UNSOL_UICC_SUBSCRIPTION_STATUS_CHANGED,
            &response, sizeof(response));
        if (myDds == 0) {
            ALOGD("--jiju--: sending all data disconnected on SUB: = %d", myDds);
            RIL_onUnsolicitedResponse(myDds, RIL_UNSOL_DATA_CALL_LIST_CHANGED, NULL, 0);
        }
    } else if (strcmp(unsolCmd, "MY_UNSOL_DATA_DISCONNECTED") == 0) {
        if (myDds != -1) {
            RIL_onUnsolicitedResponse(myDds, RIL_UNSOL_DATA_CALL_LIST_CHANGED, NULL, 0);
        }
    } else if (strcmp(unsolCmd, "MY_UNSOL_RADIO_OFF") == 0) {
        sState[0] = RADIO_STATE_OFF;
        RIL_onUnsolicitedResponse (0, RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED,
                                    NULL, 0);
    } else {
        ALOGD("--jiju--: MY_UNSOL: NONE!");
    }
}

/**
 * Called by atchannel when an unsolicited line appears
 * This is called on atchannel's reader thread. AT commands may
 * not be issued here
 */
static void onUnsolicited (const char *s, const char *sms_pdu)
{
    char *line = NULL, *p;
    int err;
    int inst_id = REF_RIL_FIRST_INSTANCE_ID;

    /* Ignore unsolicited responses until we're initialized.
     * This is OK because the RIL library will poll for initial state
     */
    if (sState[inst_id] == RADIO_STATE_UNAVAILABLE) {
        return;
    }

    if (strStartsWith(s, "%CTZV:")) {
        /* TI specific -- NITZ time */
        char *response;

        line = p = strdup(s);
        at_tok_start(&p);

        err = at_tok_nextstr(&p, &response);

        free(line);
        if (err != 0) {
            ALOGE("invalid NITZ line %s\n", s);
        } else {
            RIL_onUnsolicitedResponse (
                inst_id,
                RIL_UNSOL_NITZ_TIME_RECEIVED,
                response, strlen(response));
        }
    } else if (strStartsWith(s,"+CRING:")
                || strStartsWith(s,"RING")
                || strStartsWith(s,"NO CARRIER")
                || strStartsWith(s,"+CCWA")
    ) {
        // Workarround for simulating the incomming call on instance 1 and 2
        // Alternate unsol call state changed will be sent to each instances.
        if (!mo_call) {
            if (isMultiSimEnabled()) {
                if (strStartsWith(s,"RING")) {
                    mt_ring_counter++;
                    if (mt_ring_counter % 2 == 0) {
                        inst_id = REF_RIL_FIRST_INSTANCE_ID;
                    } else {
                        inst_id = REF_RIL_SECOND_INSTANCE_ID;
                    }
                    ALOGE("Incoming Ring. Sending UNSOL_RESPONSE_CALL_STATE_CHANGED on %d", inst_id);
                }
            }

            RIL_onUnsolicitedResponse (
                    inst_id,
                    RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED,
                    NULL, 0);
        }
#ifdef WORKAROUND_FAKE_CGEV
        RIL_requestTimedCallback (inst_id, onDataCallListChanged, (void *)inst_id, NULL); //TODO use new function
#endif /* WORKAROUND_FAKE_CGEV */
    } else if (strStartsWith(s,"+CREG:")
                || strStartsWith(s,"+CGREG:")
    ) {
        RIL_onUnsolicitedResponse (
            inst_id,
            RIL_UNSOL_RESPONSE_VOICE_NETWORK_STATE_CHANGED,
            NULL, 0);
#ifdef WORKAROUND_FAKE_CGEV
        RIL_requestTimedCallback (inst_id, onDataCallListChanged, NULL, NULL);
#endif /* WORKAROUND_FAKE_CGEV */
    } else if (strStartsWith(s, "+CMT:")) {
        // Workarround to simulate the incomming SMS on instance 1 and 2
        // Alternate unsol new sms will be sent to each of the instances.
        if (isMultiSimEnabled()) {
            if (line_sms == 0) {
                inst_id = REF_RIL_FIRST_INSTANCE_ID;
                line_sms = 1;
            } else {
                inst_id = REF_RIL_SECOND_INSTANCE_ID;
                line_sms = 0;
            }
        }

        RIL_onUnsolicitedResponse (
            inst_id,
            RIL_UNSOL_RESPONSE_NEW_SMS,
            sms_pdu, strlen(sms_pdu));
    } else if (strStartsWith(s, "+CDS:")) {
        RIL_onUnsolicitedResponse (
            inst_id,
            RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT,
            sms_pdu, strlen(sms_pdu));
    } else if (strStartsWith(s, "+CGEV:")) {
        /* Really, we can ignore NW CLASS and ME CLASS events here,
         * but right now we don't since extranous
         * RIL_UNSOL_DATA_CALL_LIST_CHANGED calls are tolerated
         */
        /* can't issue AT commands here -- call on main thread */
        RIL_requestTimedCallback (inst_id, onDataCallListChanged, (void *)inst_id, NULL);
#ifdef WORKAROUND_FAKE_CGEV
    } else if (strStartsWith(s, "+CME ERROR: 150")) {
        RIL_requestTimedCallback (inst_id, onDataCallListChanged, (void *)inst_id, NULL);
#endif /* WORKAROUND_FAKE_CGEV */
    } else if(strStartsWith(s,"+MYUNSOL:")) {
        char *myUnsol = NULL;
        ALOGD("--jiju--: s = %s", s);
        myUnsol = strdup(s);
        handleMyUnsolProactiveCommands(myUnsol);
    }
}

/* Called on command or reader thread */
static void onATReaderClosed()
{
    ALOGI("AT channel closed\n");
    at_close();
    s_closed = 1;

    setRadioState (REF_RIL_FIRST_INSTANCE_ID, RADIO_STATE_UNAVAILABLE);
    if (isMultiSimEnabled()) {
        setRadioState (REF_RIL_SECOND_INSTANCE_ID, RADIO_STATE_UNAVAILABLE);
    }
}

/* Called on command thread */
static void onATTimeout()
{
    ALOGI("AT channel timeout; closing\n");
    at_close();

    s_closed = 1;

    /* FIXME cause a radio reset here */

    setRadioState (REF_RIL_FIRST_INSTANCE_ID, RADIO_STATE_UNAVAILABLE);
    if (isMultiSimEnabled()) {
        setRadioState (REF_RIL_SECOND_INSTANCE_ID, RADIO_STATE_UNAVAILABLE);
    }
}

static void usage(char *s)
{
#ifdef RIL_SHLIB
    fprintf(stderr, "reference-ril requires: -p <tcp port> or -d /dev/tty_device\n");
#else
    fprintf(stderr, "usage: %s [-p <tcp port>] [-d /dev/tty_device]\n", s);
    exit(-1);
#endif
}

static int isMultiSimEnabled()
{
    int enabled = 1;
    char prop_val[PROPERTY_VALUE_MAX];
    if (property_get("persist.multisim.config", prop_val, "0") > 0)
    {
        if ((strncmp(prop_val, "dsds", 4) == 0) || (strncmp(prop_val, "dsda", 4) == 0)) {
            enabled = 1;
        }
    }
    ALOGD("REF_RIL: isMultiSimEnabled: prop_val = %s enabled = %d", prop_val, enabled);
    return enabled;
}

static void *
mainLoop(void *param)
{
    int fd;
    int ret;

    int inst_id = (int)param;

    ALOGD("mainLoop(%d)", inst_id);

    AT_DUMP("== ", "entering mainLoop()", -1 );
    at_set_on_reader_closed(onATReaderClosed);
    at_set_on_timeout(onATTimeout);

    for (;;) {
        // Open the socket and initialize the at channel only once.
        if (inst_id == REF_RIL_FIRST_INSTANCE_ID) {
        fd = -1;
        while  (fd < 0) {
            if (s_port > 0) {
                fd = socket_loopback_client(s_port, SOCK_STREAM);
            } else if (s_device_socket) {
                if (!strcmp(s_device_path, "/dev/socket/qemud")) {
                    /* Before trying to connect to /dev/socket/qemud (which is
                     * now another "legacy" way of communicating with the
                     * emulator), we will try to connecto to gsm service via
                     * qemu pipe. */
                    fd = qemu_pipe_open("qemud:gsm");
                    if (fd < 0) {
                        /* Qemu-specific control socket */
                        fd = socket_local_client( "qemud",
                                                  ANDROID_SOCKET_NAMESPACE_RESERVED,
                                                  SOCK_STREAM );
                        if (fd >= 0 ) {
                            char  answer[2];

                            if ( write(fd, "gsm", 3) != 3 ||
                                 read(fd, answer, 2) != 2 ||
                                 memcmp(answer, "OK", 2) != 0)
                            {
                                close(fd);
                                fd = -1;
                            }
                       }
                    }
                }
                else
                    fd = socket_local_client( s_device_path,
                                            ANDROID_SOCKET_NAMESPACE_FILESYSTEM,
                                            SOCK_STREAM );
            } else if (s_device_path != NULL) {
                fd = open (s_device_path, O_RDWR);
                if ( fd >= 0 && !memcmp( s_device_path, "/dev/ttyS", 9 ) ) {
                    /* disable echo on serial ports */
                    struct termios  ios;
                    tcgetattr( fd, &ios );
                    ios.c_lflag = 0;  /* disable ECHO, ICANON, etc... */
                    tcsetattr( fd, TCSANOW, &ios );
                }
            }

            if (fd < 0) {
                perror ("opening AT interface. retrying...");
                sleep(10);
                /* never returns */
            }
        }

        s_closed = 0;
        ret = at_open(fd, onUnsolicited);

        if (ret < 0) {
            ALOGE ("AT error %d on at_open\n", ret);
            return 0;
        }
        }

        RIL_requestTimedCallback(inst_id, initializeCallback, (void *)inst_id, &TIMEVAL_0);

        // Give initializeCallback a chance to dispatched, since
        // we don't presently have a cancellation mechanism
        sleep(1);

        waitForClose();
        ALOGI("Re-opening after close");
    }
}

#ifdef RIL_SHLIB

pthread_t s_tid_mainloop_0;
pthread_t s_tid_mainloop_1;

const RIL_RadioFunctions *RIL_Init(const struct RIL_Env *env, int argc, char **argv)
{
    int ret;
    int fd = -1;
    int opt;
    pthread_attr_t attr;
    int inst_id = REF_RIL_FIRST_INSTANCE_ID;

    optind = 1;

    while ( -1 != (opt = getopt(argc, argv, "p:d:s:c:"))) {
        switch (opt) {
            case 'p':
                s_port = atoi(optarg);
                if (s_port == 0) {
                    usage(argv[0]);
                    return NULL;
                }
                ALOGI("Opening loopback port %d\n", s_port);
            break;

            case 'd':
                s_device_path = optarg;
                ALOGI("Opening tty device %s\n", s_device_path);
            break;

            case 's':
                s_device_path   = optarg;
                s_device_socket = 1;
                ALOGI("Opening socket %s\n", s_device_path);
            break;

            case 'c':
                inst_id = atoi(optarg);
                ALOGI("instance Id : %d", inst_id);
            break;

            default:
                usage(argv[0]);
                return NULL;
        }
    }

    s_rilenv[inst_id] = env;
    ALOGD("RIL_Init : inst_id = %d", inst_id);

    if (s_port < 0 && s_device_path == NULL) {
        usage(argv[0]);
        return NULL;
    }

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    // Create mainLoop thread for instance 1 or 2.
    if (inst_id == REF_RIL_FIRST_INSTANCE_ID) {
        ret = pthread_create(&s_tid_mainloop_0, &attr, mainLoop, (void *)inst_id);
    } else {
        ret = pthread_create(&s_tid_mainloop_1, &attr, mainLoop, (void *)inst_id);
    }

    return &s_callbacks[inst_id];
}
#else /* RIL_SHLIB */
int main (int argc, char **argv)
{
    int ret;
    int fd = -1;
    int opt;

    while ( -1 != (opt = getopt(argc, argv, "p:d:"))) {
        switch (opt) {
            case 'p':
                s_port = atoi(optarg);
                if (s_port == 0) {
                    usage(argv[0]);
                }
                ALOGI("Opening loopback port %d\n", s_port);
            break;

            case 'd':
                s_device_path = optarg;
                ALOGI("Opening tty device %s\n", s_device_path);
            break;

            case 's':
                s_device_path   = optarg;
                s_device_socket = 1;
                ALOGI("Opening socket %s\n", s_device_path);
            break;

            default:
                usage(argv[0]);
        }
    }

    if (s_port < 0 && s_device_path == NULL) {
        usage(argv[0]);
    }

    RIL_register(&s_callbacks);

    mainLoop(NULL);

    return 0;
}

#endif /* RIL_SHLIB */
