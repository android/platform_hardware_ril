/* //hardware/ril/muxgsm-ril/atchannel.c
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

#include "atchannel.h"
#include "at_tok.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <alloca.h>
#include <poll.h>
#include <termios.h>

#define LOG_NDEBUG 0
#define LOG_TAG "AT"
#include <utils/Log.h>

#include "misc.h"

#define NUM_ELEMS(x) (sizeof(x) / sizeof(x[0]))

#define GSM_MAX_FRAME_SIZE          128
#define MAX_AT_RESPONSE             (64 * GSM_MAX_FRAME_SIZE)
#define COMMAND_RETRY_COUNT         5
#define COMMAND_TIMEOUT_MSEC        2000
#define LONG_COMMAND_TIMEOUT_MSEC   5000
#define AT_RETRY_COUNT              3
#define AT_TIMEOUT_MSEC             250

#define FRAME_ADV_FLAG      0x7E

#define FRAME_ADV_ESC       0x7D
#define FRAME_ADV_ESC_COPML 0x20

#define GSM_EA              0x01
#define GSM_CR              0x02
#define GSM_PF              0x10

#define GSM_TYPE_UI         0x03 // Un-numbered Information
#define GSM_TYPE_DM         0x0F // Disconnected Mode
#define GSM_TYPE_SABM       0x2F // Set Asynchronous Balanced Mode
#define GSM_TYPE_DISC       0x43 // DISConnect
#define GSM_TYPE_UA         0x63 // Un-numbered Acknowledgement
#define GSM_TYPE_UIH        0xEF // Un-numbered Information w/Header

#define GSM_CONTROL_NSC     0x11 // Non Supported Command
#define GSM_CONTROL_TEST    0x21 // TEST
#define GSM_CONTROL_PSC     0x41 // Power Saving Control
#define GSM_CONTROL_RLS     0x51 // Remote Line Status
#define GSM_CONTROL_PN      0x81 // Parameter Negotiation
#define GSM_CONTROL_RPN     0x91 // Remote Port Negotiation
#define GSM_CONTROL_CLD     0xC1 // CLose Down
#define GSM_CONTROL_SNC     0xD1 // Service Negotiation Command
#define GSM_CONTROL_MSC     0xE1 // Modem Status

#define GSM_SIGNAL_FC       0x02
#define GSM_SIGNAL_RTC      0x04
#define GSM_SIGNAL_IC       0x40
#define GSM_SIGNAL_DV       0x80

#define GSM_FRAME_IS(type, f) ((f & ~GSM_PF) == type)

#define GSM_WRITE_RETRIES   5
#define GSM_MAX_CHANNELS    32

static pthread_t s_tid_reader;
static int s_fd = -1;            /* fd of the AT channel */
static ATUnsolHandler s_unsolHandler;

/*
 * for current pending command
 * these are protected by mutex
 */

static struct channelData
{
    int pipeFd;
    int framesOK;
    int opened;
    pthread_t pipeTid;
    pthread_cond_t cond;
    ATCommandType type;
    const char *responsePrefix;
    const char *smsPDU;
    char *cmdSMS;
    char *txtSMS;
    size_t lenSMS;
    ATResponse *p_response;
} s_chanData[GSM_MAX_CHANNELS];

static pthread_mutex_t at_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mux_mutex = PTHREAD_MUTEX_INITIALIZER;

static void (*s_onTimeout)(void);
static void (*s_onReaderClosed)(void);
static int s_readerClosed;

static void onReaderClosed();

static void sleepMsec(long long msec)
{
    struct timespec ts;
    int err;

    ts.tv_sec = (msec / 1000);
    ts.tv_nsec = (msec % 1000) * 1000 * 1000;

    do {
        err = nanosleep (&ts, &ts);
    } while (err < 0 && errno == EINTR);
}


/**
 * returns 1 if line is a final response indicating error
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static const char * s_finalResponsesError[] =
{
    "ERROR",
    "+CMS ERROR:",
    "+CME ERROR:",
    "+EXT ERROR:",
    "NO CARRIER",                /* sometimes! */
    "NO ANSWER",
    "NO DIALTONE",
};

static int isFinalResponseError(const char *line)
{
    size_t i;

    for (i = 0 ; i < NUM_ELEMS(s_finalResponsesError) ; i++)
        if (strStartsWith(line, s_finalResponsesError[i]))
            return 1;

    return 0;
}


/**
 * returns 1 if line is a final response indicating success
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static const char * s_finalResponsesSuccess[] =
{
    "OK",
    "AT-Command Interpreter ready",
    "CONNECT"                    /* some stacks start up data on another channel */
};

static int isFinalResponseSuccess(const char *line)
{
    size_t i;

    for (i = 0 ; i < NUM_ELEMS(s_finalResponsesSuccess) ; i++)
        if (strStartsWith(line, s_finalResponsesSuccess[i]))
            return 1;

    return 0;
}


/**
 * returns 1 if line is a final response, either  error or success
 * See 27.007 annex B
 * WARNING: NO CARRIER and others are sometimes unsolicited
 */
