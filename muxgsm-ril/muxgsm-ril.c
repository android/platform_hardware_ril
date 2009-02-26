/* //hardware/ril/muxgsm-ril/muxgsm-ril.c
 **
 ** Copyright 2006, Google Inc.
 ** Copyright 2008-2009, Wind River - Based on reference-ril
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
#include <termios.h>

#define LOG_TAG "RIL"
#include <utils/Log.h>

#define MAX_AT_RESPONSE 0x1000

// Enable workaround for bug in (TI-based) HTC stack
// 1) Make incoming call, do not answer
// 2) Hangup remote end
// Expected: call should disappear from CLCC line
// Actual: Call shows as "ACTIVE" before disappearing
#define WORKAROUND_ERRONEOUS_ANSWER 1

// Some varients of the TI stack do not support the +CGEV unsolicited
// response. However, they seem to send an unsolicited +CME ERROR: 150
#define WORKAROUND_FAKE_CGEV 1

#define FACILITY_LOCK_REQUEST   "2"

#define PPP_OPTION_FILE "/data/misc/ppp/options.gprs"

static void onRequest (int request, void *data, size_t datalen, RIL_Token t);
static RIL_RadioState currentState();
static int onSupports (int requestCode);
static void onCancel (RIL_Token t);
static const char *getVersion();
static int isRadioOn();
static int getSIMStatus();
static void onPDPContextListChanged(void *param);

extern const char * requestToString(int request);

/*** Static Variables ***/
static const RIL_RadioFunctions s_callbacks =
{
    RIL_VERSION,
    onRequest,
    currentState,
    onSupports,
    onCancel,
    getVersion
};

static const struct RIL_Env *s_rilenv;

#define RIL_onRequestComplete(t, e, response, responselen) s_rilenv->OnRequestComplete(t,e, response, responselen)
#define RIL_onUnsolicitedResponse(a,b,c) s_rilenv->OnUnsolicitedResponse(a,b,c)
#define RIL_requestTimedCallback(a,b,c) s_rilenv->RequestTimedCallback(a,b,c)

static RIL_RadioState sState = RADIO_STATE_UNAVAILABLE;

static pthread_mutex_t s_state_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_state_cond = PTHREAD_COND_INITIALIZER;

static int s_port = -1;
static const char * s_device_path = NULL;
static int          s_device_socket = 0;

/* trigger change to this with s_state_cond */
static int s_closed = 0;

static int sFD;                  /* file desc of AT channel */
static char sATBuffer[MAX_AT_RESPONSE+1];
static char *sATBufferCur = NULL;

static int baudRate;
static int screen_on;

#define SIM_PHONEBOOK_READY    (1 << 0)
#define SIM_CPIN_READY         (1 << 1)

static int simFlags;

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
#endif                           /* WORKAROUND_ERRONEOUS_ANSWER */

static void pollSIMState (void *param);
static void setRadioState(RIL_RadioState newState);

struct baud_table
{
    int       rate;
    u_int32_t baud;
};

static const struct baud_table bTable[] =
{
    {      0, B0 },
    {   9600, B9600 },
    {  19200, B19200 },
    {  38400, B38400 },
    {  57600, B57600 },
    { 115200, B115200 },
    { 230400, B230400 },
    { 460800, B460800 },
    { 921600, B921600 },
};

static const int bTable_Len = (sizeof(bTable) / sizeof(struct baud_table));

static void set_term_params (int fd, int baudrate, int hwflow)
{
    int i;
    u_int32_t baud = 0;
    struct termios ios;

    LOGI("Setting tty device parameters\n");

    for (i = 0; i < bTable_Len; i++)
    if (bTable[i].rate == baudrate) {
        baud = bTable[i].baud;
        break;
    }
    if (baud == 0) return;

    i = tcgetattr(fd, &ios);
    if (i < 0) return;

    ios.c_lflag = 0;             /* disable ECHO, ICANON, etc... */

    if (hwflow)
        ios.c_cflag |= CRTSCTS;  /* use hardware flow control */

    cfsetispeed(&ios, B0);
    cfsetospeed(&ios, baud);

    tcsetattr(fd, TCSANOW, &ios);
}


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

    return 0;

    error:
    LOGE("invalid CLCC line\n");
    return -1;
}


/** do post-AT+CFUN=1 initialization */
static void onRadioPowerOn()
{
    /*  enable TZ unsol notifs */
    at_send_command(AT_CMD, "AT+CTZU=1", NULL);
    at_send_command(AT_CMD, "AT+CTZR=1", NULL);

    pollSIMState(NULL);
}


/** do post- SIM ready initialization */
static void onSIMReady()
{
    at_send_command_singleline(AT_CMD, "AT+CSMS=1", "+CSMS:", NULL);
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
    at_send_command(AT_CMD, "AT+CNMI=1,2,2,1,1", NULL);
}


static void requestRadioPower(void *data, size_t datalen, RIL_Token t)
{
    int onOff;

    int err;
    ATResponse *p_response = NULL;

    assert (datalen >= sizeof(int *));
    onOff = ((int *)data)[0];

    if (onOff == 0 && sState != RADIO_STATE_OFF) {
        err = at_send_command(AT_CMD, "AT+CFUN=0", &p_response);
        if (err < 0 || p_response->success == 0) goto error;
        setRadioState(RADIO_STATE_OFF);
    }
    else if (onOff > 0 && sState == RADIO_STATE_OFF) {
        err = at_send_command(AT_CMD, "AT+CFUN=1", &p_response);
        if (err < 0|| p_response->success == 0) {
            // Some stacks return an error when there is no SIM,
            // but they really turn the RF portion on
            // So, if we get an error, let's check to see if it
            // turned on anyway

            if (isRadioOn() != 1) {
                goto error;
            }
        }
        setRadioState(RADIO_STATE_SIM_NOT_READY);
    }

    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;
    error:
    at_response_free(p_response);
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}


static void requestOrSendPDPContextList(RIL_Token *t);

static void onPDPContextListChanged(void *param)
{
    requestOrSendPDPContextList(NULL);
}


static void requestPDPContextList(void *data, size_t datalen, RIL_Token t)
{
    requestOrSendPDPContextList(&t);
}


