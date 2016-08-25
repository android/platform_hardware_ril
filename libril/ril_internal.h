/*
 * Copyright (c) 2016 The Android Open Source Project
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

#ifndef ANDROID_RIL_INTERNAL_H
#define ANDROID_RIL_INTERNAL_H

#include <binder/Parcel.h>

namespace android {

#define SOCKET_NAME_RIL "rild"
#define SOCKET2_NAME_RIL "rild2"
#define SOCKET3_NAME_RIL "rild3"
#define SOCKET4_NAME_RIL "rild4"

/* Constants for response types */
#define RESPONSE_SOLICITED 0
#define RESPONSE_UNSOLICITED 1
#define RESPONSE_SOLICITED_ACK 2
#define RESPONSE_SOLICITED_ACK_EXP 3
#define RESPONSE_UNSOLICITED_ACK_EXP 4

typedef struct CommandInfo CommandInfo;

extern "C" const char * requestToString(int request);

typedef struct RequestInfo {
    int32_t token;      //this is not RIL_Token
    CommandInfo *pCI;
    struct RequestInfo *p_next;
    char cancelled;
    char local;         // responses to local commands do not go back to command process
    RIL_SOCKET_ID socket_id;
    int wasAckSent;    // Indicates whether an ack was sent earlier
} RequestInfo;

typedef struct CommandInfo {
    int requestNumber;
    // todo: remove
    void (*dispatchFunction) (Parcel &p, struct RequestInfo *pRI);
    // todo: change parcel to serial
    int(*responseFunction) (Parcel &p, int slotId, int requestNumber, int responseType, int token,
            RIL_Errno e, void *response, size_t responselen);
} CommandInfo;

int addRequestToList(RequestInfo *pRI, int request, int token, RIL_SOCKET_ID socket_id);

char * RIL_getRilSocketName();

}   // namespace android

#endif //ANDROID_RIL_INTERNAL_H