static int isFinalResponse(const char *line)
{
    return isFinalResponseSuccess(line) || isFinalResponseError(line);
}


/**
 * returns 1 if line is the first line in (what will be) a two-line
 * SMS unsolicited response
 */
static const char * s_smsUnsoliciteds[] =
{
    "+CMT:",
    "+CDS:",
    "+CBM:"
};
static int isSMSUnsolicited(const char *line)
{
    size_t i;

    for (i = 0 ; i < NUM_ELEMS(s_smsUnsoliciteds) ; i++)
        if (strStartsWith(line, s_smsUnsoliciteds[i]))
            return 1;

    return 0;
}

static int mustEscape (const unsigned char c)
{
    size_t i;

    // We want to escape the following characters
    static const unsigned char esc[] = {
        FRAME_ADV_FLAG, FRAME_ADV_ESC, 0x11, 0x91, 0x13, 0x93
    };

    for (i = 0; i < NUM_ELEMS(esc); i++)
        if (c == esc[i])
            return 1;

    return 0;
}

static size_t fillBuffer(unsigned char *dst, const unsigned char *src, size_t length)
{
    size_t i, is, id = 0;
    for (is = 0; is < length; is++, id++) {
        if (mustEscape (src[is])) {
            dst[id++] = FRAME_ADV_ESC;
            dst[id] = src[is] ^ FRAME_ADV_ESC_COPML;
        }
        else
            dst[id] = src[is];
    }
    return id;
}


extern const unsigned char r_crc8[];
#define crc8(x, y) (r_crc8[(x) ^ (y)])

static int writeFrame (int channel, int control,
                       const unsigned char *input, int length)
{
    unsigned char prefix[2] = {
        GSM_EA | GSM_CR | ((0x3f & channel) << 2),
        control & 0xff
    };
    unsigned char postfix[1] = {
        0xff ^ crc8(crc8(0xff, prefix[0]), prefix[1])
    };

    unsigned char *buf = alloca((length + 5) * 2);
    int offs = 0;
    int err;

    if (s_fd < 0 || s_readerClosed > 0)
        return AT_ERROR_CHANNEL_CLOSED;

    if (length > GSM_MAX_FRAME_SIZE)
        length = GSM_MAX_FRAME_SIZE;

    buf[offs++] = FRAME_ADV_FLAG;
    buf[offs++] = FRAME_ADV_FLAG;
    buf[offs++] = FRAME_ADV_FLAG;

    offs += fillBuffer(buf + offs, prefix, 2);
    offs += fillBuffer(buf + offs, input, length);
    offs += fillBuffer(buf + offs, postfix, 1);

    buf[offs++] = FRAME_ADV_FLAG;

    pthread_mutex_lock(&mux_mutex);
    err = write(s_fd, buf, offs);
    pthread_mutex_unlock(&mux_mutex);

    return (err < 0) ? err : length;
}


static int readFrame(int *channel, int *f_type, unsigned char *buf)
{
    int len, isEscape;
    char byte;
    unsigned char *frame = alloca(GSM_MAX_FRAME_SIZE * 2 + 1);

again:
    len = 0;
    isEscape = 0;

    do {
        int i = read (s_fd, &byte, 1);
        if (i < 0) return i;
    } while (byte != FRAME_ADV_FLAG);

    do {
        int i = read (s_fd, &byte, 1);
        if (i < 0) return i;
        switch (byte) {
            case FRAME_ADV_ESC:
                isEscape = 1;
                break;
            case FRAME_ADV_FLAG:
                if (! isEscape) {
                    if (len == 0) continue;
                    goto done;
                }
                // fall through on purpose...
            default:
                if (isEscape) {
                    byte ^= FRAME_ADV_ESC_COPML;
                    isEscape = 0;
                }
                frame[len++] = byte;
        }
    } while (len < GSM_MAX_FRAME_SIZE * 2);

    LOGD("MUX< Frame is too long - ignored.\n");
    goto again;

done:
    if (len-- < 3) return 0;

    *channel = frame[0] >> 2;
    *f_type  = frame[1];

    unsigned char fcs = 0xff;

    if (GSM_FRAME_IS(GSM_TYPE_UI, *f_type)) {
        int i;
        for (i = 0; i < len; ++i)
            fcs = crc8(fcs, frame[i]);
    } else {
        fcs = crc8(crc8(fcs, frame[0]), frame[1]);
    }

    if (crc8(fcs, frame[len]) != 0xcf) {
        LOGD("MUX< crc mismatch [%x]\n", (int)crc8(fcs, frame[len]));
        goto again;
    }

    len -= 2;
    memcpy (buf, frame + 2, len);
    buf[len] = 0;

    return len;
}