static void requestOrSendPDPContextList(RIL_Token *t)
{
    ATResponse *p_response;
    ATLine *p_cur;
    int err;
    int n = 0;
    char *out;

    err = at_send_command_multiline (AT_CMD, "AT+CGACT?", "+CGACT:", &p_response);
    if (err != 0 || p_response->success == 0) {
        if (t != NULL)
            RIL_onRequestComplete(*t, RIL_E_GENERIC_FAILURE, NULL, 0);
        else
            RIL_onUnsolicitedResponse(RIL_UNSOL_PDP_CONTEXT_LIST_CHANGED,
                NULL, 0);
        return;
    }

    for (p_cur = p_response->p_intermediates; p_cur != NULL;
        p_cur = p_cur->p_next)
    n++;

    RIL_PDP_Context_Response *responses =
        alloca(n * sizeof(RIL_PDP_Context_Response));

    int i;
    for (i = 0; i < n; i++) {
        responses[i].cid = -1;
        responses[i].active = -1;
        responses[i].type = "";
        responses[i].apn = "";
        responses[i].address = "";
    }

    RIL_PDP_Context_Response *response = responses;
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

    err = at_send_command_multiline (AT_CMD, "AT+CGDCONT?", "+CGDCONT:", &p_response);
    if (err != 0 || p_response->success == 0) {
        if (t != NULL)
            RIL_onRequestComplete(*t, RIL_E_GENERIC_FAILURE, NULL, 0);
        else
            RIL_onUnsolicitedResponse(RIL_UNSOL_PDP_CONTEXT_LIST_CHANGED,
                NULL, 0);
        return;
    }

    for (p_cur = p_response->p_intermediates; p_cur != NULL;
    p_cur = p_cur->p_next) {
        char *line = p_cur->line;
        int cid;
        char *type;
        char *apn;
        char *address;

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

        err = at_tok_nextstr(&line, &out);
        if (err < 0)
            goto error;

        responses[i].type = alloca(strlen(out) + 1);
        strcpy(responses[i].type, out);

        err = at_tok_nextstr(&line, &out);
        if (err < 0)
            goto error;

        responses[i].apn = alloca(strlen(out) + 1);
        strcpy(responses[i].apn, out);

        err = at_tok_nextstr(&line, &out);
        if (err < 0)
            goto error;

        responses[i].address = alloca(strlen(out) + 1);
        strcpy(responses[i].address, out);
    }

    at_response_free(p_response);

    if (t != NULL)
        RIL_onRequestComplete(*t, RIL_E_SUCCESS, responses,
            n * sizeof(RIL_PDP_Context_Response));
    else
        RIL_onUnsolicitedResponse(RIL_UNSOL_PDP_CONTEXT_LIST_CHANGED,
            responses,
            n * sizeof(RIL_PDP_Context_Response));

    return;

    error:
    if (t != NULL)
        RIL_onRequestComplete(*t, RIL_E_GENERIC_FAILURE, NULL, 0);
    else
        RIL_onUnsolicitedResponse(RIL_UNSOL_PDP_CONTEXT_LIST_CHANGED,
            NULL, 0);

    at_response_free(p_response);
}


static void requestQueryNetworkSelectionMode(
void *data, size_t datalen, RIL_Token t)
{
    int err;
    ATResponse *p_response = NULL;
    int response = 0;
    char *line;

    err = at_send_command_singleline(AT_CMD, "AT+COPS?", "+COPS:", &p_response);

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

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(int));
    at_response_free(p_response);
    return;
    error:
    at_response_free(p_response);
    LOGE("requestQueryNetworkSelectionMode must never return error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}


static void sendCallStateChanged(void *param)
{
    RIL_onUnsolicitedResponse (
        RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED,
        NULL, 0);
}


static void requestFacilityLock(char **data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    int result;
    char *cmd, *line;

    if (datalen != 4 * sizeof(char *))
        goto error;

    asprintf(&cmd, "AT+CLCK=\"%s\",%c,\"%s\",%s",
        data[0], *data[1], data[2], data[3]);

    err = at_send_command_singleline(AT_CMD, cmd, "+CLCK: ", &p_response);
    free(cmd);

    if (err < 0 || p_response->success == 0)
        goto error;

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &result);
    if (err < 0) goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &result, sizeof(result));
    at_response_free(p_response);
    return;

    error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}


static int forwardFromCCFCLine(char *line, RIL_CallForwardInfo *p_forward)
{
    int err;
    int state;
    int mode;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(p_forward->status));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(p_forward->serviceClass));
    if (err < 0) goto error;

    if (at_tok_hasmore(&line)) {
        err = at_tok_nextstr(&line, &(p_forward->number));

        /* tolerate null here */
        if (err < 0) return 0;

        if (p_forward->number != NULL
            && 0 == strspn(p_forward->number, "+0123456789")
        ) {
            p_forward->number = NULL;
        }

        err = at_tok_nextint(&line, &p_forward->toa);
        if (err < 0) goto error;
    }

    return 0;

    error:
    LOGE("invalid CCFC line\n");
    return -1;
}


static void requestCallForward(RIL_CallForwardInfo *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    ATLine *p_cur;
    int err;
    char *cmd;

    if (datalen != sizeof(*data))
        goto error;

    if (data->status == 2)
        asprintf(&cmd, "AT+CCFC=%d,%d",
            data->reason,
            data->status);
    else
        asprintf(&cmd, "AT+CCFC=%d,%d,\"%s\",%d,%d",
            data->reason,
            data->status,
            data->number ? data->number : "",
        data->toa,
        data->serviceClass);

    err = at_send_command_multiline (AT_CMD, cmd, "+CCFC:", &p_response);
    free(cmd);

    if (err < 0)
        goto error;

    switch (at_get_cme_error(p_response)) {
        case CME_SUCCESS:
        case CME_ERROR_NON_CME:
            break;

        case CME_OPERATION_NOT_ALLOWED:
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            goto done;

        default:
            goto error;
    }

    if (data->status == 2 ) {

        RIL_CallForwardInfo **forwardList, *forwardPool;
        int forwardCount = 0;
        int validCount = 0;
        int i;

        for (p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next, forwardCount++
            );

        forwardList = (RIL_CallForwardInfo **)
            alloca(forwardCount * sizeof(RIL_CallForwardInfo *));

        forwardPool = (RIL_CallForwardInfo *)
            alloca(forwardCount * sizeof(RIL_CallForwardInfo));

        memset (forwardPool, 0, forwardCount * sizeof(RIL_CallForwardInfo));

        /* init the pointer array */
        for(i = 0; i < forwardCount ; i++)
            forwardList[i] = &(forwardPool[i]);

        for (p_cur = p_response->p_intermediates
            ; p_cur != NULL
            ; p_cur = p_cur->p_next
        ) {
            err = forwardFromCCFCLine(p_cur->line, forwardList[validCount]);

            if (err == 0)
                validCount++;
        }

        RIL_onRequestComplete(t, RIL_E_SUCCESS,
            validCount ? forwardList : NULL,
            validCount * sizeof (RIL_CallForwardInfo *));
    } else
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);

    done:
    at_response_free(p_response);
    return;

    error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}


static void requestGetCurrentCalls(void *data, size_t datalen, RIL_Token t)
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
#endif                       /*WORKAROUND_ERRONEOUS_ANSWER*/

    err = at_send_command_multiline (AT_CMD, "AT+CLCC", "+CLCC:", &p_response);

    if (err != 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
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
#endif                   /*WORKAROUND_ERRONEOUS_ANSWER*/

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
                LOGI(
                    "Hit WORKAROUND_ERRONOUS_ANSWER case."
                    " Repoll count: %d\n", s_repollCallsCount);
                s_repollCallsCount++;
                goto error;
            }
        }
    }

    s_expectAnswer = 0;
    s_repollCallsCount = 0;
