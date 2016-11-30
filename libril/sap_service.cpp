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

#include <android/hardware/radio/1.0/ISap.h>

#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <sap_service.h>
#include "pb_decode.h"
#include "pb_encode.h"

using namespace android::hardware::radio::V1_0;
using ::android::hardware::Return;
using ::android::hardware::Status;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_array;
using ::android::hardware::Void;
using android::CommandInfo;
using android::RequestInfo;
using android::requestToString;
using android::sp;

static CommandInfo *s_commands;

struct SapImpl;

#if (SIM_COUNT >= 2)
sp<SapImpl> sapService[SIM_COUNT];
#else
sp<SapImpl> sapService[1];
#endif

struct SapImpl : public ISap {
    int32_t slotId;
    sp<ISapCallback> sapCallback;
    RIL_SOCKET_ID rilSocketId;

    Return<void> setCallback(const ::android::sp<ISapCallback>& sapCallbackParam);

    Return<void> connectReq(int32_t token, int32_t maxMsgSize);

    Return<void> disconnectReq(int32_t token);

    Return<void> apduReq(int32_t token, SapApduType type,
            const ::android::hardware::hidl_vec<uint8_t>& command);

    Return<void> transferAtrReq(int32_t token);

    Return<void> powerReq(int32_t token, bool state);

    Return<void> resetSimReq(int32_t token);

    Return<void> transferCardReaderStatusReq(int32_t token);

    Return<void> setTransferProtocolReq(int32_t token, SapTransferProtocol transferProtocol);
};

Return<void> SapImpl::setCallback(const ::android::sp<ISapCallback>& sapCallbackParam) {
    RLOGD("SapImpl::setCallback");
    sapCallback = sapCallbackParam;
    return Status::ok();
}

#define MAX_BUF_SIZE (1024*8)
uint8_t sendBuffer[MAX_BUF_SIZE];

Return<void> SapImpl::connectReq(int32_t token, int32_t maxMsgSize) {
    MsgHeader msg;
    msg.token = token;
    msg.type = MsgType_REQUEST;
    msg.id = MsgId_RIL_SIM_SAP_CONNECT;
    msg.error = Error_RIL_E_SUCCESS;

    /***** Encode RIL_SIM_SAP_CONNECT_REQ *****/
    uint8_t *reqPtr = NULL;
    uint16_t reqLen = 0;
    RIL_SIM_SAP_CONNECT_REQ req;
    pb_ostream_t stream;

    reqPtr = (uint8_t *)malloc(sizeof(RIL_SIM_SAP_CONNECT_REQ));
    if (reqPtr == NULL) {
        RLOGE("SapImpl::connectReq: Error allocating reqPtr");
        // todo: set a service specific code?
        return Status::fromExceptionCode(Status::EX_SERVICE_SPECIFIC);
    }

    memset(&req, 0, sizeof(RIL_SIM_SAP_CONNECT_REQ));
    memset(reqPtr, 0, sizeof(RIL_SIM_SAP_CONNECT_REQ));

    req.max_message_size = maxMsgSize;

    /* Encode the outgoing message */
    stream = pb_ostream_from_buffer(reqPtr, sizeof(RIL_SIM_SAP_CONNECT_REQ));
    if (!pb_encode(&stream, RIL_SIM_SAP_CONNECT_REQ_fields, &req)) {
        RLOGE("SapImpl::connectReq: Error encoding RIL_SIM_SAP_CONNECT_REQ");
        free(reqPtr);
        // todo: set a service specific code?
        return Status::fromExceptionCode(Status::EX_SERVICE_SPECIFIC);
    }
    reqLen = stream.bytes_written;
    /***** Encode RIL_SIM_SAP_CONNECT_REQ done *****/

    /* encoded req is payload */
    msg.payload->size = reqLen;
    memcpy(msg.payload->bytes, reqPtr, reqLen);

    /* Encode MsgHeader */
    /*pb_ostream_t ostream;
    int bufBytesWritten = 0;
    ostream = pb_ostream_from_buffer(sendBuffer + 4, sizeof(sendBuffer) - 4);
    if (pb_encode(ostream, MsgHeader_fields, &msg)) {
        bufBytesWritten = ostream.bytes_written;
    }

    unsigned char *tmp = (unsigned char*) sendBuffer;
    tmp[0] = (unsigned char)((bufBytesWritten) >> 24);
    tmp[1] = (unsigned char)(((bufBytesWritten) >> 16 ) & 0xff);
    tmp[2] = (unsigned char)(((bufBytesWritten) >> 8 ) & 0xff);
    tmp[3] = (unsigned char)((bufBytesWritten) & 0xff);*/

    RilSapSocket *sapSocket = RilSapSocket::getSocketById(rilSocketId);
    if (sapSocket) {
        sapSocket->dispatchRequest(&msg);
    } else {
        RLOGE("SapImpl::connectReq: sapSocket is null");
        free(reqPtr);
        // todo: set a service specific code?
        return Status::fromExceptionCode(Status::EX_SERVICE_SPECIFIC);
    }
    free(reqPtr);
    return Status::ok();
}