static int chat (char *cmd, int tsec)
{
    ssize_t written;
    int sel, cur, len;
    fd_set rfds;
    char buf[1024];

    if (s_fd < 0 || s_readerClosed > 0)
        return AT_ERROR_CHANNEL_CLOSED;

    /* First, drain anything that is waiting...
     */
    do {
        struct timeval timeout = {0, 0};

        FD_ZERO(&rfds);
        FD_SET(s_fd, &rfds);

        sel = select(s_fd + 1, &rfds, NULL, NULL, &timeout);
        if (sel < 0) return sel;
        if (FD_ISSET(s_fd, &rfds)) {
            len = read(s_fd, buf, sizeof(buf));
            if (len < 0) return len;
        }
    } while (sel);

    LOGD("CHAT> %s\n", cmd);

    /* the main string */
    cur = 0;
    len = strlen(cmd);
    while (cur < len) {
        do {
            written = write (s_fd, cmd + cur, len - cur);
        } while (written < 0 && errno == EINTR);

        if (written < 0)
            return AT_ERROR_GENERIC;

        cur += written;
    }

    /* the \r  */

    do {
        written = write (s_fd, "\r", 1);
    } while ((written < 0 && errno == EINTR) || (written == 0));

    if (written < 0) return AT_ERROR_GENERIC;

    memset(buf, 0, sizeof(buf));
    cur = 0;
    do {
        struct timeval timeout = {.tv_sec = tsec, .tv_usec = 0};

        FD_ZERO(&rfds);
        FD_SET(s_fd, &rfds);

        sel = select(s_fd + 1, &rfds, NULL, NULL, &timeout);
        if (sel < 0) return sel;
        if (FD_ISSET(s_fd, &rfds)) {
            len = read(s_fd, buf + cur, 1);
            if (len < 0) return len;
            if (buf[cur] == '\r' || buf[cur] == '\n') {
                if (strlen(buf) > 1) {
                    LOGD("CHAT< %s", buf);
                    if (isFinalResponseSuccess(buf)) return 1;
                    if (isFinalResponseError(buf)) return -1;
                }
                cur = 0;
                memset(buf, 0, sizeof(buf));
            } else
            cur += len;
        }
    } while (sel);

    return 0;
}


/** add an intermediate response to s_chanData[channel].p_response*/
static void addIntermediate(int channel, const char *line)
{
    ATLine *p_new;

    p_new = (ATLine  *) malloc(sizeof(ATLine));

    p_new->line = strdup(line);

    /* note: this adds to the head of the list, so the list
       will be in reverse order of lines received. the order is flipped
       again before passing on to the command issuer */
    p_new->p_next = s_chanData[channel].p_response->p_intermediates;
    s_chanData[channel].p_response->p_intermediates = p_new;
}


/** assumes mutex is held */
static void handleFinalResponse(int channel, const char *line)
{
    s_chanData[channel].p_response->finalResponse = strdup(line);

    pthread_cond_signal(&s_chanData[channel].cond);
}


static void handleUnsolicited(int channel, const char *line)
{
    if (s_unsolHandler)
        s_unsolHandler (channel, line, NULL);
}


static void processLine(int channel, const char *line)
{
    pthread_mutex_lock(&at_mutex);

    if (s_chanData[channel].p_response == NULL) {
        /* no command pending */
        handleUnsolicited(channel, line);
    }
    else if (isFinalResponseSuccess(line)) {
        s_chanData[channel].p_response->success = 1;
        handleFinalResponse(channel, line);
    }
    else if (isFinalResponseError(line)) {
        s_chanData[channel].p_response->success = 0;
        handleFinalResponse(channel, line);
    }
    else if (s_chanData[channel].smsPDU != NULL && 0 == strcmp(line, "> ")) {
        // See eg. TS 27.005 4.3
        // Commands like AT+CMGS have a "> " prompt
        char *lineZ =
            alloca(strlen(s_chanData[channel].smsPDU) + 2);
        strcpy (lineZ, (char *)s_chanData[channel].smsPDU);
        strcat (lineZ, "\032");
        do {
            int err = writeFrame(channel, GSM_TYPE_UIH, (unsigned char *)lineZ, strlen(lineZ));
            if (err < 0) break;
            lineZ += err;
        } while (*lineZ);
        s_chanData[channel].smsPDU = NULL;
    }
    else switch (s_chanData[channel].type) {
        case NO_RESULT:
            handleUnsolicited(channel, line);
            break;
        case NUMERIC:
            if (s_chanData[channel].p_response->p_intermediates == NULL
                && strStartsWith (line, s_chanData[channel].responsePrefix)
            ) {
                int n = (s_chanData[channel].responsePrefix) ? strlen(s_chanData[channel].responsePrefix) : 0;
                if (isdigit(line[n]))
                    addIntermediate(channel, line + n);
                else
                    /* either we already have an intermediate response or
                       the line doesn't begin with a digit */
                    handleUnsolicited(channel, line);
            }
            else {
                handleUnsolicited(channel, line);
            }
            break;
        case SINGLELINE:
            if (s_chanData[channel].p_response->p_intermediates == NULL
                && strStartsWith (line, s_chanData[channel].responsePrefix)
            ) {
                addIntermediate(channel, line);
            }
            else {
                /* we already have an intermediate response */
                handleUnsolicited(channel, line);
            }
            break;
        case MULTILINE:
            if (strStartsWith (line, s_chanData[channel].responsePrefix)) {
                addIntermediate(channel, line);
            }
            else {
                handleUnsolicited(channel, line);
            }
            break;

        default:                 /* this should never be reached */
            LOGE("Unsupported AT command type %d\n", s_chanData[channel].type);
            handleUnsolicited(channel, line);
            break;
    }

    pthread_mutex_unlock(&at_mutex);
}