#endif                       /*WORKAROUND_ERRONEOUS_ANSWER*/

    RIL_onRequestComplete(t, RIL_E_SUCCESS, pp_calls,
        countValidCalls * sizeof (RIL_Call *));

    at_response_free(p_response);

    if (countValidCalls) {
        // We don't seem to get a "NO CARRIER" message from
        // smd, so we're forced to poll until the call ends.
        RIL_requestTimedCallback (sendCallStateChanged, NULL, &TIMEVAL_CALLSTATEPOLL);
    }

    return;
    error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}


static void requestDial(void *data, size_t datalen, RIL_Token t)
{
    RIL_Dial *p_dial;
    char *cmd;
    const char *clir;
    int ret;

    p_dial = (RIL_Dial *)data;

    switch (p_dial->clir) {
                                 /*invocation*/
        case 1: clir = "I"; break;
                                 /*suppression*/
        case 2: clir = "i"; break;
        default:
        case 0: clir = ""; break;/*subscription default*/
    }

    asprintf(&cmd, "ATD%s%s;", p_dial->address, clir);

    ret = at_send_command(AT_CMD, cmd, NULL);

    free(cmd);

    /* success or failure is ignored by the upper layer here.
       it will call GET_CURRENT_CALLS and determine success that way */
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}


static void requestWriteSmsToSim(void *data, size_t datalen, RIL_Token t)
{
    RIL_SMS_WriteArgs *p_args;
    char *cmd;
    int length;
    int err;
    ATResponse *p_response = NULL;

    p_args = (RIL_SMS_WriteArgs *)data;

    length = strlen(p_args->pdu)/2;
    asprintf(&cmd, "AT+CMGW=%d,%d", length, p_args->status);

    err = at_send_command_sms(AT_CMD, cmd, p_args->pdu, "+CMGW:", &p_response);

    if (err != 0 || p_response->success == 0) goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);

    return;
    error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}


static void requestHangup(void *data, size_t datalen, RIL_Token t)
{
    int *p_line;
    int err;
    char *cmd;
    ATResponse *p_response = NULL;

    p_line = (int *)data;

    // 3GPP 22.030 6.5.5
    // "Releases a specific active call X"
    asprintf(&cmd, "AT+CHLD=1%d", p_line[0]);

    err = at_send_command(AT_CMD, cmd, &p_response);
    free(cmd);

    if (err < 0)
        goto error;

    switch (at_get_cme_error(p_response)) {
        case CME_OPERATION_NOT_ALLOWED:
            /* If we haven't connected to anyone yet (ringing)
             * then CHLD is not permitted so just do a hangup.
             */
            err = at_send_command(AT_CMD, "ATH", NULL);
            break;

        default:
            break;
    }

    error:
    at_response_free(p_response);

    /* success or failure is ignored by the upper layer here.
       it will call GET_CURRENT_CALLS and determine success that way */
    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
}