Return<void> SapImpl::disconnectReq(int32_t token) {
    return Status::ok();
}

Return<void> SapImpl::apduReq(int32_t token, SapApduType type,
        const ::android::hardware::hidl_vec<uint8_t>& command) {
    return Status::ok();
}

Return<void> SapImpl::transferAtrReq(int32_t token) {
    return Status::ok();
}

Return<void> SapImpl::powerReq(int32_t token, bool state) {
    return Status::ok();
}

Return<void> SapImpl::resetSimReq(int32_t token) {
    return Status::ok();
}

Return<void> SapImpl::transferCardReaderStatusReq(int32_t token) {
    return Status::ok();
}

Return<void> SapImpl::setTransferProtocolReq(int32_t token, SapTransferProtocol transferProtocol) {
    return Status::ok();
}

void *sapDecodeMessage(MsgId msgId, uint8_t *payloadPtr, size_t payloadLen) {
    void *responsePtr = NULL;
    bool decodeStatus = false;
    pb_istream_t stream;

    /* Create the stream */
    stream = pb_istream_from_buffer((uint8_t *)payloadPtr, payloadLen);

    /* Decode based on the message id */
    switch (msgId)
    {
        case MsgId_RIL_SIM_SAP_CONNECT:
            responsePtr = malloc(sizeof(RIL_SIM_SAP_CONNECT_REQ));
            if (responsePtr) {
                if (!pb_decode(&stream, RIL_SIM_SAP_CONNECT_REQ_fields, responsePtr)) {
                    RLOGE("Error decoding RIL_SIM_SAP_CONNECT_REQ");
                    return NULL;
                }
            }
            break;

        case MsgId_RIL_SIM_SAP_DISCONNECT:
            responsePtr = malloc(sizeof(RIL_SIM_SAP_DISCONNECT_REQ));
            if (responsePtr) {
                if (!pb_decode(&stream, RIL_SIM_SAP_DISCONNECT_REQ_fields, responsePtr)) {
                    RLOGE("Error decoding RIL_SIM_SAP_DISCONNECT_REQ");
                    return NULL;
                }
            }
            break;

        case MsgId_RIL_SIM_SAP_APDU:
            responsePtr = malloc(sizeof(RIL_SIM_SAP_APDU_REQ));
            if (responsePtr) {
                if (!pb_decode(&stream, RIL_SIM_SAP_APDU_REQ_fields, responsePtr)) {
                    RLOGE("Error decoding RIL_SIM_SAP_APDU_REQ");
                    return NULL;
                }
            }
            break;

        case MsgId_RIL_SIM_SAP_TRANSFER_ATR:
            responsePtr = malloc(sizeof(RIL_SIM_SAP_TRANSFER_ATR_REQ));
            if (responsePtr) {
                if (!pb_decode(&stream, RIL_SIM_SAP_TRANSFER_ATR_REQ_fields, responsePtr)) {
                    RLOGE("Error decoding RIL_SIM_SAP_TRANSFER_ATR_REQ");
                    return NULL;
                }
            }
            break;

        case MsgId_RIL_SIM_SAP_POWER:
            responsePtr = malloc(sizeof(RIL_SIM_SAP_POWER_REQ));
            if (responsePtr) {
                if (!pb_decode(&stream, RIL_SIM_SAP_POWER_REQ_fields, responsePtr)) {
                    RLOGE("Error decoding RIL_SIM_SAP_POWER_REQ");
                    return NULL;
                }
            }
            break;

        case MsgId_RIL_SIM_SAP_RESET_SIM:
            responsePtr = malloc(sizeof(RIL_SIM_SAP_RESET_SIM_REQ));
            if (responsePtr) {
                if (!pb_decode(&stream, RIL_SIM_SAP_RESET_SIM_REQ_fields, responsePtr)) {
                    RLOGE("Error decoding RIL_SIM_SAP_RESET_SIM_REQ");
                    return NULL;
                }
            }
            break;

        case MsgId_RIL_SIM_SAP_TRANSFER_CARD_READER_STATUS:
            responsePtr = malloc(sizeof(RIL_SIM_SAP_TRANSFER_CARD_READER_STATUS_REQ));
            if (responsePtr) {
                if (!pb_decode(&stream, RIL_SIM_SAP_TRANSFER_CARD_READER_STATUS_REQ_fields, responsePtr)) {
                    RLOGE("Error decoding RIL_SIM_SAP_TRANSFER_CARD_READER_STATUS_REQ");
                    return NULL;
                }
            }
            break;

        /* Unsupported */
        case MsgId_UNKNOWN_REQ:
        case MsgId_RIL_SIM_SAP_ERROR_RESP:
        case MsgId_RIL_SIM_SAP_SET_TRANSFER_PROTOCOL:
        default:
            break;
    }
    return responsePtr;
} /* sapDecodeMessage */