static void onReaderClosed()
{
    if (s_onReaderClosed != NULL && s_readerClosed == 0) {

        pthread_mutex_lock(&at_mutex);

        s_readerClosed = 1;

        int channel;
        for (channel = 0; channel < GSM_MAX_CHANNELS; channel++)
            pthread_cond_signal(&s_chanData[channel].cond);

        pthread_mutex_unlock(&at_mutex);

        s_onReaderClosed();
    }
}


static int handleControl (unsigned char *frame, int length)
{
    if (length > 0) {
        int i, type_length;
        int control = (unsigned char)frame[0];
        for (i = 0; i < length && !(frame[i] & GSM_EA); i++);
        type_length = ++i;
        if (control & GSM_CR) {
            int len = 0;
            // not an ack. length is supplied with 1 bit set at end.
            for (; i < length; i++) {
                len = (len << 7) + ((frame[i] >> 1) & 0x7f);
                if (frame[i] & 1) break;
            }
            i++;
            switch (control & ~GSM_CR) {
                case GSM_CONTROL_CLD:
                    LOGD("Mux-mode termination request\n");
                    break;
                case GSM_CONTROL_PSC:
                    /* Can we use the power service command for anything? */
                    break;
                case GSM_CONTROL_TEST:
                    /* Not useful to us */
                    break;
                case GSM_CONTROL_MSC:
                    if (i + 1 < length) {
                        int channel = frame[i++] >> 2;
                        int signals = frame[i];
                        s_chanData[channel].framesOK = !(signals & GSM_SIGNAL_FC);
                        LOGD("Modem status on channel %d:\n", channel);
                        LOGD("    Frames %s allowed\n",
                             s_chanData[channel].framesOK ? "are" : "are not" );
                        if (signals & GSM_SIGNAL_RTC) LOGD("    Signal RTC\n");
                        if (signals & GSM_SIGNAL_IC)  LOGD("    Signal Ring\n");
                        if (signals & GSM_SIGNAL_DV)  LOGD("    Signal DV\n");
                    }
                    break;
                default:
                    LOGD("Mux-mode Unknown command (%x)\n", control);
                    frame[0] = GSM_CONTROL_NSC;
                    length = type_length;
                    break;
            }
            // ack/nack the command
            frame[0] &= ~GSM_CR;
            writeFrame(0, GSM_TYPE_UIH | GSM_PF, frame, length);
        }
        else {
            // received an ack/nack
            if ((control & ~GSM_CR) == GSM_CONTROL_NSC)
                LOGD("Command sent not supported.\n");
        }
    }
    return 0;
}


static int handleCommand (int channel, char *buf)
{
    do {
        /* Isolate the line
         */
        while (*buf && (*buf == '\r' || *buf == '\n')) buf++;
        if (! *buf) break;
        char *line = buf;
        while (*buf && (*buf != '\r' && *buf != '\n')) buf++;
        int hasCRLF = *buf ? 1 : 0;
        if (hasCRLF) *buf++ = 0;

        if (s_chanData[channel].cmdSMS) {
            char *smsText = s_chanData[channel].txtSMS;

            if (smsText) {
                smsText = realloc(smsText, strlen(smsText) + strlen(line) + 1);
                if (smsText)
                    strcat (smsText, line);
            } else
                smsText = strdup(line);

            s_chanData[channel].txtSMS = smsText;

            LOGD("MUX SMS: hasCRLF=%d, text len=%d, sms len=%d",
                 hasCRLF, strlen(smsText), s_chanData[channel].lenSMS);

            if (strlen(smsText) >= s_chanData[channel].lenSMS) {
                if (s_unsolHandler)
                    s_unsolHandler (channel, s_chanData[channel].cmdSMS, smsText);
                free(s_chanData[channel].cmdSMS);
                free(s_chanData[channel].txtSMS);
                s_chanData[channel].cmdSMS = NULL;
                s_chanData[channel].txtSMS = NULL;
            }
        }
        else if(isSMSUnsolicited(line)) {
            char *p_cur = line;
            int err, ret = 0;
            s_chanData[channel].cmdSMS = strdup(line);
            err = at_tok_start(&p_cur);

            if (err < 0)
                LOGD("MUX[%d]: error parsing sms command", channel);

            err = at_tok_nextint(&p_cur, &ret); /* Some kind of missing ID? */
            err = at_tok_nextint(&p_cur, &ret);

            if (err < 0)
                LOGD("MUX[%d]: error parsing sms length", channel);

            /* We get additional characters for some reason,
             * but not sure why. phone number?
             * If we don't add them in then we could get a message
             * that breaks on a line less the added characters and things
             * will get messed up. But it doesn't seem to be consistent,
             * so we can't just add an arbitary number.
             */
            s_chanData[channel].lenSMS = ret * 2;

        } else {
            LOGD("MUX[%d]< %s", channel, line);
            processLine(channel, line);
        }
    } while (*buf);

    return 0;
}