static void requestSignalStrength(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    int err;
    int response[2];
    char *line;

    err = at_send_command_singleline(AT_CMD, "AT+CSQ", "+CSQ:", &p_response);

    if (err < 0 || p_response->success == 0) {
        RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
        goto error;
    }

    line = p_response->p_intermediates->line;

    err = at_tok_start(&line);
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(response[0]));
    if (err < 0) goto error;

    err = at_tok_nextint(&line, &(response[1]));
    if (err < 0) goto error;

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));

    at_response_free(p_response);
    return;

    error:
    LOGE("requestSignalStrength must never return an error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}


static void requestRegistrationState(int request, void *data,
size_t datalen, RIL_Token t)
{
    int err;
    int response[3];
    char * responseStr[3];
    ATResponse *p_response = NULL;
    const char *cmd;
    const char *prefix;
    char *line, *p;
    int resp_len;
    int commas;
    int skip;

    if (request == RIL_REQUEST_REGISTRATION_STATE) {
        cmd = "AT+CREG?";
        prefix = "+CREG:";
    }
    else if (request == RIL_REQUEST_GPRS_REGISTRATION_STATE) {
        cmd = "AT+CGREG?";
        prefix = "+CGREG:";
    }
    else {
        assert(0);
        goto error;
    }

    err = at_send_command_singleline(AT_CMD, cmd, prefix, &p_response);

    if (err != 0 || p_response->success == 0) goto error;

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
     */

    /* count number of commas */
    commas = 0;
    for (p = line ; *p != '\0' ;p++) {
        if (*p == ',') commas++;
    }

    switch (commas) {
        case 0:                  /* +C[G]REG: <stat> */
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            response[1] = -1;
            response[2] = -1;
            break;

        case 1:                  /* +C[G]REG: <n>, <stat> */
            err = at_tok_nextint(&line, &skip);
            if (err < 0) goto error;
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            response[1] = -1;
            response[2] = -1;
            break;

        case 2:                  /* +C[G]REG: <stat>, <lac>, <cid> */
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[1]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[2]);
            if (err < 0) goto error;
            break;

        case 3:                  /* +C[G]REG: <n>, <stat>, <lac>, <cid> */
            err = at_tok_nextint(&line, &skip);
            if (err < 0) goto error;
            err = at_tok_nextint(&line, &response[0]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[1]);
            if (err < 0) goto error;
            err = at_tok_nexthexint(&line, &response[2]);
            if (err < 0) goto error;
            break;

        default:
            goto error;
    }

    asprintf(&responseStr[0], "%d", response[0]);
    asprintf(&responseStr[1], "%d", response[1]);
    asprintf(&responseStr[2], "%d", response[2]);

    RIL_onRequestComplete(t, RIL_E_SUCCESS, responseStr, sizeof(responseStr));
    at_response_free(p_response);

    return;
    error:
    LOGE("requestRegistrationState must never return an error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}


static void requestOperator(void *data, size_t datalen, RIL_Token t)
{
    int err;
    int i;
    int skip;
    ATLine *p_cur;
    char *response[3];

    memset(response, 0, sizeof(response));

    ATResponse *p_response = NULL;

    err = at_send_command_multiline(AT_CMD,
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

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
    at_response_free(p_response);

    return;
    error:
    LOGE("requestOperator must not return error when radio is on");
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}


static void requestNetworkList(void *data, size_t datalen, RIL_Token t)
{
    static char *statStr[] = {
        "unknown",
        "available",
        "current",
        "forbidden"
    };
    int err, count, stat;
    char *line, *tok;
    char **response, **cur;
    ATResponse *p_response = NULL;

    err = at_send_command_singleline(AT_CMD, "AT+COPS=?", "+COPS:", &p_response);

    if (err != 0 || p_response->success == 0) goto error;

    line = p_response->p_intermediates->line;

    for (count = 0; *line; line++)
        if (*line == ')')
            count++;

    response = alloca(count * 4 * sizeof(char *));
    if (!response) goto error;

    line = p_response->p_intermediates->line;
    count = 0;
    cur = response;

    while ((line = strchr(line, '('))) {
        line++;

        err = at_tok_nextint(&line, &stat);
        if (err < 0) continue;

        cur[3] = statStr[stat];

        err = at_tok_nextstr(&line, &(cur[0]));
        if (err < 0) continue;

        err = at_tok_nextstr(&line, &(cur[1]));
        if (err < 0) continue;

        err = at_tok_nextstr(&line, &(cur[2]));
        if (err < 0) continue;

        cur += 4;
        count++;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, count * 4 * sizeof(char *));
    at_response_free(p_response);

    return;
    error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}


static void requestSendSMS(void *data, size_t datalen, RIL_Token t)
{
    int err;
    const char *smsc;
    const char *pdu;
    int tpLayerLength;
    char *cmd1, *cmd2;
    RIL_SMS_Response response;
    ATResponse *p_response = NULL;

    smsc = ((const char **)data)[0];
    pdu = ((const char **)data)[1];

    tpLayerLength = strlen(pdu)/2;

    // "NULL for default SMSC"
    if (smsc == NULL) {
        smsc= "00";
    }

    asprintf(&cmd1, "AT+CMGS=%d", tpLayerLength);
    asprintf(&cmd2, "%s%s", smsc, pdu);

    err = at_send_command_sms(AT_CMD, cmd1, cmd2, "+CMGS:", &p_response);

    if (err != 0 || p_response->success == 0) goto error;

    memset(&response, 0, sizeof(response));

    /* FIXME fill in messageRef and ackPDU */

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &response, sizeof(response));
    at_response_free(p_response);

    return;
    error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}

#define WRITE_PPP_OPTION(option) write(fd, option, strlen(option))

static void writePppOptions (int channel, const char *tty_path, const char *username, const char *password)
{
    char *line;
    int fd = creat (PPP_OPTION_FILE, 0644);
    if (fd < 0) {
        LOGE ("Error creating PPP options file: " PPP_OPTION_FILE ".\n");
        return;
    }
    asprintf(&line, "%s\n", tty_path);
    WRITE_PPP_OPTION(line);
    free(line);

    WRITE_PPP_OPTION("crtscts\n");

    if (username) {
        asprintf(&line, "user %s\n", username);
        WRITE_PPP_OPTION(line);
        free(line);
    }

    if (password) {
        asprintf(&line, "password %s\n", password);
        WRITE_PPP_OPTION(line);
        free(line);
    }

    asprintf(&line, "unit %d\n", channel - AT_DATA);
    WRITE_PPP_OPTION(line);
    free(line);

    WRITE_PPP_OPTION("noauth\n");
    WRITE_PPP_OPTION("nodetach\n");
    WRITE_PPP_OPTION("default-asyncmap\n");
    WRITE_PPP_OPTION("ipcp-accept-local\n");
    WRITE_PPP_OPTION("ipcp-accept-remote\n");
    WRITE_PPP_OPTION("usepeerdns\n");
    WRITE_PPP_OPTION("defaultroute\n");

    close(fd);
}


static void requestSetupDefaultPDP(void *data, size_t datalen, RIL_Token t)
{
    const char *apn = ((const char **)data)[0];
    const char *username = ((const char **)data)[1];
    const char *password = ((const char **)data)[2];
    ATResponse *p_response = NULL;
    char *cmd;
    int err;

    char tty_path[64];
    err = at_pseudo (AT_DATA, tty_path, sizeof(tty_path));

    if (err < 0) {
        LOGD("Error connecting a GPRS channel!\n");
        goto error;
    }

    LOGD("Connecting GPRS channel to tty %s, APN '%s'\n", tty_path, apn);
    writePppOptions (AT_DATA, tty_path, username, password);

    asprintf(&cmd, "AT+CGDCONT=1,\"IP\",\"%s\"", apn);
    //FIXME check for error here
    err = at_send_command(AT_DATA, cmd, NULL);
    free(cmd);

    // Set required QoS params to default
    err = at_send_command(AT_DATA, "AT+CGQREQ=1", NULL);

    // Set minimum QoS params to default
    err = at_send_command(AT_DATA, "AT+CGQMIN=1", NULL);

    // packet-domain event reporting
    err = at_send_command(AT_DATA, "AT+CGEREP=1,0", NULL);

    // Start data
    err = at_send_command(AT_DATA, "ATDT*99***1#", &p_response);
    if (err < 0 || p_response->success == 0) goto error;

    char *response[2] = { "1", "gprs" };

    RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
    at_response_free(p_response);

    return;

error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
}


static void requestSMSAcknowledge(void *data, size_t datalen, RIL_Token t)
{
    int ackSuccess;
    int err;

    ackSuccess = ((int *)data)[0];

    if (ackSuccess == 1) {
        err = at_send_command(AT_CMD, "AT+CNMA=1", NULL);
    }
    else if (ackSuccess == 0) {
        err = at_send_command(AT_CMD, "AT+CNMA=2", NULL);
    }
    else {
        LOGE("unsupported arg to RIL_REQUEST_SMS_ACKNOWLEDGE\n");
        goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    return;

    error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
}


static void  requestSIM_IO(void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response = NULL;
    RIL_SIM_IO_Response sr;
    int err;
    char *cmd = NULL;
    RIL_SIM_IO *p_args;
    char *line;

    memset(&sr, 0, sizeof(sr));

    p_args = (RIL_SIM_IO *)data;

    /* FIXME handle pin2 */

    if (p_args->data == NULL) {
        asprintf(&cmd, "AT+CRSM=%d,%d,%d,%d,%d",
            p_args->command, p_args->fileid,
            p_args->p1, p_args->p2, p_args->p3);
    }
    else {
        asprintf(&cmd, "AT+CRSM=%d,%d,%d,%d,%d,%s",
            p_args->command, p_args->fileid,
            p_args->p1, p_args->p2, p_args->p3, p_args->data);
    }

    err = at_send_command_singleline(AT_CMD, cmd, "+CRSM:", &p_response);

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

    RIL_onRequestComplete(t, RIL_E_SUCCESS, &sr, sizeof(sr));
    at_response_free(p_response);
    free(cmd);

    return;
    error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
    free(cmd);

}


static void  requestEnterSimPin(void*  data, size_t  datalen, RIL_Token  t)
{
    ATResponse   *p_response = NULL;
    int           err;
    char*         cmd = NULL;
    const char**  strings = (const char**)data;;

    if ( datalen == sizeof(char*) ) {
        asprintf(&cmd, "AT+CPIN=\"%s\"", strings[0]);
    }
    else if ( datalen == 2*sizeof(char*) ) {
        asprintf(&cmd, "AT+CPIN=\"%s\",\"%s\"", strings[0], strings[1]);
    } else
    goto error;

    err = at_send_command_singleline(AT_CMD, cmd, "+CPIN:", &p_response);
    free(cmd);

    if (err < 0 || p_response->success == 0) {
        error:
        RIL_onRequestComplete(t, RIL_E_PASSWORD_INCORRECT, NULL, 0);
    }
    else {
        RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    }
    at_response_free(p_response);
}


static void  requestSendUSSD(void *data, size_t datalen, RIL_Token t)
{
    const char *ussdRequest;
    char *cmd;
    int err;
    ATResponse *p_response = NULL;

    ussdRequest = (char *)(data);

    asprintf(&cmd, "AT+CUSD=1,\"%s\",15", ussdRequest);

    err = at_send_command(AT_CMD, cmd, &p_response);
    free(cmd);

    if (err < 0)
        goto error;

    switch (at_get_cme_error(p_response)) {
        case CME_SUCCESS:
        case CME_ERROR_NON_CME:
            break;

        default:
            goto error;
    }

    RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
    at_response_free(p_response);
    return;

    error:
    RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
    at_response_free(p_response);
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
onRequest (int request, void *data, size_t datalen, RIL_Token t)
{
    ATResponse *p_response;
    int err;

    LOGD("onRequest: %s", requestToString(request));

    /* Ignore all requests except RIL_REQUEST_GET_SIM_STATUS
     * when RADIO_STATE_UNAVAILABLE.
     */
    if (sState == RADIO_STATE_UNAVAILABLE
        && request != RIL_REQUEST_GET_SIM_STATUS
    ) {
        RIL_onRequestComplete(t, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
        return;
    }

    /* Ignore all non-power requests when RADIO_STATE_OFF
     * (except RIL_REQUEST_GET_SIM_STATUS)
     */
    if (sState == RADIO_STATE_OFF
        && !(request == RIL_REQUEST_RADIO_POWER
        || request == RIL_REQUEST_GET_SIM_STATUS)
    ) {
        RIL_onRequestComplete(t, RIL_E_RADIO_NOT_AVAILABLE, NULL, 0);
        return;
    }

    switch (request) {
        case RIL_REQUEST_BASEBAND_VERSION:
        {
            p_response = NULL;
            err = at_send_command_numeric(AT_CMD, "AT%BAND?", "%BAND: ", &p_response);

            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            }
            else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS,
                    p_response->p_intermediates->line, sizeof(char *));
            }
            at_response_free(p_response);
            break;
        }
        case RIL_REQUEST_QUERY_FACILITY_LOCK:
        {
            char *lockData[4];
            lockData[0] = ((char **)data)[0];
            lockData[1] = FACILITY_LOCK_REQUEST;
            lockData[2] = ((char **)data)[1];
            lockData[3] = ((char **)data)[2];
            requestFacilityLock(lockData, datalen + sizeof(char *), t);
            break;
        }
        case RIL_REQUEST_SET_FACILITY_LOCK:
        {
            requestFacilityLock(data, datalen, t);
            break;
        }
        case RIL_REQUEST_QUERY_CALL_FORWARD_STATUS:
        {
            RIL_CallForwardInfo request;
            request.reason = ((int *)data)[0];
            request.status = 2;
            requestCallForward(&request, sizeof(request), t);
            break;
        }
        case RIL_REQUEST_SET_CALL_FORWARD:
        {
            requestCallForward(data, datalen, t);
            break;
        }
        case RIL_REQUEST_GET_SIM_STATUS:
        {
            int simStatus;

            simStatus = getSIMStatus();

            RIL_onRequestComplete(t, RIL_E_SUCCESS, &simStatus, sizeof(simStatus));
            break;
        }
        case RIL_REQUEST_GET_CURRENT_CALLS:
            requestGetCurrentCalls(data, datalen, t);
            break;
        case RIL_REQUEST_DIAL:
            requestDial(data, datalen, t);
            break;
        case RIL_REQUEST_HANGUP:
            requestHangup(data, datalen, t);
            break;
        case RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND:
            // 3GPP 22.030 6.5.5
            // "Releases all held calls or sets User Determined User Busy
            //  (UDUB) for a waiting call."
            at_send_command(AT_CMD, "AT+CHLD=0", NULL);

            /* success or failure is ignored by the upper layer here.
               it will call GET_CURRENT_CALLS and determine success that way */
            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            break;
        case RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND:
            // 3GPP 22.030 6.5.5
            // "Releases all active calls (if any exist) and accepts
            //  the other (held or waiting) call."
            at_send_command(AT_CMD, "AT+CHLD=1", NULL);

            /* success or failure is ignored by the upper layer here.
               it will call GET_CURRENT_CALLS and determine success that way */
            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            break;
        case RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE:
            // 3GPP 22.030 6.5.5
            // "Places all active calls (if any exist) on hold and accepts
            //  the other (held or waiting) call."
            at_send_command(AT_CMD, "AT+CHLD=2", NULL);

#ifdef WORKAROUND_ERRONEOUS_ANSWER
            s_expectAnswer = 1;
#endif                   /* WORKAROUND_ERRONEOUS_ANSWER */

            /* success or failure is ignored by the upper layer here.
               it will call GET_CURRENT_CALLS and determine success that way */
            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            break;
        case RIL_REQUEST_ANSWER:
            at_send_command(AT_CMD, "ATA", NULL);

#ifdef WORKAROUND_ERRONEOUS_ANSWER
            s_expectAnswer = 1;
#endif                   /* WORKAROUND_ERRONEOUS_ANSWER */

            /* success or failure is ignored by the upper layer here.
               it will call GET_CURRENT_CALLS and determine success that way */
            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            break;
        case RIL_REQUEST_CONFERENCE:
            // 3GPP 22.030 6.5.5
            // "Adds a held call to the conversation"
            at_send_command(AT_CMD, "AT+CHLD=3", NULL);

            /* success or failure is ignored by the upper layer here.
               it will call GET_CURRENT_CALLS and determine success that way */
            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            break;
        case RIL_REQUEST_UDUB:
            /* user determined user busy */
            /* sometimes used: ATH */
            at_send_command(AT_CMD, "ATH", NULL);

            /* success or failure is ignored by the upper layer here.
               it will call GET_CURRENT_CALLS and determine success that way */
            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
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
                at_send_command(AT_CMD, cmd, NULL);
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            }
            else {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            }
        }
        break;

        case RIL_REQUEST_SIGNAL_STRENGTH:
            requestSignalStrength(data, datalen, t);
            break;
        case RIL_REQUEST_REGISTRATION_STATE:
        case RIL_REQUEST_GPRS_REGISTRATION_STATE:
            requestRegistrationState(request, data, datalen, t);
            break;
        case RIL_REQUEST_OPERATOR:
            requestOperator(data, datalen, t);
            break;
        case RIL_REQUEST_RADIO_POWER:
            requestRadioPower(data, datalen, t);
            break;
        case RIL_REQUEST_DTMF:
        {
            char c = ((char *)data)[0];
            char *cmd;
            asprintf(&cmd, "AT+VTS=%c", (int)c);
            at_send_command(AT_CMD, cmd, NULL);
            free(cmd);
            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            break;
        }
        case RIL_REQUEST_SEND_SMS:
            requestSendSMS(data, datalen, t);
            break;
        case RIL_REQUEST_SETUP_DEFAULT_PDP:
            requestSetupDefaultPDP(data, datalen, t);
            break;
        case RIL_REQUEST_SMS_ACKNOWLEDGE:
            requestSMSAcknowledge(data, datalen, t);
            break;

        case RIL_REQUEST_GET_IMSI:
            p_response = NULL;
            err = at_send_command_numeric(AT_CMD, "AT+CIMI", "", &p_response);

            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            }
            else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS,
                    p_response->p_intermediates->line, sizeof(char *));
            }
            at_response_free(p_response);
            break;

        case RIL_REQUEST_GET_IMEI:
            p_response = NULL;
            err = at_send_command_numeric(AT_CMD, "AT+CGSN", "+CGSN: ", &p_response);

            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            }
            else {
                int n = (*p_response->p_intermediates->line == '+') ? 7 : 0;
                RIL_onRequestComplete(t, RIL_E_SUCCESS,
                    p_response->p_intermediates->line + n, sizeof(char *));
            }
            at_response_free(p_response);
            break;

        case RIL_REQUEST_GET_CLIR:
        {
            p_response = NULL;
            int response[2] = {1, 1};
            err = at_send_command_singleline(AT_CMD, "AT+CLIR?", "+CLIR: ", &p_response);

            if (err >= 0 && p_response->success) {
                char *line = p_response->p_intermediates->line;

                err = at_tok_start(&line);

                if (err >= 0) {
                    err = at_tok_nextint(&line, &response[0]);

                    if (err >= 0)
                        err = at_tok_nextint(&line, &response[1]);
                }
            }

            RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
            at_response_free(p_response);
            break;
        }
        case RIL_REQUEST_SET_CLIR:
        {
            int n = ((int *)data)[0];
            char *cmd;
            asprintf(&cmd, "AT+CLIR=%d", n);
            at_send_command(AT_CMD, cmd, NULL);
            free(cmd);
            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            break;
        }
        case RIL_REQUEST_QUERY_CALL_WAITING:
        {
            p_response = NULL;
            int response[2] = {0, 0};
            int c = ((int *)data)[0];
            char *cmd;
            asprintf(&cmd, "AT+CCWA=1,2,%d", c);
            err = at_send_command_singleline(AT_CMD, cmd, "+CCWA: ", &p_response);
            free(cmd);

            if (err >= 0 && p_response->success) {
                char *line = p_response->p_intermediates->line;

                err = at_tok_start(&line);

                if (err >= 0) {
                    err = at_tok_nextint(&line, &response[0]);

                    if (err >= 0)
                        err = at_tok_nextint(&line, &response[1]);
                }
            }

            RIL_onRequestComplete(t, RIL_E_SUCCESS, response, sizeof(response));
            at_response_free(p_response);
            break;
        }
        case RIL_REQUEST_SCREEN_STATE:
            screen_on = ((int *)data)[0];

            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            break;

        case RIL_REQUEST_SIM_IO:
            requestSIM_IO(data,datalen,t);
            break;

        case RIL_REQUEST_SEND_USSD:
            requestSendUSSD(data, datalen, t);
            break;

        case RIL_REQUEST_CANCEL_USSD:
            p_response = NULL;
            err = at_send_command_numeric(AT_CMD, "AT+CUSD=2", "", &p_response);

            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            }
            else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS,
                    p_response->p_intermediates->line, sizeof(char *));
            }
            at_response_free(p_response);
            break;

        case RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE:
        {
            int gsmType = 1;
            RIL_onRequestComplete(t, RIL_E_SUCCESS,
                &gsmType, sizeof(int *));
        }
        break;

        case RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC:
            at_send_command(AT_CMD, "AT+CGATT=1", NULL);
            at_send_command(AT_CMD, "AT+COPS=0", NULL);
            at_send_command(AT_CMD, "AT+CGAUTO", NULL);
            RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
            break;

        case RIL_REQUEST_QUERY_AVAILABLE_NETWORKS:
            requestNetworkList(data, datalen, t);
            break;

        case RIL_REQUEST_PDP_CONTEXT_LIST:
            requestPDPContextList(data, datalen, t);
            break;

        case RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE:
            requestQueryNetworkSelectionMode(data, datalen, t);
            break;

        case RIL_REQUEST_OEM_HOOK_RAW:
            // echo back data
            RIL_onRequestComplete(t, RIL_E_SUCCESS, data, datalen);
            break;

        case RIL_REQUEST_OEM_HOOK_STRINGS:
        {
            int i;
            const char ** cur;

            LOGD("got OEM_HOOK_STRINGS: 0x%8p %lu", data, (long)datalen);

            for (i = (datalen / sizeof (char *)), cur = (const char **)data ;
            i > 0 ; cur++, i --) {
                LOGD("> '%s'", *cur);
            }

            // echo back strings
            RIL_onRequestComplete(t, RIL_E_SUCCESS, data, datalen);
            break;
        }

        case RIL_REQUEST_WRITE_SMS_TO_SIM:
            requestWriteSmsToSim(data, datalen, t);
            break;

        case RIL_REQUEST_DELETE_SMS_ON_SIM:
        {
            char * cmd;
            p_response = NULL;
            asprintf(&cmd, "AT+CMGD=%d", ((int *)data)[0]);
            err = at_send_command(AT_CMD, cmd, &p_response);
            free(cmd);
            if (err < 0 || p_response->success == 0) {
                RIL_onRequestComplete(t, RIL_E_GENERIC_FAILURE, NULL, 0);
            }
            else {
                RIL_onRequestComplete(t, RIL_E_SUCCESS, NULL, 0);
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
            requestEnterSimPin(data, datalen, t);
            break;

        default:
            RIL_onRequestComplete(t, RIL_E_REQUEST_NOT_SUPPORTED, NULL, 0);
            break;
    }
}