sp<SapImpl> getSapImpl(RilSapSocket *sapSocket) {
    switch (sapSocket->id) {
        case RIL_SOCKET_1:
            return sapService[0];
        #if (SIM_COUNT >= 2)
        case RIL_SOCKET_2:
            return sapService[1];
        #if (SIM_COUNT >= 3)
        case RIL_SOCKET_3:
            return sapService[2];
        #if (SIM_COUNT >= 4)
        case RIL_SOCKET_4:
            return sapService[3];
        #endif
        #endif
        #endif
        default:
            return NULL;
    }
}

void sap::processResponse(MsgHeader *rsp, RilSapSocket *sapSocket) {
    MsgId responseId = rsp->id;
    uint8_t *data = rsp->payload->bytes;
    size_t dataLen = rsp->payload->size;

    void *messagePtr = sapDecodeMessage(responseId, data, dataLen);

    sp<SapImpl> sapImpl = getSapImpl(sapSocket);
    if (sapImpl->sapCallback == NULL) {
        RLOGE("processResponse: sapCallback == NULL; MsgId = %d", responseId);
        return;
    }

    switch (responseId) {
        case MsgId_RIL_SIM_SAP_CONNECT:
            sapImpl->sapCallback->connectResponse(rsp->id, (SapConnectRsp)((RIL_SIM_SAP_CONNECT_RSP *)messagePtr)->response, ((RIL_SIM_SAP_CONNECT_RSP *)messagePtr)->max_message_size);
            break;
        case MsgId_RIL_SIM_SAP_DISCONNECT:
            break;
        case MsgId_RIL_SIM_SAP_APDU:
            break;
        case MsgId_RIL_SIM_SAP_TRANSFER_ATR:
            break;
        case MsgId_RIL_SIM_SAP_POWER:
            break;
        case MsgId_RIL_SIM_SAP_RESET_SIM:
            break;
        case MsgId_RIL_SIM_SAP_STATUS:
            break;
        case MsgId_RIL_SIM_SAP_TRANSFER_CARD_READER_STATUS:
            break;
        case MsgId_RIL_SIM_SAP_ERROR_RESP:
            break;
        case MsgId_RIL_SIM_SAP_SET_TRANSFER_PROTOCOL:
            break;
        case MsgId_UNKNOWN_REQ:
        default:
            break;
    }
}

void sap::registerService(RIL_RadioFunctions *callbacks, CommandInfo *commands) {
    using namespace android::hardware;
    int simCount = 1;
    char *serviceNames[] = {
        "sap_uim_socket1"
        #if (SIM_COUNT >= 2)
        , "sap_uim_socket2"
        #if (SIM_COUNT >= 3)
        , "sap_uim_socket3"
        #if (SIM_COUNT >= 4)
        , "sap_uim_socket4"
        #endif
        #endif
        #endif
    };

    RIL_SOCKET_ID socketIds[] = {
        RIL_SOCKET_1
        #if (SIM_COUNT >= 2)
        , RIL_SOCKET_2
        #if (SIM_COUNT >= 3)
        , RIL_SOCKET_3
        #if (SIM_COUNT >= 4)
        , RIL_SOCKET_4
        #endif
        #endif
        #endif
    };
    #if (SIM_COUNT >= 2)
    simCount = SIM_COUNT;
    #endif

    for (int i = 0; i < simCount; i++) {
        sapService[i] = new SapImpl;
        sapService[i]->slotId = i;
        sapService[i]->rilSocketId = socketIds[i];
        RLOGD("registerService: starting ISap %s", serviceNames[i]);
        android::status_t status = sapService[i]->registerAsService(serviceNames[i]);
    }

    s_commands = commands;
}