static void *readerLoop(void *arg)
{
    unsigned char buffer[MAX_AT_RESPONSE+1];
    int len, channel, f_type;

    for (;;) {
        len = readFrame(&channel, &f_type, buffer);
        if (len < 0) break;

        if (GSM_FRAME_IS(GSM_TYPE_UI, f_type) ||
            GSM_FRAME_IS(GSM_TYPE_UIH, f_type)) {

            if (channel > 0) {
                /* Only forward packets when there is a pipe FD
                 * and frames are allowed.
                 */
                if (s_chanData[channel].framesOK && s_chanData[channel].pipeFd >= 0) {
                    int err = write (s_chanData[channel].pipeFd, buffer, len);
                    // Should we do something here on an error, or just let it drop?
                }
                else
                    handleCommand (channel, (char *)buffer);
            }
            else {
                handleControl(buffer, len);
            }
        }
        else {
            switch ((f_type & ~GSM_PF)) {
                case GSM_TYPE_UA:
                    if (s_chanData[channel].opened) {
                        LOGD("Logical channel %d closed\n", channel);
                    }
                    else {
                        pthread_mutex_lock(&at_mutex);
                        s_chanData[channel].opened = 1;
                        pthread_cond_signal(&s_chanData[channel].cond);
                        pthread_mutex_unlock(&at_mutex);
                        LOGD("%s channel %d opened\n", channel ? "Logical" : "Control", channel);
                    }
                    break;
                case GSM_TYPE_DM:
                    if (s_chanData[channel].opened) {
                        LOGD("DM received, but channel %d was already closed\n", channel);
                    }
                    else {
                        LOGD("%s channel %d couldn't be opened\n",
                             channel ? "Logical" : "Control", channel);
                    }
                    break;
                case GSM_TYPE_DISC:
                    if (s_chanData[channel].opened) {
                        pthread_mutex_lock(&at_mutex);
                        s_chanData[channel].opened = 0;
                        pthread_cond_signal(&s_chanData[channel].cond);
                        pthread_mutex_unlock(&at_mutex);
                        writeFrame(channel, GSM_TYPE_UA | GSM_PF, NULL, 0);
                        LOGD("%s channel %d closed\n",
                             channel ? "Logical" : "Control", channel);
                    }
                    else {
                        LOGD("Received disconnect when channel %d already closed\n", channel);
                        writeFrame(channel, GSM_TYPE_DM | GSM_PF, NULL, 0);
                    }
                    break;
                case GSM_TYPE_SABM:
                    if (s_chanData[channel].opened) {
                        LOGD("%s channel %d opened\n",
                             channel ? "Logical" : "Control", channel);
                    } else
                        LOGD("Received SABM when channel %d closed\n", channel);

                    pthread_mutex_lock(&at_mutex);
                    s_chanData[channel].opened = 1;
                    pthread_cond_signal(&s_chanData[channel].cond);
                    pthread_mutex_unlock(&at_mutex);
                    writeFrame(channel, GSM_TYPE_UA | GSM_PF, NULL, 0);
                    break;
            }
        }
    }

    onReaderClosed();
    return NULL;
}


static void *pseudoLoop(void *arg)
{
    int channel = (int)arg;
    unsigned char buffer[GSM_MAX_FRAME_SIZE];
    int err, len, off;
    struct pollfd fds;

    fds.fd = s_chanData[channel].pipeFd;
    fds.events = POLLIN;
    fds.revents = 0;

    /* We can survive EAGAIN and EIO errors, so just keep looping on either. */
    for (;;) {
        err = poll(&fds, 1, -1);
        if (err == 0 || (err < 0 && (errno == EAGAIN || errno == EIO))) continue;
        if (err < 0) goto error;

        err = read (fds.fd, buffer, sizeof(buffer));
        if (err == 0 || (err < 0 && (errno == EAGAIN || errno == EIO))) continue;
        if (err < 0) goto error;

        len = err;
        off = 0;
        do {
            err = writeFrame(channel, GSM_TYPE_UIH, buffer + off, len);
            if (err < 0) goto error;
            off += err;
            len -= err;
        } while (len > 0);
    }

error:
    LOGD("Got error reading/writing data on pseudo channel %d. errno = %d\n", channel, errno);
    s_chanData[channel].pipeFd = -1;
    close (fds.fd);
    return NULL;
}


static void clearPendingCommand(int channel)
{
    if (s_chanData[channel].p_response)
        at_response_free(s_chanData[channel].p_response);

    s_chanData[channel].p_response     = NULL;
    s_chanData[channel].responsePrefix = NULL;
    s_chanData[channel].smsPDU         = NULL;
}


/**
 * Starts AT handler on stream "fd'
 * returns 0 on success, -1 on error
 */