/**
 * Synchronous call from the RIL to us to return current radio state.
 * RADIO_STATE_UNAVAILABLE should be the initial state.
 */
static RIL_RadioState
currentState()
{
    return sState;
}


/**
 * Call from RIL to us to find out whether a specific request code
 * is supported by this implementation.
 *
 * Return 1 for "supported" and 0 for "unsupported"
 */

static int
onSupports (int requestCode)
{
    //@@@ todo

    return 1;
}


static void onCancel (RIL_Token t)
{
    //@@@todo

}


static const char * getVersion(void)
{
    return "android muxgsm-ril 1.0";
}


static void
setRadioState(RIL_RadioState newState)
{
    RIL_RadioState oldState;

    pthread_mutex_lock(&s_state_mutex);

    oldState = sState;

    if (s_closed > 0) {
        // If we're closed, the only reasonable state is
        // RADIO_STATE_UNAVAILABLE
        // This is here because things on the main thread
        // may attempt to change the radio state after the closed
        // event happened in another thread
        newState = RADIO_STATE_UNAVAILABLE;
    }

    if (sState != newState || s_closed > 0) {
        sState = newState;

        pthread_cond_broadcast (&s_state_cond);
    }

    pthread_mutex_unlock(&s_state_mutex);

    /* do these outside of the mutex */
    if (sState != oldState) {
        RIL_onUnsolicitedResponse (RIL_UNSOL_RESPONSE_RADIO_STATE_CHANGED,
            NULL, 0);

        /* FIXME onSimReady() and onRadioPowerOn() cannot be called
         * from the AT reader thread
         * Currently, this doesn't happen, but if that changes then these
         * will need to be dispatched on the request thread
         */
        if (sState == RADIO_STATE_SIM_READY) {
            onSIMReady();
        }
        else if (sState == RADIO_STATE_SIM_NOT_READY) {
            onRadioPowerOn();
        }
    }
}


