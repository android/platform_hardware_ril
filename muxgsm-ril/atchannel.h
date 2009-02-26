/* //hardware/ril/muxgsm-ril/atchannel.h
**
** Copyright 2006, Google Inc.
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

#ifndef ATCHANNEL_H
#define ATCHANNEL_H 1

#ifdef __cplusplus
extern "C" {
#endif

/* Control and data channels used for AT commands
 */
#define AT_CMD			1
#define AT_DATA			2

#define AT_ERROR_GENERIC			-1
#define AT_ERROR_COMMAND_PENDING	-2
#define AT_ERROR_CHANNEL_CLOSED		-3
#define AT_ERROR_TIMEOUT			-4
#define AT_ERROR_INVALID_THREAD		-5 /* AT commands may not be issued from
                                       reader thread (or unsolicited response
                                       callback */
#define AT_ERROR_INVALID_RESPONSE	-6 /* eg an at_send_command_singleline that
                                        did not get back an intermediate
                                        response */


typedef enum {
    NO_RESULT,   /* no intermediate response expected */
    NUMERIC,     /* a single intermediate response starting with a 0-9 */
    SINGLELINE,  /* a single intermediate response starting with a prefix */
    MULTILINE    /* multiple line intermediate response
                    starting with a prefix */
} ATCommandType;

/** a singly-lined list of intermediate responses */
typedef struct ATLine  {
    struct ATLine *p_next;
    char *line;
} ATLine;

/** Free this with at_response_free() */
typedef struct {
    int success;              /* true if final response indicates
                                 success (eg "OK") */
    char *finalResponse;      /* eg OK, ERROR */
    ATLine  *p_intermediates; /* any intermediate responses */
} ATResponse;

/**
 * a user-provided unsolicited response handler function
 * this will be called from the reader thread, so do not block
 * "s" is the line, and "sms_pdu" is either NULL or the PDU response
 * for multi-line TS 27.005 SMS PDU responses (eg +CMT:)
 */
typedef void (*ATUnsolHandler)(const int channel,
                               const char *s,
                               const char *sms_pdu);

int at_open(int fd, int baud, ATUnsolHandler h);
void at_close();

/* This callback is invoked on the command thread.
   You should reset or handshake here to avoid getting out of sync */
void at_set_on_timeout(void (*onTimeout)(void));

/* This callback is invoked on the reader thread (like ATUnsolHandler)
   when the input stream closes before you call at_close
   (not when you call at_close())
   You should still call at_close()
   It may also be invoked immediately from the current thread if the read
   channel is already closed */
void at_set_on_reader_closed(void (*onClose)(void));

int at_send_command_singleline (const int channel,
                                const char *command,
                                const char *responsePrefix,
                                ATResponse **pp_outResponse);

int at_send_command_numeric (const int channel,
                             const char *command,
                             const char *responsePrefix,
                             ATResponse **pp_outResponse);

int at_send_command_multiline (const int channel,
                               const char *command,
                               const char *responsePrefix,
                               ATResponse **pp_outResponse);


int at_channel(int channel);

int at_pseudo(int channel, char *ptsname, int len);

int at_send_command (const int channel,
                     const char *command,
                     ATResponse **pp_outResponse);

int at_send_command_sms (const int channel,
                         const char *command,
                         const char *pdu,
                         const char *responsePrefix,
                         ATResponse **pp_outResponse);

void at_response_free(ATResponse *p_response);

typedef enum {
    CME_ERROR_NON_CME			= -1,
    CME_SUCCESS					= 0,
    CME_OPERATION_NOT_ALLOWED	= 3,
    CME_SIM_NOT_INSERTED		= 10
} AT_CME_Error;

AT_CME_Error at_get_cme_error(const ATResponse *p_response);

typedef enum {
    CMS_ERROR_NON_CMS	= -1,
    CMS_SUCCESS			= 0,
    CMS_SIM_BUSY		= 314
} AT_CMS_Error;

AT_CMS_Error at_get_cms_error(const ATResponse *p_response);

#ifdef __cplusplus
}
#endif

#endif /*ATCHANNEL_H*/