int at_open(int fd, int baud, ATUnsolHandler h)
{
    static const unsigned char PN_frame[] = {
        GSM_CONTROL_PN | GSM_CR, 0x11, 0x01,
        0x00, 0x01, 0x0A, 0x40, 0x00, 0x03, 0x01
    };

    static int baud_rates[] = {
        0, 9600, 19200, 38400, 57600, 115200, 230400, 460800
    };

    int ret, i;
    pthread_attr_t attr;

    s_fd = fd;
    s_readerClosed = 0;
    s_unsolHandler = h;

    bzero(s_chanData, sizeof(s_chanData));

    for (i=0; i < GSM_MAX_CHANNELS; i++) {
        s_chanData[i].pipeFd = -1;
        s_chanData[i].framesOK = 0;
        pthread_cond_init(&s_chanData[i].cond, NULL);
    }

    /* Reset the modem. Note: The modem might already be functional
     * in Multiplexed mode, so all this might fail.
     */
    if (chat("\r\n\r\n\r\nAT", 1) <= 0) {
        writeFrame(0, GSM_CONTROL_CLD | GSM_CR, NULL, 0);
        chat("AT", 1);
    }

    for (i=0; i < AT_RETRY_COUNT; i++) {
        ret = chat("ATZ", 6);
        if (ret > 0) break;
    }

    /* atchannel is tolerant of echo but it must
     * have verbose result codes.
     */
    chat("ATE0Q0V1", 3);

    /* Enter data multiplex mode.
     */
    for (i=0; i < 7 && baud > baud_rates[i]; i++);
    char command[64];
    snprintf(command, sizeof(command),
        "AT+CMUX=1,0,%d,%d", i, GSM_MAX_FRAME_SIZE);
    chat(command, 5);

    writeFrame(0, GSM_TYPE_SABM | GSM_PF, NULL, 0);

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&s_tid_reader, &attr, readerLoop, &attr);

    if (ret < 0) {
        perror ("pthread_create");
        return -1;
    }

    writeFrame(0, GSM_TYPE_UIH | GSM_PF, PN_frame, sizeof(PN_frame));

    return 0;
}


/* FIXME is it ok to call this from the reader and the command thread? */
void at_close()
{
    int channel;
    for (channel=1; channel < GSM_MAX_CHANNELS; channel++) {
        if (s_chanData[channel].pipeFd >= 0)
            close(s_chanData[channel].pipeFd);
        if (s_chanData[channel].opened) {
            LOGD("Closing down logical channel %d\n", channel);
            writeFrame(channel, GSM_CONTROL_CLD | GSM_CR, NULL, 0);
        }
    }

    writeFrame(0, GSM_CONTROL_CLD | GSM_CR, NULL, 0);

    pthread_mutex_lock(&at_mutex);

    s_readerClosed = 1;

    for (channel=0; channel < GSM_MAX_CHANNELS; channel++)
        pthread_cond_signal(&s_chanData[channel].cond);

    pthread_mutex_unlock(&at_mutex);

    if (s_fd >= 0) close (s_fd);
    s_fd = -1;

    /* the reader thread should eventually die */
}


static ATResponse * at_response_new()
{
    return (ATResponse *) calloc(1, sizeof(ATResponse));
}


void at_response_free(ATResponse *p_response)
{
    ATLine *p_line;

    if (p_response == NULL) return;

    p_line = p_response->p_intermediates;

    while (p_line != NULL) {
        ATLine *p_toFree;

        p_toFree = p_line;
        p_line = p_line->p_next;

        free(p_toFree->line);
        free(p_toFree);
    }

    free (p_response->finalResponse);
    free (p_response);
}


/**
 * The line reader places the intermediate responses in reverse order
 * here we flip them back
 */
static void reverseIntermediates(ATResponse *p_response)
{
    ATLine *pcur,*pnext;

    pcur = p_response->p_intermediates;
    p_response->p_intermediates = NULL;

    while (pcur != NULL) {
        pnext = pcur->p_next;
        pcur->p_next = p_response->p_intermediates;
        p_response->p_intermediates = pcur;
        pcur = pnext;
    }
}


/**
 * Internal send_command implementation
 * Doesn't lock or call the timeout callback
 *
 * timeoutMsec == 0 means infinite timeout
 */