/** returns one of RIM_SIM_*. Returns RIL_SIM_NOT_READY on error */
static int
getSIMStatus()
{
    ATResponse *p_response = NULL;
    char *cpinLine = NULL;
    char *cpinResult = NULL;
    int ret = RIL_SIM_NOT_READY;
    int err;

    if (sState == RADIO_STATE_OFF || sState == RADIO_STATE_UNAVAILABLE) {
        goto done;
    }

    err = at_send_command_singleline(AT_CMD, "AT+CPIN?", "+CPIN:", &p_response);

    if (err != 0) {
        goto done;
    }

    switch (at_get_cme_error(p_response)) {
        case CME_SUCCESS:
        case CME_ERROR_NON_CME:
            break;

        case CME_SIM_NOT_INSERTED:
            ret = RIL_SIM_ABSENT;
            goto done;

        default:
            goto done;
    }

    /* CPIN? has succeeded and phonebook is ready, now look at the result */

    cpinLine = p_response->p_intermediates->line;
    err = at_tok_start (&cpinLine);

    if (err < 0) {
        goto done;
    }

    err = at_tok_nextstr(&cpinLine, &cpinResult);

    if (err < 0) {
        goto done;
    }

    if (0 == strcmp (cpinResult, "SIM PIN")) {
        ret = RIL_SIM_PIN;
        goto done;
    }
    else if (0 == strcmp (cpinResult, "SIM PUK")) {
        ret = RIL_SIM_PUK;
        goto done;
    }
    else if (0 == strcmp (cpinResult, "PH-NET PIN")) {
        ret = RIL_SIM_NETWORK_PERSONALIZATION;
        goto done;
    }
    else if (0 != strcmp (cpinResult, "READY")) {
        /* we're treating unsupported lock types as "sim absent" */
        ret = RIL_SIM_ABSENT;
        goto done;
    }

    simFlags |= SIM_CPIN_READY;

    /* Only when the phonebook is ready is the SIM truely ready */
    if (simFlags & SIM_PHONEBOOK_READY) {
        ret = RIL_SIM_READY;
    }

done:
    at_response_free(p_response);
    return ret;
}