static int at_send_command_full_nolock (const int channel,
const char *command,
ATCommandType type,
const char *responsePrefix,
const char *smspdu,
long long timeoutMsec,
ATResponse **pp_outResponse)
{
    int err = 0;
    char *input = alloca(strlen(command)+3);

    if(s_chanData[channel].p_response != NULL) {
        err = AT_ERROR_COMMAND_PENDING;
        goto error;
    }

    strcpy (input, command);
    strcat (input, "\r\n");
    LOGD("MUX[%d]> %s", channel, input);

    do {
        err = writeFrame(channel, GSM_TYPE_UIH, (unsigned char *)input, strlen(input));
        if (err < 0) goto error;
        input += err;
    } while (*input);

    s_chanData[channel].type           = type;
    s_chanData[channel].responsePrefix = responsePrefix;
    s_chanData[channel].smsPDU         = smspdu;
    s_chanData[channel].p_response     = at_response_new();

    while (s_chanData[channel].p_response->finalResponse == NULL && s_readerClosed == 0) {
        if (timeoutMsec != 0)
            err = pthread_cond_timeout_np(&s_chanData[channel].cond, &at_mutex, timeoutMsec);
        else
            err = pthread_cond_wait(&s_chanData[channel].cond, &at_mutex);

        if (err == ETIMEDOUT) {
            err = AT_ERROR_TIMEOUT;
            goto error;
        }
    }

    if (pp_outResponse == NULL) {
        at_response_free(s_chanData[channel].p_response);
    }
    else {
        /* line reader stores intermediate responses in reverse order */
        reverseIntermediates(s_chanData[channel].p_response);
        *pp_outResponse = s_chanData[channel].p_response;
    }

    s_chanData[channel].p_response = NULL;

    if(s_readerClosed > 0) {
        err = AT_ERROR_CHANNEL_CLOSED;
        goto error;
    }

    err = 0;
    error:
    clearPendingCommand(channel);

    return err;
}


/**
 * Internal send_command implementation
 *
 * timeoutMsec == 0 means infinite timeout
 */
static int at_send_command_full (const int channel,
const char *command,
ATCommandType type,
const char *responsePrefix,
const char *smspdu,
long long timeoutMsec,
ATResponse **pp_outResponse)
{
    int err, i;

    if (0 != pthread_equal(s_tid_reader, pthread_self())) {
        /* cannot be called from reader thread */
        return AT_ERROR_INVALID_THREAD;
    }

    pthread_mutex_lock(&at_mutex);

    for (i = 0 ; i < COMMAND_RETRY_COUNT ; i++) {
        err = at_send_command_full_nolock(channel, command, type,
            responsePrefix, smspdu,
            timeoutMsec, pp_outResponse);

        if (err == 0) {
            break;
        }
    }

    pthread_mutex_unlock(&at_mutex);

    if (err == AT_ERROR_TIMEOUT && s_onTimeout != NULL) {
        s_onTimeout();
    }

    return err;
}


/**
 * Issue a single normal AT command with no intermediate response expected
 *
 * "command" should not include \r
 * pp_outResponse can be NULL
 *
 * if non-NULL, the resulting ATResponse * must be eventually freed with
 * at_response_free
 */
int at_send_command (const int channel,
const char *command,
ATResponse **pp_outResponse)
{
    int err;

    err = at_send_command_full (channel, command, NO_RESULT, NULL,
        NULL, COMMAND_TIMEOUT_MSEC, pp_outResponse);

    return err;
}


int at_send_command_singleline (const int channel,
const char *command,
const char *responsePrefix,
ATResponse **pp_outResponse)
{
    int err;

    err = at_send_command_full (channel, command, SINGLELINE, responsePrefix,
        NULL, COMMAND_TIMEOUT_MSEC, pp_outResponse);

    if (err == 0 && pp_outResponse != NULL
        && (*pp_outResponse)->success > 0
        && (*pp_outResponse)->p_intermediates == NULL
    ) {
        /* successful command must have an intermediate response */
        at_response_free(*pp_outResponse);
        *pp_outResponse = NULL;
        return AT_ERROR_INVALID_RESPONSE;
    }

    return err;
}


int at_send_command_numeric (const int channel,
const char *command,
const char *responsePrefix,
ATResponse **pp_outResponse)
{
    int err;

    err = at_send_command_full (channel, command, NUMERIC, responsePrefix,
        NULL, COMMAND_TIMEOUT_MSEC, pp_outResponse);

    if (err == 0 && pp_outResponse != NULL
        && (*pp_outResponse)->success > 0
        && (*pp_outResponse)->p_intermediates == NULL
    ) {
        /* successful command must have an intermediate response */
        at_response_free(*pp_outResponse);
        *pp_outResponse = NULL;
        return AT_ERROR_INVALID_RESPONSE;
    }

    return err;
}


int at_send_command_sms (const int channel,
const char *command,
const char *pdu,
const char *responsePrefix,
ATResponse **pp_outResponse)
{
    int err;

    err = at_send_command_full (channel, command, SINGLELINE, responsePrefix,
        pdu, LONG_COMMAND_TIMEOUT_MSEC, pp_outResponse);

    if (err == 0 && pp_outResponse != NULL
        && (*pp_outResponse)->success > 0
        && (*pp_outResponse)->p_intermediates == NULL
    ) {
        /* successful command must have an intermediate response */
        at_response_free(*pp_outResponse);
        *pp_outResponse = NULL;
        return AT_ERROR_INVALID_RESPONSE;
    }

    return err;
}


int at_send_command_multiline (const int channel,
const char *command,
const char *responsePrefix,
ATResponse **pp_outResponse)
{
    int err;

    err = at_send_command_full (channel, command, MULTILINE, responsePrefix,
        NULL, LONG_COMMAND_TIMEOUT_MSEC, pp_outResponse);

    return err;
}


/** This callback is invoked on the command thread */
void at_set_on_timeout(void (*onTimeout)(void))
{
    s_onTimeout = onTimeout;
}


/**
 *  This callback is invoked on the reader thread (like ATUnsolHandler)
 *  when the input stream closes before you call at_close
 *  (not when you call at_close())
 *  You should still call at_close()
 */

void at_set_on_reader_closed(void (*onClose)(void))
{
    s_onReaderClosed = onClose;
}


/**
 * Open the provided channel number.
 */

int at_channel(int channel)
{
    int i;
    int err = 0;

    if (0 != pthread_equal(s_tid_reader, pthread_self())) {
        /* cannot be called from reader thread */
        LOGE("at_channel called from read thread!!\n");
        return AT_ERROR_INVALID_THREAD;
    }

    pthread_mutex_lock(&at_mutex);

    writeFrame(channel, GSM_TYPE_SABM | GSM_PF, NULL, 0);

    err = pthread_cond_timeout_np(&s_chanData[channel].cond, &at_mutex, COMMAND_TIMEOUT_MSEC);

    if (err == 0) {
        char channel_val = channel << 2 | 0x3;
        unsigned char msc1[] = {GSM_CONTROL_MSC, 0x05, channel_val, 0x0d};
        unsigned char msc2[] = {GSM_CONTROL_MSC | GSM_CR, 0x05, channel_val, 0x81};

        writeFrame(0, GSM_TYPE_UIH | GSM_PF, msc1, sizeof(msc1));
        writeFrame(0, GSM_TYPE_UIH | GSM_PF, msc2, sizeof(msc2));

        for (i = 0 ; i < AT_RETRY_COUNT ; i++) {
            /* some stacks start with verbose off */
            err = at_send_command_full_nolock (channel, "AT", NO_RESULT,
                NULL, NULL, AT_TIMEOUT_MSEC, NULL);

            if (err == 0)
                break;
        }
    }

    pthread_mutex_unlock(&at_mutex);

    return err;
}


/**
 * Open the provided channel number.
 */

int at_pseudo(int channel, char *ptsname, int len)
{
    int err = 0;
    int fd = s_chanData[channel].pipeFd;
    struct termios ios;

    if (fd < 0) {

        fd = open ("/dev/ptmx", O_RDWR | O_NONBLOCK);
        if (fd < 0) return -1;

        s_chanData[channel].pipeFd = fd;

        pthread_attr_t attr;
        pthread_attr_init (&attr);

        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        err = pthread_create(&s_chanData[channel].pipeTid, &attr,
                pseudoLoop, (void *)channel);
    }

    err = tcgetattr(fd, &ios);
    if (err < 0) goto error;

    ios.c_cflag &= ~(CSIZE | CSTOPB | PARENB);
    ios.c_cflag |= CS8 | CREAD | CLOCAL;
    ios.c_iflag  = IGNPAR;
    ios.c_oflag  = 0;
    ios.c_lflag  = 0;

    tcsetattr(fd, TCSAFLUSH, &ios);

    err = grantpt (fd);
    if (err < 0) goto error;

    err = unlockpt (fd);
    if (err < 0) goto error;

    ptsname_r (s_chanData[channel].pipeFd, ptsname, len);

    /* Allow group to access.
     */
    chmod (ptsname, 0660);

    return err;

error:
    LOGE("at_pseudo failed to setup terminal!!\n");
    s_chanData[channel].pipeFd = -1;
    if (fd >= 0) close (fd);
    return err;
}


/**
 * Returns error code from response
 * Assumes AT+CMEE=1 (numeric) mode
 */
AT_CME_Error at_get_cme_error(const ATResponse *p_response)
{
    int ret;
    int err;
    char *p_cur;

    if (p_response->success > 0) {
        return CME_SUCCESS;
    }

    if (p_response->finalResponse == NULL
        || !strStartsWith(p_response->finalResponse, "+CME ERROR:")
    ) {
        return CME_ERROR_NON_CME;
    }

    p_cur = p_response->finalResponse;
    err = at_tok_start(&p_cur);

    if (err < 0) {
        return CME_ERROR_NON_CME;
    }

    err = at_tok_nextint(&p_cur, &ret);

    if (err < 0) {
        return CME_ERROR_NON_CME;
    }

    return (AT_CME_Error) ret;
}


/**
 * Returns error code from response
 */
AT_CMS_Error at_get_cms_error(const ATResponse *p_response)
{
    int ret;
    int err;
    char *p_cur;

    if (p_response->success > 0) {
        return CMS_SUCCESS;
    }

    if (p_response->finalResponse == NULL
        || !strStartsWith(p_response->finalResponse, "+CMS ERROR:")
    ) {
        return CMS_ERROR_NON_CMS;
    }

    p_cur = p_response->finalResponse;
    err = at_tok_start(&p_cur);

    if (err < 0) {
        return CMS_ERROR_NON_CMS;
    }

    err = at_tok_nextint(&p_cur, &ret);

    if (err < 0) {
        return CMS_ERROR_NON_CMS;
    }

    return (AT_CMS_Error) ret;
}