/**
 * SIM ready means any commands that access the SIM will work, including:
 *  AT+CPIN, AT+CSMS, AT+CNMI, AT+CRSM
 *  (all SMS-related commands)
 */

static void pollSIMState (void *param)
{
    if (sState != RADIO_STATE_SIM_NOT_READY) {
        // no longer valid to poll
        return;
    }

    if (simFlags == SIM_CPIN_READY) {
        /* CPIN command says ready, but phonebook isn't ready yet */
        RIL_requestTimedCallback (pollSIMState, NULL, &TIMEVAL_SIMPOLL);
        return;
    }

    switch(getSIMStatus()) {
        case RIL_SIM_ABSENT:
        case RIL_SIM_PIN:
        case RIL_SIM_PUK:
        case RIL_SIM_NETWORK_PERSONALIZATION:
        default:
            setRadioState(RADIO_STATE_SIM_LOCKED_OR_ABSENT);
            return;

        case RIL_SIM_NOT_READY:
            RIL_requestTimedCallback (pollSIMState, NULL, &TIMEVAL_SIMPOLL);
            return;

        case RIL_SIM_READY:
            setRadioState(RADIO_STATE_SIM_READY);
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

    err = at_send_command_singleline(AT_CMD, "AT+CFUN?", "+CFUN:", &p_response);

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

    simFlags = 0;
    setRadioState (RADIO_STATE_OFF);

    at_channel(AT_CMD);
    at_channel(AT_DATA);

    /* note: we don't check errors here. Everything important will
       be handled in onATTimeout and onATReaderClosed */

    at_send_command(AT_CMD, "ATE0Q0V1", NULL);
    at_send_command(AT_DATA, "ATE0Q0V1", NULL);

    /* use +CRING instead of RING */
    at_send_command(AT_CMD, "AT+CRC=1", NULL);

    /*  No auto-answer */
    at_send_command(AT_CMD, "ATS0=0", NULL);

    /*  Extended errors */
    at_send_command(AT_CMD, "AT+CMEE=1", NULL);
    at_send_command(AT_DATA, "AT+CMEE=1", NULL);

    /*  Network registration events */
    err = at_send_command(AT_CMD, "AT+CREG=2", &p_response);

    /* some handsets -- in tethered mode -- don't support CREG=2 */
    if (err < 0 || p_response->success == 0) {
        at_send_command(AT_CMD, "AT+CREG=1", NULL);
    }

    at_response_free(p_response);

    /*  GPRS registration events */
    at_send_command(AT_CMD, "AT+CGREG=2", NULL);

    /*  Call Waiting notifications */
    at_send_command(AT_CMD, "AT+CCWA=1", NULL);

    /*  Alternating voice/data on */
    at_send_command(AT_CMD, "AT+CMOD=2", NULL);

    /*  Not muted */
    at_send_command(AT_CMD, "AT+CMUT=0", NULL);

    /*  +CSSU unsolicited supp service notifications */
    at_send_command(AT_CMD, "AT+CSSN=0,1", NULL);

    /*  no connected line identification */
    at_send_command(AT_CMD, "AT+COLP=0", NULL);

    /*  HEX character set */
    at_send_command(AT_CMD, "AT+CSCS=\"HEX\"", NULL);

    /*  USSD unsolicited */
    at_send_command(AT_CMD, "AT+CUSD=1", NULL);

    /*  Enable +CGEV GPRS event notifications, but don't buffer */
    at_send_command(AT_CMD, "AT+CGEREP=1,0", NULL);

    /*  SMS PDU mode */
    at_send_command(AT_CMD, "AT+CMGF=0", NULL);

    /*  notifications when SMS is ready (currently ignored) */
    at_send_command(AT_CMD, "AT%CSTAT=1", NULL);

    /*  never go into deep sleep */
    at_send_command(AT_CMD, "AT%SLEEP=2", NULL);

    /* assume radio is off on error */
    if (isRadioOn() > 0) {
        setRadioState (RADIO_STATE_SIM_NOT_READY);
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


/**
 * Called by atchannel when an unsolicited line appears
 * This is called on atchannel's reader thread. AT commands may
 * not be issued here
 */
static void onUnsolicited (int channel, const char *s, const char *sms_pdu)
{
    char *line = NULL;
    int err;

    if (strStartsWith(s, "%CSTAT: PHB")) {
        int phbReady;
        char *response;
        line = strdup(s);
        at_tok_start(&line);
        err = at_tok_nextstr(&line, &response);
        err = at_tok_nextint(&line, &phbReady);
        free(line);
        if (phbReady)
            simFlags |= SIM_PHONEBOOK_READY;
        else
            simFlags &= ~SIM_PHONEBOOK_READY;
        return;
    }

    /* Ignore other unsolicited responses until we're initialized.
     * This is OK because the RIL library will poll for initial state
     */
    if (sState == RADIO_STATE_UNAVAILABLE) {
        return;
    }

    if (strStartsWith(s, "%CTZV:")) {
        /* TI specific -- NITZ time */
        char *response;

        line = strdup(s);
        at_tok_start(&line);

        err = at_tok_nextstr(&line, &response);

        if (err != 0) {
            LOGE("invalid NITZ line %s\n", s);
        }
        else {
            RIL_onUnsolicitedResponse (
                RIL_UNSOL_NITZ_TIME_RECEIVED,
                response, strlen(response));
        }
        free(line);
    } else if (strStartsWith(s,"+CRING:")
        || strStartsWith(s,"RING")
    ) {
        RIL_onUnsolicitedResponse (
            RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED,
            NULL, 0);
        RIL_onUnsolicitedResponse (
            RIL_UNSOL_CALL_RING,
            NULL, 0);
    } else if (strStartsWith(s,"NO CARRIER")
        || strStartsWith(s,"+CCWA")
    ) {
        RIL_onUnsolicitedResponse (
            RIL_UNSOL_RESPONSE_CALL_STATE_CHANGED,
            NULL, 0);
#ifdef WORKAROUND_FAKE_CGEV
        RIL_requestTimedCallback (onPDPContextListChanged, NULL, NULL);
#endif                   /* WORKAROUND_FAKE_CGEV */
    } else if (strStartsWith(s,"+CREG:")
        || strStartsWith(s,"+CGREG:")
    ) {
        RIL_onUnsolicitedResponse (
            RIL_UNSOL_RESPONSE_NETWORK_STATE_CHANGED,
            NULL, 0);
#ifdef WORKAROUND_FAKE_CGEV
        RIL_requestTimedCallback (onPDPContextListChanged, NULL, NULL);
#endif                   /* WORKAROUND_FAKE_CGEV */
    }
    else if (strStartsWith(s, "+CMT:")) {
        RIL_onUnsolicitedResponse (
            RIL_UNSOL_RESPONSE_NEW_SMS,
            sms_pdu, strlen(sms_pdu));
    }
    else if (strStartsWith(s, "+CDS:")) {
        RIL_onUnsolicitedResponse (
            RIL_UNSOL_RESPONSE_NEW_SMS_STATUS_REPORT,
            sms_pdu, strlen(sms_pdu));
    }
    else if (strStartsWith(s, "+CGEV:")) {
        /* Really, we can ignore NW CLASS and ME CLASS events here,
         * but right now we don't since extranous
         * RIL_UNSOL_PDP_CONTEXT_LIST_CHANGED calls are tolerated
         */
        /* can't issue AT commands here -- call on main thread */
        RIL_requestTimedCallback (onPDPContextListChanged, NULL, NULL);
#ifdef WORKAROUND_FAKE_CGEV
    }
    else if (strStartsWith(s, "+CME ERROR: 150")) {
        RIL_requestTimedCallback (onPDPContextListChanged, NULL, NULL);
#endif                   /* WORKAROUND_FAKE_CGEV */
    }
}


/* Called on command or reader thread */
static void onATReaderClosed()
{
    LOGI("AT channel closed\n");
    at_close();
    s_closed = 1;

    setRadioState (RADIO_STATE_UNAVAILABLE);
}


/* Called on command thread */
static void onATTimeout()
{
    LOGI("AT channel timeout; closing\n");
    at_close();

    s_closed = 1;

    /* FIXME cause a radio reset here */

    setRadioState (RADIO_STATE_UNAVAILABLE);
}


static void usage(char *s)
{
    fprintf(stderr, "muxgsm-ril requires: -p <tcp port> or -d /dev/tty_device\n");
}


static void *
mainLoop(void *param)
{
    int ret;
    at_set_on_reader_closed(onATReaderClosed);
    at_set_on_timeout(onATTimeout);

    for (;;) {
        sFD = -1;
        while  (sFD < 0) {
            if (s_port > 0) {
                sFD = socket_loopback_client(s_port, SOCK_STREAM);
            }
            else if (s_device_socket) {
                sFD = socket_local_client( s_device_path,
                    ANDROID_SOCKET_NAMESPACE_FILESYSTEM,
                    SOCK_STREAM );
            }
            else if (s_device_path != NULL) {
                char *dev_name = strtok( strdup(s_device_path), "," );
                char *baud_str = strtok( NULL, "," );

                baudRate = (baud_str && baud_str[0]) ? atoi( baud_str ) : 115200;

                sFD = open (dev_name, O_RDWR);

                if (sFD >= 0 && isatty(sFD))
                    set_term_params(sFD, baudRate, 1);

                free (dev_name);
            }

            if (sFD < 0) {
                perror ("opening AT interface. retrying...");
                sleep(10);
                /* never returns */
            }
        }

        s_closed = 0;
        ret = at_open(sFD, baudRate, onUnsolicited);

        if (ret < 0) {
            LOGE ("AT error %d on at_open\n", ret);
            return 0;
        }

        RIL_requestTimedCallback(initializeCallback, NULL, &TIMEVAL_0);

        // Give initializeCallback a chance to dispatched, since
        // we don't presently have a cancellation mechanism
        sleep(1);

        waitForClose();
        LOGI("Re-opening after close");
    }
}


pthread_t s_tid_mainloop;

const RIL_RadioFunctions *RIL_Init(const struct RIL_Env *env, int argc, char **argv)
{
    int ret;
    int opt;
    pthread_attr_t attr;

    s_rilenv = env;

    while ( -1 != (opt = getopt(argc, argv, "p:d:s:"))) {
        switch (opt) {
            case 'p':
                s_port = atoi(optarg);
                if (s_port == 0) {
                    usage(argv[0]);
                    return NULL;
                }
                LOGI("Opening loopback port %d\n", s_port);
                break;

            case 'd':
                s_device_path = optarg;
                LOGI("Opening tty device %s\n", s_device_path);
                break;

            case 's':
                s_device_path   = optarg;
                s_device_socket = 1;
                LOGI("Opening socket %s\n", s_device_path);
                break;

            default:
                usage(argv[0]);
                return NULL;
        }
    }

    if (s_port < 0 && s_device_path == NULL) {
        usage(argv[0]);
        return NULL;
    }

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&s_tid_mainloop, &attr, mainLoop, NULL);

    return &s_callbacks;
}
