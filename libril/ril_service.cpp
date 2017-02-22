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

#define LOG_TAG "RILC"

#include <android/hardware/radio/1.0/IRadio.h>
#include <android/hardware/radio/deprecated/1.0/IOemHook.h>

#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <ril_service.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/SystemClock.h>
#include <inttypes.h>

#define INVALID_HEX_CHAR 16

using namespace android::hardware::radio::V1_0;
using namespace android::hardware::radio::deprecated::V1_0;
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::joinRpcThreadpool;
using ::android::hardware::Return;
using ::android::hardware::Status;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_array;
using ::android::hardware::Void;
using android::CommandInfo;
using android::RequestInfo;
using android::requestToString;
using android::sp;

#define BOOL_TO_INT(x) (x ? 1 : 0)
#define ATOI_NULL_HANDLED(x) (x ? atoi(x) : -1)
#define ATOI_NULL_HANDLED_DEF(x, defaultVal) (x ? atoi(x) : defaultVal)

RIL_RadioFunctions *s_vendorFunctions = NULL;
static CommandInfo *s_commands;

struct RadioImpl;
struct OemHookImpl;

#if (SIM_COUNT >= 2)
sp<RadioImpl> radioService[SIM_COUNT];
sp<OemHookImpl> oemHookService[SIM_COUNT];
// counter used for synchronization. It is incremented every time response callbacks are updated.
volatile int32_t mCounter[SIM_COUNT];
#else
sp<RadioImpl> radioService[1];
sp<OemHookImpl> oemHookService[1];
// counter used for synchronization. It is incremented every time response callbacks are updated.
volatile int32_t mCounter[1];
#endif

static pthread_rwlock_t radioServiceRwlock = PTHREAD_RWLOCK_INITIALIZER;

#if (SIM_COUNT >= 2)
static pthread_rwlock_t radioServiceRwlock2 = PTHREAD_RWLOCK_INITIALIZER;
#if (SIM_COUNT >= 3)
static pthread_rwlock_t radioServiceRwlock3 = PTHREAD_RWLOCK_INITIALIZER;
#if (SIM_COUNT >= 4)
static pthread_rwlock_t radioServiceRwlock4 = PTHREAD_RWLOCK_INITIALIZER;
#endif
#endif
#endif

void convertRilHardwareConfigListToHal(void *response, size_t responseLen,
        hidl_vec<HardwareConfig>& records);

void convertRilRadioCapabilityToHal(void *response, size_t responseLen, RadioCapability& rc);

void convertRilLceDataInfoToHal(void *response, size_t responseLen, LceDataInfo& lce);

void convertRilSignalStrengthToHal(void *response, size_t responseLen,
        SignalStrength& signalStrength);

void convertRilDataCallToHal(RIL_Data_Call_Response_v11 *dcResponse,
        SetupDataCallResult& dcResult);

void convertRilDataCallListToHal(void *response, size_t responseLen,
        hidl_vec<SetupDataCallResult>& dcResultList);

void convertRilCellInfoListToHal(void *response, size_t responseLen, hidl_vec<CellInfo>& records);

struct RadioImpl : public IRadio {
    int32_t mSlotId;
    sp<IRadioResponse> mRadioResponse;
    sp<IRadioIndication> mRadioIndication;

    Return<void> setResponseFunctions(
            const ::android::sp<IRadioResponse>& radioResponse,
            const ::android::sp<IRadioIndication>& radioIndication);

    Return<void> getIccCardStatus(int32_t serial);

    Return<void> supplyIccPinForApp(int32_t serial, const hidl_string& pin,
            const hidl_string& aid);

    Return<void> supplyIccPukForApp(int32_t serial, const hidl_string& puk,
            const hidl_string& pin, const hidl_string& aid);

    Return<void> supplyIccPin2ForApp(int32_t serial,
            const hidl_string& pin2,
            const hidl_string& aid);

    Return<void> supplyIccPuk2ForApp(int32_t serial, const hidl_string& puk2,
            const hidl_string& pin2, const hidl_string& aid);

    Return<void> changeIccPinForApp(int32_t serial, const hidl_string& oldPin,
            const hidl_string& newPin, const hidl_string& aid);

    Return<void> changeIccPin2ForApp(int32_t serial, const hidl_string& oldPin2,
            const hidl_string& newPin2, const hidl_string& aid);

    Return<void> supplyNetworkDepersonalization(int32_t serial, const hidl_string& netPin);

    Return<void> getCurrentCalls(int32_t serial);

    Return<void> dial(int32_t serial, const Dial& dialInfo);

    Return<void> getImsiForApp(int32_t serial,
            const ::android::hardware::hidl_string& aid);

    Return<void> hangup(int32_t serial, int32_t gsmIndex);

    Return<void> hangupWaitingOrBackground(int32_t serial);

    Return<void> hangupForegroundResumeBackground(int32_t serial);

    Return<void> switchWaitingOrHoldingAndActive(int32_t serial);

    Return<void> conference(int32_t serial);

    Return<void> rejectCall(int32_t serial);

    Return<void> getLastCallFailCause(int32_t serial);

    Return<void> getSignalStrength(int32_t serial);

    Return<void> getVoiceRegistrationState(int32_t serial);

    Return<void> getDataRegistrationState(int32_t serial);

    Return<void> getOperator(int32_t serial);

    Return<void> setRadioPower(int32_t serial, bool on);

    Return<void> sendDtmf(int32_t serial,
            const ::android::hardware::hidl_string& s);

    Return<void> sendSms(int32_t serial, const GsmSmsMessage& message);

    Return<void> sendSMSExpectMore(int32_t serial, const GsmSmsMessage& message);

    Return<void> setupDataCall(int32_t serial,
            RadioTechnology radioTechnology,
            const DataProfileInfo& profileInfo,
            bool modemCognitive,
            bool roamingAllowed,
            bool isRoaming);

    Return<void> iccIOForApp(int32_t serial,
            const IccIo& iccIo);

    Return<void> sendUssd(int32_t serial,
            const ::android::hardware::hidl_string& ussd);

    Return<void> cancelPendingUssd(int32_t serial);

    Return<void> getClir(int32_t serial);

    Return<void> setClir(int32_t serial, int32_t status);

    Return<void> getCallForwardStatus(int32_t serial,
            const CallForwardInfo& callInfo);

    Return<void> setCallForward(int32_t serial,
            const CallForwardInfo& callInfo);

    Return<void> getCallWaiting(int32_t serial, int32_t serviceClass);

    Return<void> setCallWaiting(int32_t serial, bool enable, int32_t serviceClass);

    Return<void> acknowledgeLastIncomingGsmSms(int32_t serial,
            bool success, SmsAcknowledgeFailCause cause);

    Return<void> acceptCall(int32_t serial);

    Return<void> deactivateDataCall(int32_t serial,
            int32_t cid, bool reasonRadioShutDown);

    Return<void> getFacilityLockForApp(int32_t serial,
            const ::android::hardware::hidl_string& facility,
            const ::android::hardware::hidl_string& password,
            int32_t serviceClass,
            const ::android::hardware::hidl_string& appId);

    Return<void> setFacilityLockForApp(int32_t serial,
            const ::android::hardware::hidl_string& facility,
            bool lockState,
            const ::android::hardware::hidl_string& password,
            int32_t serviceClass,
            const ::android::hardware::hidl_string& appId);

    Return<void> setBarringPassword(int32_t serial,
            const ::android::hardware::hidl_string& facility,
            const ::android::hardware::hidl_string& oldPassword,
            const ::android::hardware::hidl_string& newPassword);

    Return<void> getNetworkSelectionMode(int32_t serial);

    Return<void> setNetworkSelectionModeAutomatic(int32_t serial);

    Return<void> setNetworkSelectionModeManual(int32_t serial,
            const ::android::hardware::hidl_string& operatorNumeric);

    Return<void> getAvailableNetworks(int32_t serial);

    Return<void> startDtmf(int32_t serial,
            const ::android::hardware::hidl_string& s);

    Return<void> stopDtmf(int32_t serial);

    Return<void> getBasebandVersion(int32_t serial);

    Return<void> separateConnection(int32_t serial, int32_t gsmIndex);

    Return<void> setMute(int32_t serial, bool enable);

    Return<void> getMute(int32_t serial);

    Return<void> getClip(int32_t serial);

    Return<void> getDataCallList(int32_t serial);

    Return<void> sendScreenState(int32_t serial, bool enable);

    Return<void> setSuppServiceNotifications(int32_t serial, bool enable);

    Return<void> writeSmsToSim(int32_t serial,
            const SmsWriteArgs& smsWriteArgs);

    Return<void> deleteSmsOnSim(int32_t serial, int32_t index);

    Return<void> setBandMode(int32_t serial, RadioBandMode mode);

    Return<void> getAvailableBandModes(int32_t serial);

    Return<void> sendEnvelope(int32_t serial,
            const ::android::hardware::hidl_string& command);

    Return<void> sendTerminalResponseToSim(int32_t serial,
            const ::android::hardware::hidl_string& commandResponse);

    Return<void> handleStkCallSetupRequestFromSim(int32_t serial, bool accept);

    Return<void> explicitCallTransfer(int32_t serial);

    Return<void> setPreferredNetworkType(int32_t serial, PreferredNetworkType nwType);

    Return<void> getPreferredNetworkType(int32_t serial);

    Return<void> getNeighboringCids(int32_t serial);

    Return<void> setLocationUpdates(int32_t serial, bool enable);

    Return<void> setCdmaSubscriptionSource(int32_t serial,
            CdmaSubscriptionSource cdmaSub);

    Return<void> setCdmaRoamingPreference(int32_t serial, CdmaRoamingType type);

    Return<void> getCdmaRoamingPreference(int32_t serial);

    Return<void> setTTYMode(int32_t serial, TtyMode mode);

    Return<void> getTTYMode(int32_t serial);

    Return<void> setPreferredVoicePrivacy(int32_t serial, bool enable);

    Return<void> getPreferredVoicePrivacy(int32_t serial);

    Return<void> sendCDMAFeatureCode(int32_t serial,
            const ::android::hardware::hidl_string& featureCode);

    Return<void> sendBurstDtmf(int32_t serial,
            const ::android::hardware::hidl_string& dtmf,
            int32_t on,
            int32_t off);

    Return<void> sendCdmaSms(int32_t serial, const CdmaSmsMessage& sms);

    Return<void> acknowledgeLastIncomingCdmaSms(int32_t serial,
            const CdmaSmsAck& smsAck);

    Return<void> getGsmBroadcastConfig(int32_t serial);

    Return<void> setGsmBroadcastConfig(int32_t serial,
            const hidl_vec<GsmBroadcastSmsConfigInfo>& configInfo);

    Return<void> setGsmBroadcastActivation(int32_t serial, bool activate);

    Return<void> getCdmaBroadcastConfig(int32_t serial);

    Return<void> setCdmaBroadcastConfig(int32_t serial,
            const hidl_vec<CdmaBroadcastSmsConfigInfo>& configInfo);

    Return<void> setCdmaBroadcastActivation(int32_t serial, bool activate);

    Return<void> getCDMASubscription(int32_t serial);

    Return<void> writeSmsToRuim(int32_t serial, const CdmaSmsWriteArgs& cdmaSms);

    Return<void> deleteSmsOnRuim(int32_t serial, int32_t index);

    Return<void> getDeviceIdentity(int32_t serial);

    Return<void> exitEmergencyCallbackMode(int32_t serial);

    Return<void> getSmscAddress(int32_t serial);

    Return<void> setSmscAddress(int32_t serial,
            const ::android::hardware::hidl_string& smsc);

    Return<void> reportSmsMemoryStatus(int32_t serial, bool available);

    Return<void> reportStkServiceIsRunning(int32_t serial);

    Return<void> getCdmaSubscriptionSource(int32_t serial);

    Return<void> requestIsimAuthentication(int32_t serial,
            const ::android::hardware::hidl_string& challenge);

    Return<void> acknowledgeIncomingGsmSmsWithPdu(int32_t serial,
            bool success,
            const ::android::hardware::hidl_string& ackPdu);

    Return<void> sendEnvelopeWithStatus(int32_t serial,
            const ::android::hardware::hidl_string& contents);

    Return<void> getVoiceRadioTechnology(int32_t serial);

    Return<void> getCellInfoList(int32_t serial);

    Return<void> setCellInfoListRate(int32_t serial, int32_t rate);

    Return<void> setInitialAttachApn(int32_t serial, const DataProfileInfo& dataProfileInfo,
            bool modemCognitive, bool isRoaming);

    Return<void> getImsRegistrationState(int32_t serial);

    Return<void> sendImsSms(int32_t serial, const ImsSmsMessage& message);

    Return<void> iccTransmitApduBasicChannel(int32_t serial, const SimApdu& message);

    Return<void> iccOpenLogicalChannel(int32_t serial,
            const ::android::hardware::hidl_string& aid);

    Return<void> iccCloseLogicalChannel(int32_t serial, int32_t channelId);

    Return<void> iccTransmitApduLogicalChannel(int32_t serial, const SimApdu& message);

    Return<void> nvReadItem(int32_t serial, NvItem itemId);

    Return<void> nvWriteItem(int32_t serial, const NvWriteItem& item);

    Return<void> nvWriteCdmaPrl(int32_t serial,
            const ::android::hardware::hidl_vec<uint8_t>& prl);

    Return<void> nvResetConfig(int32_t serial, ResetNvType resetType);

    Return<void> setUiccSubscription(int32_t serial, const SelectUiccSub& uiccSub);

    Return<void> setDataAllowed(int32_t serial, bool allow);

    Return<void> getHardwareConfig(int32_t serial);

    Return<void> requestIccSimAuthentication(int32_t serial,
            int32_t authContext,
            const ::android::hardware::hidl_string& authData,
            const ::android::hardware::hidl_string& aid);

    Return<void> setDataProfile(int32_t serial,
            const ::android::hardware::hidl_vec<DataProfileInfo>& profiles, bool isRoaming);

    Return<void> requestShutdown(int32_t serial);

    Return<void> getRadioCapability(int32_t serial);

    Return<void> setRadioCapability(int32_t serial, const RadioCapability& rc);

    Return<void> startLceService(int32_t serial, int32_t reportInterval, bool pullMode);

    Return<void> stopLceService(int32_t serial);

    Return<void> pullLceData(int32_t serial);

    Return<void> getModemActivityInfo(int32_t serial);

    Return<void> setAllowedCarriers(int32_t serial,
            bool allAllowed,
            const CarrierRestrictions& carriers);

    Return<void> getAllowedCarriers(int32_t serial);

    Return<void> sendDeviceState(int32_t serial, DeviceStateType deviceStateType, bool state);

    Return<void> setIndicationFilter(int32_t serial, int32_t indicationFilter);

    Return<void> setSimCardPower(int32_t serial, bool powerUp);

    Return<void> responseAcknowledgement();

    void checkReturnStatus(Return<void>& ret);
};

struct OemHookImpl : public IOemHook {
    int32_t mSlotId;
    sp<IOemHookResponse> mOemHookResponse;
    sp<IOemHookIndication> mOemHookIndication;

    Return<void> setResponseFunctions(
            const ::android::sp<IOemHookResponse>& oemHookResponse,
            const ::android::sp<IOemHookIndication>& oemHookIndication);

    Return<void> sendRequestRaw(int32_t serial,
            const ::android::hardware::hidl_vec<uint8_t>& data);

    Return<void> sendRequestStrings(int32_t serial,
            const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& data);
};

void memsetAndFreeStrings(int numPointers, ...) {
    va_list ap;
    va_start(ap, numPointers);
    for (int i = 0; i < numPointers; i++) {
        char *ptr = va_arg(ap, char *);
        if (ptr) {
#ifdef MEMSET_FREED
            // TODO: Should pass in the maximum length of the string
            memsetString(ptr);
#endif
            free(ptr);
        }
    }
    va_end(ap);
}

void sendErrorResponse(RequestInfo *pRI, RIL_Errno err) {
    android::Parcel p; // TODO: should delete this after translation of all commands is complete
    pRI->pCI->responseFunction(p, (int) pRI->socket_id, pRI->pCI->requestNumber,
            (int) RadioResponseType::SOLICITED, pRI->token, err, NULL, 0);
}

/**
 * Copies over src to dest. If memory allocation fails, responseFunction() is called for the
 * request with error RIL_E_NO_MEMORY.
 * Returns true on success, and false on failure.
 */
bool copyHidlStringToRil(char **dest, const hidl_string &src, RequestInfo *pRI) {
    size_t len = src.size();
    if (len == 0) {
        *dest = NULL;
        return true;
    }
    *dest = (char *) calloc(len + 1, sizeof(char));
    if (*dest == NULL) {
        RLOGE("Memory allocation failed for request %s", requestToString(pRI->pCI->requestNumber));
        sendErrorResponse(pRI, RIL_E_NO_MEMORY);
        return false;
    }
    strncpy(*dest, src, len + 1);
    return true;
}

hidl_string convertCharPtrToHidlString(const char *ptr) {
    hidl_string ret;
    if (ptr != NULL) {
        // TODO: replace this with strnlen
        ret.setToExternal(ptr, strlen(ptr));
    }
    return ret;
}

bool dispatchVoid(int serial, int slotId, int request) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }
    s_vendorFunctions->onRequest(request, NULL, 0, pRI);
    return true;
}

bool dispatchString(int serial, int slotId, int request, const char * str) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    char *pString;
    if (!copyHidlStringToRil(&pString, str, pRI)) {
        return false;
    }

    s_vendorFunctions->onRequest(request, pString, sizeof(char *), pRI);

    memsetAndFreeStrings(1, pString);
    return true;
}

bool dispatchStrings(int serial, int slotId, int request, int countStrings, ...) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    char **pStrings;
    pStrings = (char **)calloc(countStrings, sizeof(char *));
    if (pStrings == NULL) {
        RLOGE("Memory allocation failed for request %s", requestToString(request));
        sendErrorResponse(pRI, RIL_E_NO_MEMORY);
        return false;
    }
    va_list ap;
    va_start(ap, countStrings);
    for (int i = 0; i < countStrings; i++) {
        const char* str = va_arg(ap, const char *);
        if (!copyHidlStringToRil(&pStrings[i], str, pRI)) {
            va_end(ap);
            for (int j = 0; j < i; j++) {
                memsetAndFreeStrings(1, pStrings[j]);
            }
            free(pStrings);
            return false;
        }
    }
    va_end(ap);

    s_vendorFunctions->onRequest(request, pStrings, countStrings * sizeof(char *), pRI);

    if (pStrings != NULL) {
        for (int i = 0 ; i < countStrings ; i++) {
            memsetAndFreeStrings(1, pStrings[i]);
        }

#ifdef MEMSET_FREED
        memset(pStrings, 0, countStrings * sizeof(char *));
#endif
        free(pStrings);
    }
    return true;
}

bool dispatchStrings(int serial, int slotId, int request, const hidl_vec<hidl_string>& data) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    int countStrings = data.size();
    char **pStrings;
    pStrings = (char **)calloc(countStrings, sizeof(char *));
    if (pStrings == NULL) {
        RLOGE("Memory allocation failed for request %s", requestToString(request));
        sendErrorResponse(pRI, RIL_E_NO_MEMORY);
        return false;
    }

    for (int i = 0; i < countStrings; i++) {
        if (!copyHidlStringToRil(&pStrings[i], data[i], pRI)) {
            for (int j = 0; j < i; j++) {
                memsetAndFreeStrings(1, pStrings[j]);
            }
            free(pStrings);
            return false;
        }
    }

    s_vendorFunctions->onRequest(request, pStrings, countStrings * sizeof(char *), pRI);

    if (pStrings != NULL) {
        for (int i = 0 ; i < countStrings ; i++) {
            memsetAndFreeStrings(1, pStrings[i]);
        }

#ifdef MEMSET_FREED
        memset(pStrings, 0, countStrings * sizeof(char *));
#endif
        free(pStrings);
    }
    return true;
}

bool dispatchInts(int serial, int slotId, int request, int countInts, ...) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    int *pInts = (int *)calloc(countInts, sizeof(int));

    if (pInts == NULL) {
        RLOGE("Memory allocation failed for request %s", requestToString(request));
        sendErrorResponse(pRI, RIL_E_NO_MEMORY);
        return false;
    }
    va_list ap;
    va_start(ap, countInts);
    for (int i = 0; i < countInts; i++) {
        pInts[i] = va_arg(ap, int);
    }
    va_end(ap);

    s_vendorFunctions->onRequest(request, pInts, countInts * sizeof(int), pRI);

    if (pInts != NULL) {
#ifdef MEMSET_FREED
        memset(pInts, 0, countInts * sizeof(int));
#endif
        free(pInts);
    }
    return true;
}

bool dispatchCallForwardStatus(int serial, int slotId, int request,
                              const CallForwardInfo& callInfo) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    RIL_CallForwardInfo cf;
    cf.status = (int) callInfo.status;
    cf.reason = callInfo.reason;
    cf.serviceClass = callInfo.serviceClass;
    cf.toa = callInfo.toa;
    cf.timeSeconds = callInfo.timeSeconds;

    if (!copyHidlStringToRil(&cf.number, callInfo.number, pRI)) {
        return false;
    }

    s_vendorFunctions->onRequest(request, &cf, sizeof(cf), pRI);

    memsetAndFreeStrings(1, cf.number);

    return true;
}

bool dispatchRaw(int serial, int slotId, int request, const hidl_vec<uint8_t>& rawBytes) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    const uint8_t *uData = rawBytes.data();

    s_vendorFunctions->onRequest(request, (void *) uData, rawBytes.size(), pRI);

    return true;
}

bool dispatchIccApdu(int serial, int slotId, int request, const SimApdu& message) {
    RequestInfo *pRI = android::addRequestToList(serial, slotId, request);
    if (pRI == NULL) {
        return false;
    }

    RIL_SIM_APDU apdu;
    memset (&apdu, 0, sizeof(RIL_SIM_APDU));

    apdu.sessionid = message.sessionId;
    apdu.cla = message.cla;
    apdu.instruction = message.instruction;
    apdu.p1 = message.p1;
    apdu.p2 = message.p2;
    apdu.p3 = message.p3;

    if (!copyHidlStringToRil(&apdu.data, message.data, pRI)) {
        return false;
    }

    s_vendorFunctions->onRequest(request, &apdu, sizeof(apdu), pRI);

    memsetAndFreeStrings(1, apdu.data);

    return true;
}

void checkReturnStatus(int32_t slotId, Return<void>& ret) {
    if (ret.isOk() == false) {
        RLOGE("checkReturnStatus: unable to call response/indication callback");
        // Remote process hosting the callbacks must be dead. Reset the callback objects;
        // there's no other recovery to be done here. When the client process is back up, it will
        // call setResponseFunctions()

        // Caller should already hold rdlock, release that first
        // note the current counter to avoid overwriting updates made by another thread before
        // write lock is acquired.
        int counter = mCounter[slotId];
        pthread_rwlock_t *radioServiceRwlockPtr = radio::getRadioServiceRwlock(slotId);
        int ret = pthread_rwlock_unlock(radioServiceRwlockPtr);
        assert(ret == 0);

        // acquire wrlock
        ret = pthread_rwlock_wrlock(radioServiceRwlockPtr);
        assert(ret == 0);

        // make sure the counter value has not changed
        if (counter == mCounter[slotId]) {
            radioService[slotId]->mRadioResponse = NULL;
            radioService[slotId]->mRadioIndication = NULL;
            oemHookService[slotId]->mOemHookResponse = NULL;
            oemHookService[slotId]->mOemHookIndication = NULL;
            mCounter[slotId]++;
        } else {
            RLOGE("checkReturnStatus: not resetting responseFunctions as they likely "
                    "got updated on another thread");
        }

        // release wrlock
        ret = pthread_rwlock_unlock(radioServiceRwlockPtr);
        assert(ret == 0);

        // Reacquire rdlock
        ret = pthread_rwlock_rdlock(radioServiceRwlockPtr);
        assert(ret == 0);
    }
}

void RadioImpl::checkReturnStatus(Return<void>& ret) {
    ::checkReturnStatus(mSlotId, ret);
}

Return<void> RadioImpl::setResponseFunctions(
        const ::android::sp<IRadioResponse>& radioResponseParam,
        const ::android::sp<IRadioIndication>& radioIndicationParam) {
    RLOGD("RadioImpl::setResponseFunctions");

    pthread_rwlock_t *radioServiceRwlockPtr = radio::getRadioServiceRwlock(mSlotId);
    int ret = pthread_rwlock_wrlock(radioServiceRwlockPtr);
    assert(ret == 0);

    mRadioResponse = radioResponseParam;
    mRadioIndication = radioIndicationParam;
    mCounter[mSlotId]++;

    ret = pthread_rwlock_unlock(radioServiceRwlockPtr);
    assert(ret == 0);

    return Void();
}

Return<void> RadioImpl::getIccCardStatus(int32_t serial) {
    RLOGD("RadioImpl::getIccCardStatus: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_SIM_STATUS);
    return Void();
}

Return<void> RadioImpl::supplyIccPinForApp(int32_t serial, const hidl_string& pin,
        const hidl_string& aid) {
    RLOGD("RadioImpl::supplyIccPinForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_ENTER_SIM_PIN,
            2, (const char *)pin, (const char *)aid);
    return Void();
}

Return<void> RadioImpl::supplyIccPukForApp(int32_t serial, const hidl_string& puk,
                                           const hidl_string& pin, const hidl_string& aid) {
    RLOGD("RadioImpl::supplyIccPukForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_ENTER_SIM_PUK,
            3, (const char *)puk, (const char *)pin, (const char *)aid);
    return Void();
}

Return<void> RadioImpl::supplyIccPin2ForApp(int32_t serial, const hidl_string& pin2,
                                            const hidl_string& aid) {
    RLOGD("RadioImpl::supplyIccPin2ForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_ENTER_SIM_PIN2,
            2, (const char *)pin2, (const char *)aid);
    return Void();
}

Return<void> RadioImpl::supplyIccPuk2ForApp(int32_t serial, const hidl_string& puk2,
                                            const hidl_string& pin2, const hidl_string& aid) {
    RLOGD("RadioImpl::supplyIccPuk2ForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_ENTER_SIM_PUK2,
            3, (const char *)puk2, (const char *)pin2, (const char *)aid);
    return Void();
}

Return<void> RadioImpl::changeIccPinForApp(int32_t serial, const hidl_string& oldPin,
                                           const hidl_string& newPin, const hidl_string& aid) {
    RLOGD("RadioImpl::changeIccPinForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_CHANGE_SIM_PIN,
            3, (const char *)oldPin, (const char *)newPin, (const char *)aid);
    return Void();
}

Return<void> RadioImpl::changeIccPin2ForApp(int32_t serial, const hidl_string& oldPin2,
                                            const hidl_string& newPin2, const hidl_string& aid) {
    RLOGD("RadioImpl::changeIccPin2ForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_CHANGE_SIM_PIN2,
            3, (const char *)oldPin2, (const char *)newPin2, (const char *)aid);
    return Void();
}

Return<void> RadioImpl::supplyNetworkDepersonalization(int32_t serial,
                                                       const hidl_string& netPin) {
    RLOGD("RadioImpl::supplyNetworkDepersonalization: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION,
            1, (const char *)netPin);
    return Void();
}

Return<void> RadioImpl::getCurrentCalls(int32_t serial) {
    RLOGD("RadioImpl::getCurrentCalls: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_CURRENT_CALLS);
    return Void();
}

Return<void> RadioImpl::dial(int32_t serial, const Dial& dialInfo) {
    RLOGD("RadioImpl::dial: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_DIAL);
    if (pRI == NULL) {
        return Void();
    }
    RIL_Dial dial = {};
    RIL_UUS_Info uusInfo = {};
    int32_t sizeOfDial = sizeof(dial);

    if (!copyHidlStringToRil(&dial.address, dialInfo.address, pRI)) {
        return Void();
    }
    dial.clir = (int) dialInfo.clir;

    if (dialInfo.uusInfo.size() != 0) {
        uusInfo.uusType = (RIL_UUS_Type) dialInfo.uusInfo[0].uusType;
        uusInfo.uusDcs = (RIL_UUS_DCS) dialInfo.uusInfo[0].uusDcs;

        if (dialInfo.uusInfo[0].uusData.size() == 0) {
            uusInfo.uusData = NULL;
            uusInfo.uusLength = 0;
        } else {
            if (!copyHidlStringToRil(&uusInfo.uusData, dialInfo.uusInfo[0].uusData, pRI)) {
                memsetAndFreeStrings(1, dial.address);
                return Void();
            }
            uusInfo.uusLength = dialInfo.uusInfo[0].uusData.size();
        }

        dial.uusInfo = &uusInfo;
    }

    s_vendorFunctions->onRequest(RIL_REQUEST_DIAL, &dial, sizeOfDial, pRI);

    memsetAndFreeStrings(2, dial.address, uusInfo.uusData);

    return Void();
}

Return<void> RadioImpl::getImsiForApp(int32_t serial, const hidl_string& aid) {
    RLOGD("RadioImpl::getImsiForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_GET_IMSI,
            1, (const char *) aid);
    return Void();
}

Return<void> RadioImpl::hangup(int32_t serial, int32_t gsmIndex) {
    RLOGD("RadioImpl::hangup: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_HANGUP, 1, gsmIndex);
    return Void();
}

Return<void> RadioImpl::hangupWaitingOrBackground(int32_t serial) {
    RLOGD("RadioImpl::hangupWaitingOrBackground: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND);
    return Void();
}

Return<void> RadioImpl::hangupForegroundResumeBackground(int32_t serial) {
    RLOGD("RadioImpl::hangupForegroundResumeBackground: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND);
    return Void();
}

Return<void> RadioImpl::switchWaitingOrHoldingAndActive(int32_t serial) {
    RLOGD("RadioImpl::switchWaitingOrHoldingAndActive: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE);
    return Void();
}

Return<void> RadioImpl::conference(int32_t serial) {
    RLOGD("RadioImpl::conference: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_CONFERENCE);
    return Void();
}

Return<void> RadioImpl::rejectCall(int32_t serial) {
    RLOGD("RadioImpl::rejectCall: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_UDUB);
    return Void();
}

Return<void> RadioImpl::getLastCallFailCause(int32_t serial) {
    RLOGD("RadioImpl::getLastCallFailCause: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_LAST_CALL_FAIL_CAUSE);
    return Void();
}

Return<void> RadioImpl::getSignalStrength(int32_t serial) {
    RLOGD("RadioImpl::getSignalStrength: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_SIGNAL_STRENGTH);
    return Void();
}

Return<void> RadioImpl::getVoiceRegistrationState(int32_t serial) {
    RLOGD("RadioImpl::getVoiceRegistrationState: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_VOICE_REGISTRATION_STATE);
    return Void();
}

Return<void> RadioImpl::getDataRegistrationState(int32_t serial) {
    RLOGD("RadioImpl::getDataRegistrationState: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_DATA_REGISTRATION_STATE);
    return Void();
}

Return<void> RadioImpl::getOperator(int32_t serial) {
    RLOGD("RadioImpl::getOperator: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_OPERATOR);
    return Void();
}

Return<void> RadioImpl::setRadioPower(int32_t serial, bool on) {
    RLOGD("RadioImpl::setRadioPower: serial %d on %d", serial, on);
    dispatchInts(serial, mSlotId, RIL_REQUEST_RADIO_POWER, 1, BOOL_TO_INT(on));
    return Void();
}

Return<void> RadioImpl::sendDtmf(int32_t serial, const hidl_string& s) {
    RLOGD("RadioImpl::sendDtmf: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_DTMF, (const char *) s);
    return Void();
}

Return<void> RadioImpl::sendSms(int32_t serial, const GsmSmsMessage& message) {
    RLOGD("RadioImpl::sendSms: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_SEND_SMS,
            2, (const char *) message.smscPdu, (const char *) message.pdu);
    return Void();
}

Return<void> RadioImpl::sendSMSExpectMore(int32_t serial, const GsmSmsMessage& message) {
    RLOGD("RadioImpl::sendSMSExpectMore: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_SEND_SMS_EXPECT_MORE,
            2, (const char *) message.smscPdu, (const char *) message.pdu);
    return Void();
}

const char *convertMvnoTypeToString(MvnoType type) {
    switch (type) {
        case MvnoType::IMSI: return "imsi";
        case MvnoType::GID: return "gid";
        case MvnoType::SPN: return "spn";
        default: return NULL;
    }
}

Return<void> RadioImpl::setupDataCall(int32_t serial, RadioTechnology radioTechnology,
                                      const DataProfileInfo& dataProfileInfo, bool modemCognitive,
                                      bool roamingAllowed, bool isRoaming) {

    RLOGD("RadioImpl::setupDataCall: serial %d", serial);

    if (s_vendorFunctions->version >= 4 && s_vendorFunctions->version <= 14) {
        const hidl_string &protocol =
                (isRoaming ? dataProfileInfo.roamingProtocol : dataProfileInfo.protocol);
        dispatchStrings(serial, mSlotId, RIL_REQUEST_SETUP_DATA_CALL, 7,
            std::to_string((int) radioTechnology + 2).c_str(),
            std::to_string((int) dataProfileInfo.profileId).c_str(),
            dataProfileInfo.apn.c_str(),
            dataProfileInfo.user.c_str(),
            dataProfileInfo.password.c_str(),
            std::to_string((int) dataProfileInfo.authType).c_str(),
            protocol.c_str());
    } else if (s_vendorFunctions->version >= 15) {
        const char *mvnoTypeStr = convertMvnoTypeToString(dataProfileInfo.mvnoType);
        if (mvnoTypeStr == NULL) {
            RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
                    RIL_REQUEST_SETUP_DATA_CALL);
            if (pRI != NULL) {
                sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
            }
            return Void();
        }
        dispatchStrings(serial, mSlotId, RIL_REQUEST_SETUP_DATA_CALL, 15,
            std::to_string((int) radioTechnology + 2).c_str(),
            std::to_string((int) dataProfileInfo.profileId).c_str(),
            dataProfileInfo.apn.c_str(),
            dataProfileInfo.user.c_str(),
            dataProfileInfo.password.c_str(),
            std::to_string((int) dataProfileInfo.authType).c_str(),
            dataProfileInfo.protocol.c_str(),
            dataProfileInfo.roamingProtocol.c_str(),
            std::to_string(dataProfileInfo.supportedApnTypesBitmap).c_str(),
            std::to_string(dataProfileInfo.bearerBitmap).c_str(),
            BOOL_TO_INT(modemCognitive),
            std::to_string(dataProfileInfo.mtu).c_str(),
            mvnoTypeStr,
            dataProfileInfo.mvnoMatchData.c_str(),
            BOOL_TO_INT(roamingAllowed));
    } else {
        RLOGE("Unsupported RIL version %d, min version expected 4", s_vendorFunctions->version);
        RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
                RIL_REQUEST_SETUP_DATA_CALL);
        if (pRI != NULL) {
            sendErrorResponse(pRI, RIL_E_REQUEST_NOT_SUPPORTED);
        }
    }
    return Void();
}

Return<void> RadioImpl::iccIOForApp(int32_t serial, const IccIo& iccIo) {
    RLOGD("RadioImpl::iccIOForApp: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_SIM_IO);
    if (pRI == NULL) {
        return Void();
    }

    RIL_SIM_IO_v6 rilIccIo;
    rilIccIo.command = iccIo.command;
    rilIccIo.fileid = iccIo.fileId;
    if (!copyHidlStringToRil(&rilIccIo.path, iccIo.path, pRI)) {
        return Void();
    }

    rilIccIo.p1 = iccIo.p1;
    rilIccIo.p2 = iccIo.p2;
    rilIccIo.p3 = iccIo.p3;

    if (!copyHidlStringToRil(&rilIccIo.data, iccIo.data, pRI)) {
        memsetAndFreeStrings(1, rilIccIo.path);
        return Void();
    }

    if (!copyHidlStringToRil(&rilIccIo.pin2, iccIo.pin2, pRI)) {
        memsetAndFreeStrings(2, rilIccIo.path, rilIccIo.data);
        return Void();
    }

    if (!copyHidlStringToRil(&rilIccIo.aidPtr, iccIo.aid, pRI)) {
        memsetAndFreeStrings(3, rilIccIo.path, rilIccIo.data, rilIccIo.pin2);
        return Void();
    }

    s_vendorFunctions->onRequest(RIL_REQUEST_SIM_IO, &rilIccIo, sizeof(rilIccIo), pRI);

    memsetAndFreeStrings(4, rilIccIo.path, rilIccIo.data, rilIccIo.pin2, rilIccIo.aidPtr);

    return Void();
}

Return<void> RadioImpl::sendUssd(int32_t serial, const hidl_string& ussd) {
    RLOGD("RadioImpl::sendUssd: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_SEND_USSD, (const char *) ussd);
    return Void();
}

Return<void> RadioImpl::cancelPendingUssd(int32_t serial) {
    RLOGD("RadioImpl::cancelPendingUssd: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_CANCEL_USSD);
    return Void();
}

Return<void> RadioImpl::getClir(int32_t serial) {
    RLOGD("RadioImpl::getClir: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_CLIR);
    return Void();
}

Return<void> RadioImpl::setClir(int32_t serial, int32_t status) {
    RLOGD("RadioImpl::setClir: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_CLIR, 1, status);
    return Void();
}

Return<void> RadioImpl::getCallForwardStatus(int32_t serial, const CallForwardInfo& callInfo) {
    RLOGD("RadioImpl::getCallForwardStatus: serial %d", serial);
    dispatchCallForwardStatus(serial, mSlotId, RIL_REQUEST_QUERY_CALL_FORWARD_STATUS,
            callInfo);
    return Void();
}

Return<void> RadioImpl::setCallForward(int32_t serial, const CallForwardInfo& callInfo) {
    RLOGD("RadioImpl::setCallForward: serial %d", serial);
    dispatchCallForwardStatus(serial, mSlotId, RIL_REQUEST_SET_CALL_FORWARD,
            callInfo);
    return Void();
}

Return<void> RadioImpl::getCallWaiting(int32_t serial, int32_t serviceClass) {
    RLOGD("RadioImpl::getCallWaiting: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_QUERY_CALL_WAITING, 1, serviceClass);
    return Void();
}

Return<void> RadioImpl::setCallWaiting(int32_t serial, bool enable, int32_t serviceClass) {
    RLOGD("RadioImpl::setCallWaiting: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_CALL_WAITING, 2, BOOL_TO_INT(enable),
            serviceClass);
    return Void();
}

Return<void> RadioImpl::acknowledgeLastIncomingGsmSms(int32_t serial,
                                                      bool success, SmsAcknowledgeFailCause cause) {
    RLOGD("RadioImpl::acknowledgeLastIncomingGsmSms: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SMS_ACKNOWLEDGE, 2, BOOL_TO_INT(success),
            cause);
    return Void();
}

Return<void> RadioImpl::acceptCall(int32_t serial) {
    RLOGD("RadioImpl::acceptCall: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_ANSWER);
    return Void();
}

Return<void> RadioImpl::deactivateDataCall(int32_t serial,
                                           int32_t cid, bool reasonRadioShutDown) {
    RLOGD("RadioImpl::deactivateDataCall: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_DEACTIVATE_DATA_CALL,
            2, (const char *) (std::to_string(cid)).c_str(), reasonRadioShutDown ? "1" : "0");
    return Void();
}

Return<void> RadioImpl::getFacilityLockForApp(int32_t serial, const hidl_string& facility,
                                              const hidl_string& password, int32_t serviceClass,
                                              const hidl_string& appId) {
    RLOGD("RadioImpl::getFacilityLockForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_QUERY_FACILITY_LOCK,
            4, (const char *) facility, (const char *) password,
            (const char *) (std::to_string(serviceClass)).c_str(), (const char *) appId);
    return Void();
}

Return<void> RadioImpl::setFacilityLockForApp(int32_t serial, const hidl_string& facility,
                                              bool lockState, const hidl_string& password,
                                              int32_t serviceClass, const hidl_string& appId) {
    RLOGD("RadioImpl::setFacilityLockForApp: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_SET_FACILITY_LOCK,
            5, (const char *) facility, lockState ? "1" : "0", (const char *) password,
            (const char *) (std::to_string(serviceClass)).c_str(), (const char *) appId);
    return Void();
}

Return<void> RadioImpl::setBarringPassword(int32_t serial, const hidl_string& facility,
                                           const hidl_string& oldPassword,
                                           const hidl_string& newPassword) {
    RLOGD("RadioImpl::setBarringPassword: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_CHANGE_BARRING_PASSWORD,
            2, (const char *) oldPassword,  (const char *) newPassword);
    return Void();
}

Return<void> RadioImpl::getNetworkSelectionMode(int32_t serial) {
    RLOGD("RadioImpl::getNetworkSelectionMode: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE);
    return Void();
}

Return<void> RadioImpl::setNetworkSelectionModeAutomatic(int32_t serial) {
    RLOGD("RadioImpl::setNetworkSelectionModeAutomatic: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC);
    return Void();
}

Return<void> RadioImpl::setNetworkSelectionModeManual(int32_t serial,
                                                      const hidl_string& operatorNumeric) {
    RLOGD("RadioImpl::setNetworkSelectionModeManual: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL,
            (const char *) operatorNumeric);
    return Void();
}

Return<void> RadioImpl::getAvailableNetworks(int32_t serial) {
    RLOGD("RadioImpl::getAvailableNetworks: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_QUERY_AVAILABLE_NETWORKS);
    return Void();
}

Return<void> RadioImpl::startDtmf(int32_t serial, const hidl_string& s) {
    RLOGD("RadioImpl::startDtmf: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_DTMF_START,
            (const char *) s);
    return Void();
}

Return<void> RadioImpl::stopDtmf(int32_t serial) {
    RLOGD("RadioImpl::stopDtmf: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_DTMF_STOP);
    return Void();
}

Return<void> RadioImpl::getBasebandVersion(int32_t serial) {
    RLOGD("RadioImpl::getBasebandVersion: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_BASEBAND_VERSION);
    return Void();
}

Return<void> RadioImpl::separateConnection(int32_t serial, int32_t gsmIndex) {
    RLOGD("RadioImpl::separateConnection: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SEPARATE_CONNECTION, 1, gsmIndex);
    return Void();
}

Return<void> RadioImpl::setMute(int32_t serial, bool enable) {
    RLOGD("RadioImpl::setMute: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_MUTE, 1, BOOL_TO_INT(enable));
    return Void();
}

Return<void> RadioImpl::getMute(int32_t serial) {
    RLOGD("RadioImpl::getMute: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_MUTE);
    return Void();
}

Return<void> RadioImpl::getClip(int32_t serial) {
    RLOGD("RadioImpl::getClip: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_QUERY_CLIP);
    return Void();
}

Return<void> RadioImpl::getDataCallList(int32_t serial) {
    RLOGD("RadioImpl::getDataCallList: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_DATA_CALL_LIST);
    return Void();
}

Return<void> RadioImpl::sendScreenState(int32_t serial, bool enable) {
    // TODO: Completely remove this from HIDL intreface
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_SCREEN_STATE);
    sendErrorResponse(pRI, RIL_E_REQUEST_NOT_SUPPORTED);
    return Void();
}

Return<void> RadioImpl::setSuppServiceNotifications(int32_t serial, bool enable) {
    RLOGD("RadioImpl::setSuppServiceNotifications: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION, 1,
            BOOL_TO_INT(enable));
    return Void();
}

Return<void> RadioImpl::writeSmsToSim(int32_t serial, const SmsWriteArgs& smsWriteArgs) {
    RLOGD("RadioImpl::writeSmsToSim: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_WRITE_SMS_TO_SIM);
    if (pRI == NULL) {
        return Void();
    }

    RIL_SMS_WriteArgs args;
    args.status = (int) smsWriteArgs.status;

    int len;
    if (!copyHidlStringToRil(&args.pdu, smsWriteArgs.pdu, pRI)) {
        return Void();
    }

    if (!copyHidlStringToRil(&args.smsc, smsWriteArgs.smsc, pRI)) {
        memsetAndFreeStrings(1, args.pdu);
        return Void();
    }

    s_vendorFunctions->onRequest(RIL_REQUEST_WRITE_SMS_TO_SIM, &args, sizeof(args), pRI);

    memsetAndFreeStrings(2, args.smsc, args.pdu);

    return Void();
}

Return<void> RadioImpl::deleteSmsOnSim(int32_t serial, int32_t index) {
    RLOGD("RadioImpl::deleteSmsOnSim: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_DELETE_SMS_ON_SIM, 1, index);
    return Void();
}

Return<void> RadioImpl::setBandMode(int32_t serial, RadioBandMode mode) {
    RLOGD("RadioImpl::setBandMode: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_BAND_MODE, 1, mode);
    return Void();
}

Return<void> RadioImpl::getAvailableBandModes(int32_t serial) {
    RLOGD("RadioImpl::getAvailableBandModes: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE);
    return Void();
}

Return<void> RadioImpl::sendEnvelope(int32_t serial, const hidl_string& command) {
    RLOGD("RadioImpl::sendEnvelope: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND,
            (const char *) command);
    return Void();
}

Return<void> RadioImpl::sendTerminalResponseToSim(int32_t serial,
                                                  const hidl_string& commandResponse) {
    RLOGD("RadioImpl::sendTerminalResponseToSim: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE,
            (const char *) commandResponse);
    return Void();
}

Return<void> RadioImpl::handleStkCallSetupRequestFromSim(int32_t serial, bool accept) {
    RLOGD("RadioImpl::handleStkCallSetupRequestFromSim: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM,
            1, BOOL_TO_INT(accept));
    return Void();
}

Return<void> RadioImpl::explicitCallTransfer(int32_t serial) {
    RLOGD("RadioImpl::explicitCallTransfer: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_EXPLICIT_CALL_TRANSFER);
    return Void();
}

Return<void> RadioImpl::setPreferredNetworkType(int32_t serial, PreferredNetworkType nwType) {
    RLOGD("RadioImpl::setPreferredNetworkType: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE, 1, nwType);
    return Void();
}

Return<void> RadioImpl::getPreferredNetworkType(int32_t serial) {
    RLOGD("RadioImpl::getPreferredNetworkType: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE);
    return Void();
}

Return<void> RadioImpl::getNeighboringCids(int32_t serial) {
    RLOGD("RadioImpl::getNeighboringCids: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_NEIGHBORING_CELL_IDS);
    return Void();
}

Return<void> RadioImpl::setLocationUpdates(int32_t serial, bool enable) {
    RLOGD("RadioImpl::setLocationUpdates: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_LOCATION_UPDATES, 1, BOOL_TO_INT(enable));
    return Void();
}

Return<void> RadioImpl::setCdmaSubscriptionSource(int32_t serial, CdmaSubscriptionSource cdmaSub) {
    RLOGD("RadioImpl::setCdmaSubscriptionSource: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_CDMA_SET_SUBSCRIPTION_SOURCE, 1, cdmaSub);
    return Void();
}

Return<void> RadioImpl::setCdmaRoamingPreference(int32_t serial, CdmaRoamingType type) {
    RLOGD("RadioImpl::setCdmaRoamingPreference: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE, 1, type);
    return Void();
}

Return<void> RadioImpl::getCdmaRoamingPreference(int32_t serial) {
    RLOGD("RadioImpl::getCdmaRoamingPreference: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE);
    return Void();
}

Return<void> RadioImpl::setTTYMode(int32_t serial, TtyMode mode) {
    RLOGD("RadioImpl::setTTYMode: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_TTY_MODE, 1, mode);
    return Void();
}

Return<void> RadioImpl::getTTYMode(int32_t serial) {
    RLOGD("RadioImpl::getTTYMode: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_QUERY_TTY_MODE);
    return Void();
}

Return<void> RadioImpl::setPreferredVoicePrivacy(int32_t serial, bool enable) {
    RLOGD("RadioImpl::setPreferredVoicePrivacy: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE,
            1, BOOL_TO_INT(enable));
    return Void();
}

Return<void> RadioImpl::getPreferredVoicePrivacy(int32_t serial) {
    RLOGD("RadioImpl::getPreferredVoicePrivacy: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE);
    return Void();
}

Return<void> RadioImpl::sendCDMAFeatureCode(int32_t serial, const hidl_string& featureCode) {
    RLOGD("RadioImpl::sendCDMAFeatureCode: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_CDMA_FLASH,
            (const char *) featureCode);
    return Void();
}

Return<void> RadioImpl::sendBurstDtmf(int32_t serial, const hidl_string& dtmf, int32_t on,
                                      int32_t off) {
    RLOGD("RadioImpl::sendBurstDtmf: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_CDMA_BURST_DTMF,
            3, (const char *) dtmf, (const char *) (std::to_string(on)).c_str(),
            (const char *) (std::to_string(off)).c_str());
    return Void();
}

void constructCdmaSms(RIL_CDMA_SMS_Message &rcsm, const CdmaSmsMessage& sms) {
    memset(&rcsm, 0, sizeof(rcsm));

    rcsm.uTeleserviceID = sms.teleserviceId;
    rcsm.bIsServicePresent = BOOL_TO_INT(sms.isServicePresent);
    rcsm.uServicecategory = sms.serviceCategory;
    rcsm.sAddress.digit_mode = (RIL_CDMA_SMS_DigitMode) sms.address.digitMode;
    rcsm.sAddress.number_mode = (RIL_CDMA_SMS_NumberMode) sms.address.numberMode;
    rcsm.sAddress.number_type = (RIL_CDMA_SMS_NumberType) sms.address.numberType;
    rcsm.sAddress.number_plan = (RIL_CDMA_SMS_NumberPlan) sms.address.numberPlan;

    rcsm.sAddress.number_of_digits = sms.address.digits.size();
    int digitLimit= MIN((rcsm.sAddress.number_of_digits), RIL_CDMA_SMS_ADDRESS_MAX);
    for (int i = 0; i < digitLimit; i++) {
        rcsm.sAddress.digits[i] = sms.address.digits[i];
    }

    rcsm.sSubAddress.subaddressType = (RIL_CDMA_SMS_SubaddressType) sms.subAddress.subaddressType;
    rcsm.sSubAddress.odd = BOOL_TO_INT(sms.subAddress.odd);

    rcsm.sSubAddress.number_of_digits = sms.subAddress.digits.size();
    digitLimit= MIN((rcsm.sSubAddress.number_of_digits), RIL_CDMA_SMS_SUBADDRESS_MAX);
    for (int i = 0; i < digitLimit; i++) {
        rcsm.sSubAddress.digits[i] = sms.subAddress.digits[i];
    }

    rcsm.uBearerDataLen = sms.bearerData.size();
    digitLimit= MIN((rcsm.uBearerDataLen), RIL_CDMA_SMS_BEARER_DATA_MAX);
    for (int i = 0; i < digitLimit; i++) {
        rcsm.aBearerData[i] = sms.bearerData[i];
    }
}

Return<void> RadioImpl::sendCdmaSms(int32_t serial, const CdmaSmsMessage& sms) {
    RLOGD("RadioImpl::sendCdmaSms: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_CDMA_SEND_SMS);
    if (pRI == NULL) {
        return Void();
    }

    RIL_CDMA_SMS_Message rcsm;
    constructCdmaSms(rcsm, sms);

    s_vendorFunctions->onRequest(pRI->pCI->requestNumber, &rcsm, sizeof(rcsm), pRI);
    return Void();
}

Return<void> RadioImpl::acknowledgeLastIncomingCdmaSms(int32_t serial, const CdmaSmsAck& smsAck) {
    RLOGD("RadioImpl::acknowledgeLastIncomingCdmaSms: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE);
    if (pRI == NULL) {
        return Void();
    }

    RIL_CDMA_SMS_Ack rcsa = {};

    rcsa.uErrorClass = (RIL_CDMA_SMS_ErrorClass) smsAck.errorClass;
    rcsa.uSMSCauseCode = smsAck.smsCauseCode;

    s_vendorFunctions->onRequest(pRI->pCI->requestNumber, &rcsa, sizeof(rcsa), pRI);
    return Void();
}

Return<void> RadioImpl::getGsmBroadcastConfig(int32_t serial) {
    RLOGD("RadioImpl::getGsmBroadcastConfig: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG);
    return Void();
}

Return<void> RadioImpl::setGsmBroadcastConfig(int32_t serial,
                                              const hidl_vec<GsmBroadcastSmsConfigInfo>&
                                              configInfo) {
    RLOGD("RadioImpl::setGsmBroadcastConfig: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
            RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG);
    if (pRI == NULL) {
        return Void();
    }

    int num = configInfo.size();
    RIL_GSM_BroadcastSmsConfigInfo gsmBci[num];
    RIL_GSM_BroadcastSmsConfigInfo *gsmBciPtrs[num];

    for (int i = 0 ; i < num ; i++ ) {
        gsmBciPtrs[i] = &gsmBci[i];
        gsmBci[i].fromServiceId = configInfo[i].fromServiceId;
        gsmBci[i].toServiceId = configInfo[i].toServiceId;
        gsmBci[i].fromCodeScheme = configInfo[i].fromCodeScheme;
        gsmBci[i].toCodeScheme = configInfo[i].toCodeScheme;
        gsmBci[i].selected = BOOL_TO_INT(configInfo[i].selected);
    }

    s_vendorFunctions->onRequest(pRI->pCI->requestNumber, gsmBciPtrs,
            num * sizeof(RIL_GSM_BroadcastSmsConfigInfo *), pRI);
    return Void();
}

Return<void> RadioImpl::setGsmBroadcastActivation(int32_t serial, bool activate) {
    RLOGD("RadioImpl::setGsmBroadcastActivation: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION,
            1, BOOL_TO_INT(!activate));
    return Void();
}

Return<void> RadioImpl::getCdmaBroadcastConfig(int32_t serial) {
    RLOGD("RadioImpl::getCdmaBroadcastConfig: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG);
    return Void();
}

Return<void> RadioImpl::setCdmaBroadcastConfig(int32_t serial,
                                               const hidl_vec<CdmaBroadcastSmsConfigInfo>&
                                               configInfo) {
    RLOGD("RadioImpl::setCdmaBroadcastConfig: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
            RIL_REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG);
    if (pRI == NULL) {
        return Void();
    }

    int num = configInfo.size();
    RIL_CDMA_BroadcastSmsConfigInfo cdmaBci[num];
    RIL_CDMA_BroadcastSmsConfigInfo *cdmaBciPtrs[num];

    for (int i = 0 ; i < num ; i++ ) {
        cdmaBciPtrs[i] = &cdmaBci[i];
        cdmaBci[i].service_category = configInfo[i].serviceCategory;
        cdmaBci[i].language = configInfo[i].language;
        cdmaBci[i].selected = BOOL_TO_INT(configInfo[i].selected);
    }

    s_vendorFunctions->onRequest(pRI->pCI->requestNumber, cdmaBciPtrs,
            num * sizeof(RIL_CDMA_BroadcastSmsConfigInfo *), pRI);
    return Void();
}

Return<void> RadioImpl::setCdmaBroadcastActivation(int32_t serial, bool activate) {
    RLOGD("RadioImpl::setCdmaBroadcastActivation: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_CDMA_SMS_BROADCAST_ACTIVATION,
            1, BOOL_TO_INT(!activate));
    return Void();
}

Return<void> RadioImpl::getCDMASubscription(int32_t serial) {
    RLOGD("RadioImpl::getCDMASubscription: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_CDMA_SUBSCRIPTION);
    return Void();
}

Return<void> RadioImpl::writeSmsToRuim(int32_t serial, const CdmaSmsWriteArgs& cdmaSms) {
    RLOGD("RadioImpl::writeSmsToRuim: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
            RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM);
    if (pRI == NULL) {
        return Void();
    }

    RIL_CDMA_SMS_WriteArgs rcsw = {};
    rcsw.status = (int) cdmaSms.status;
    constructCdmaSms(rcsw.message, cdmaSms.message);

    s_vendorFunctions->onRequest(pRI->pCI->requestNumber, &rcsw, sizeof(rcsw), pRI);
    return Void();
}

Return<void> RadioImpl::deleteSmsOnRuim(int32_t serial, int32_t index) {
    RLOGD("RadioImpl::deleteSmsOnRuim: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM, 1, index);
    return Void();
}

Return<void> RadioImpl::getDeviceIdentity(int32_t serial) {
    RLOGD("RadioImpl::getDeviceIdentity: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_DEVICE_IDENTITY);
    return Void();
}

Return<void> RadioImpl::exitEmergencyCallbackMode(int32_t serial) {
    RLOGD("RadioImpl::exitEmergencyCallbackMode: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE);
    return Void();
}

Return<void> RadioImpl::getSmscAddress(int32_t serial) {
    RLOGD("RadioImpl::getSmscAddress: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_SMSC_ADDRESS);
    return Void();
}

Return<void> RadioImpl::setSmscAddress(int32_t serial, const hidl_string& smsc) {
    RLOGD("RadioImpl::setSmscAddress: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_SET_SMSC_ADDRESS,
            (const char *) smsc);
    return Void();
}

Return<void> RadioImpl::reportSmsMemoryStatus(int32_t serial, bool available) {
    RLOGD("RadioImpl::reportSmsMemoryStatus: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_REPORT_SMS_MEMORY_STATUS, 1,
            BOOL_TO_INT(available));
    return Void();
}

Return<void> RadioImpl::reportStkServiceIsRunning(int32_t serial) {
    RLOGD("RadioImpl::reportStkServiceIsRunning: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING);
    return Void();
}

Return<void> RadioImpl::getCdmaSubscriptionSource(int32_t serial) {
    RLOGD("RadioImpl::getCdmaSubscriptionSource: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE);
    return Void();
}

Return<void> RadioImpl::requestIsimAuthentication(int32_t serial, const hidl_string& challenge) {
    RLOGD("RadioImpl::requestIsimAuthentication: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_ISIM_AUTHENTICATION,
            (const char *) challenge);
    return Void();
}

Return<void> RadioImpl::acknowledgeIncomingGsmSmsWithPdu(int32_t serial, bool success,
                                                         const hidl_string& ackPdu) {
    RLOGD("RadioImpl::acknowledgeIncomingGsmSmsWithPdu: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU,
            2, success ? "1" : "0", (const char *) ackPdu);
    return Void();
}

Return<void> RadioImpl::sendEnvelopeWithStatus(int32_t serial, const hidl_string& contents) {
    RLOGD("RadioImpl::sendEnvelopeWithStatus: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS,
            (const char *) contents);
    return Void();
}

Return<void> RadioImpl::getVoiceRadioTechnology(int32_t serial) {
    RLOGD("RadioImpl::getVoiceRadioTechnology: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_VOICE_RADIO_TECH);
    return Void();
}

Return<void> RadioImpl::getCellInfoList(int32_t serial) {
    RLOGD("RadioImpl::getCellInfoList: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_CELL_INFO_LIST);
    return Void();
}

Return<void> RadioImpl::setCellInfoListRate(int32_t serial, int32_t rate) {
    RLOGD("RadioImpl::setCellInfoListRate: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE, 1, rate);
    return Void();
}

Return<void> RadioImpl::setInitialAttachApn(int32_t serial, const DataProfileInfo& dataProfileInfo,
                                            bool modemCognitive, bool isRoaming) {
    RLOGD("RadioImpl::setInitialAttachApn: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
            RIL_REQUEST_SET_INITIAL_ATTACH_APN);
    if (pRI == NULL) {
        return Void();
    }

    if (s_vendorFunctions->version <= 14) {
        RIL_InitialAttachApn iaa = {};

        if (!copyHidlStringToRil(&iaa.apn, dataProfileInfo.apn, pRI)) {
            return Void();
        }

        const hidl_string &protocol =
                (isRoaming ? dataProfileInfo.roamingProtocol : dataProfileInfo.protocol);

        if (!copyHidlStringToRil(&iaa.protocol, protocol, pRI)) {
            return Void();
        }
        iaa.authtype = (int) dataProfileInfo.authType;
        if (!copyHidlStringToRil(&iaa.username, dataProfileInfo.user, pRI)) {
            return Void();
        }
        if (!copyHidlStringToRil(&iaa.password, dataProfileInfo.password, pRI)) {
            return Void();
        }

        s_vendorFunctions->onRequest(RIL_REQUEST_SET_INITIAL_ATTACH_APN, &iaa, sizeof(iaa), pRI);

        memsetAndFreeStrings(4, iaa.apn, iaa.protocol, iaa.username, iaa.password);
    } else {
        RIL_InitialAttachApn_v15 iaa = {};

        if (!copyHidlStringToRil(&iaa.apn, dataProfileInfo.apn, pRI)) {
            return Void();
        }
        if (!copyHidlStringToRil(&iaa.protocol, dataProfileInfo.protocol, pRI)) {
            return Void();
        }
        if (!copyHidlStringToRil(&iaa.roamingProtocol, dataProfileInfo.roamingProtocol, pRI)) {
            return Void();
        }
        iaa.authtype = (int) dataProfileInfo.authType;
        if (!copyHidlStringToRil(&iaa.username, dataProfileInfo.user, pRI)) {
            return Void();
        }
        if (!copyHidlStringToRil(&iaa.password, dataProfileInfo.password, pRI)) {
            return Void();
        }
        iaa.supportedTypesBitmask = dataProfileInfo.supportedApnTypesBitmap;
        iaa.bearerBitmask = dataProfileInfo.bearerBitmap;
        iaa.modemCognitive = BOOL_TO_INT(modemCognitive);
        iaa.mtu = dataProfileInfo.mtu;

        // Note that there is no need for memory allocation/free.
        iaa.mvnoType = (char *) convertMvnoTypeToString(dataProfileInfo.mvnoType);
        if (iaa.mvnoType == NULL) {
            sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
            return Void();
        }

        if (!copyHidlStringToRil(&iaa.mvnoMatchData, dataProfileInfo.mvnoMatchData, pRI)) {
            return Void();
        }

        s_vendorFunctions->onRequest(RIL_REQUEST_SET_INITIAL_ATTACH_APN, &iaa, sizeof(iaa), pRI);

        memsetAndFreeStrings(6, iaa.apn, iaa.protocol, iaa.roamingProtocol, iaa.username,
                iaa.password, iaa.mvnoMatchData);
    }

    return Void();
}

Return<void> RadioImpl::getImsRegistrationState(int32_t serial) {
    RLOGD("RadioImpl::getImsRegistrationState: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_IMS_REGISTRATION_STATE);
    return Void();
}

bool dispatchImsGsmSms(const ImsSmsMessage& message, RequestInfo *pRI) {
    RIL_IMS_SMS_Message rism = {};
    char **pStrings;
    int countStrings = 2;
    int dataLen = sizeof(char *) * countStrings;

    rism.tech = RADIO_TECH_3GPP;
    rism.retry = BOOL_TO_INT(message.retry);
    rism.messageRef = message.messageRef;

    if (message.gsmMessage.size() != 1) {
        RLOGE("dispatchImsGsmSms: Invalid len %s", requestToString(pRI->pCI->requestNumber));
        sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
        return false;
    }

    pStrings = (char **)calloc(countStrings, sizeof(char *));
    if (pStrings == NULL) {
        RLOGE("dispatchImsGsmSms: Memory allocation failed for request %s",
                requestToString(pRI->pCI->requestNumber));
        sendErrorResponse(pRI, RIL_E_NO_MEMORY);
        return false;
    }

    if (!copyHidlStringToRil(&pStrings[0], message.gsmMessage[0].smscPdu, pRI)) {
#ifdef MEMSET_FREED
        memset(pStrings, 0, datalen);
#endif
        free(pStrings);
        return false;
    }

    if (!copyHidlStringToRil(&pStrings[1], message.gsmMessage[0].pdu, pRI)) {
        memsetAndFreeStrings(1, pStrings[0]);
#ifdef MEMSET_FREED
        memset(pStrings, 0, datalen);
#endif
        free(pStrings);
        return false;
    }

    rism.message.gsmMessage = pStrings;
    s_vendorFunctions->onRequest(pRI->pCI->requestNumber, &rism, sizeof(RIL_RadioTechnologyFamily) +
            sizeof(uint8_t) + sizeof(int32_t) + dataLen, pRI);

    for (int i = 0 ; i < countStrings ; i++) {
        memsetAndFreeStrings(1, pStrings[i]);
    }

#ifdef MEMSET_FREED
    memset(pStrings, 0, datalen);
#endif
    free(pStrings);

    return true;
}

bool dispatchImsCdmaSms(const ImsSmsMessage& message, RequestInfo *pRI) {
    RIL_IMS_SMS_Message rism;
    RIL_CDMA_SMS_Message rcsm;

    if (message.cdmaMessage.size() != 1) {
        RLOGE("dispatchImsCdmaSms: Invalid len %s", requestToString(pRI->pCI->requestNumber));
        sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
        return false;
    }

    rism.tech = RADIO_TECH_3GPP2;
    rism.retry = BOOL_TO_INT(message.retry);
    rism.messageRef = message.messageRef;
    rism.message.cdmaMessage = &rcsm;

    constructCdmaSms(rcsm, message.cdmaMessage[0]);

    s_vendorFunctions->onRequest(pRI->pCI->requestNumber, &rism, sizeof(RIL_RadioTechnologyFamily) +
            sizeof(uint8_t) + sizeof(int32_t) + sizeof(rcsm), pRI);

    return true;
}

Return<void> RadioImpl::sendImsSms(int32_t serial, const ImsSmsMessage& message) {
    RLOGD("RadioImpl::sendImsSms: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_IMS_SEND_SMS);
    if (pRI == NULL) {
        return Void();
    }

    RIL_RadioTechnologyFamily format = (RIL_RadioTechnologyFamily) message.tech;

    if (RADIO_TECH_3GPP == format) {
        dispatchImsGsmSms(message, pRI);
    } else if (RADIO_TECH_3GPP2 == format) {
        dispatchImsCdmaSms(message, pRI);
    } else {
        RLOGE("RadioImpl::sendImsSms: Invalid radio tech %s",
                requestToString(pRI->pCI->requestNumber));
        sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
    }
    return Void();
}

Return<void> RadioImpl::iccTransmitApduBasicChannel(int32_t serial, const SimApdu& message) {
    RLOGD("RadioImpl::iccTransmitApduBasicChannel: serial %d", serial);
    dispatchIccApdu(serial, mSlotId, RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC, message);
    return Void();
}

Return<void> RadioImpl::iccOpenLogicalChannel(int32_t serial, const hidl_string& aid) {
    RLOGD("RadioImpl::iccOpenLogicalChannel: serial %d", serial);
    dispatchString(serial, mSlotId, RIL_REQUEST_SIM_OPEN_CHANNEL,
            (const char *) aid);
    return Void();
}

Return<void> RadioImpl::iccCloseLogicalChannel(int32_t serial, int32_t channelId) {
    RLOGD("RadioImpl::iccCloseLogicalChannel: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SIM_CLOSE_CHANNEL, 1, channelId);
    return Void();
}

Return<void> RadioImpl::iccTransmitApduLogicalChannel(int32_t serial, const SimApdu& message) {
    RLOGD("RadioImpl::iccTransmitApduLogicalChannel: serial %d", serial);
    dispatchIccApdu(serial, mSlotId, RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL, message);
    return Void();
}

Return<void> RadioImpl::nvReadItem(int32_t serial, NvItem itemId) {
    RLOGD("RadioImpl::nvReadItem: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_NV_READ_ITEM);
    if (pRI == NULL) {
        return Void();
    }

    RIL_NV_ReadItem nvri;
    memset (&nvri, 0, sizeof(nvri));
    nvri.itemID = (RIL_NV_Item) itemId;

    s_vendorFunctions->onRequest(pRI->pCI->requestNumber, &nvri, sizeof(nvri), pRI);
    return Void();
}

Return<void> RadioImpl::nvWriteItem(int32_t serial, const NvWriteItem& item) {
    RLOGD("RadioImpl::nvWriteItem: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_NV_WRITE_ITEM);
    if (pRI == NULL) {
        return Void();
    }

    RIL_NV_WriteItem nvwi;
    memset (&nvwi, 0, sizeof(nvwi));

    nvwi.itemID = (RIL_NV_Item) item.itemId;

    if (!copyHidlStringToRil(&nvwi.value, item.value, pRI)) {
        return Void();
    }

    s_vendorFunctions->onRequest(pRI->pCI->requestNumber, &nvwi, sizeof(nvwi), pRI);

    memsetAndFreeStrings(1, nvwi.value);
    return Void();
}

Return<void> RadioImpl::nvWriteCdmaPrl(int32_t serial, const hidl_vec<uint8_t>& prl) {
    RLOGD("RadioImpl::nvWriteCdmaPrl: serial %d", serial);
    dispatchRaw(serial, mSlotId, RIL_REQUEST_NV_WRITE_CDMA_PRL, prl);
    return Void();
}

Return<void> RadioImpl::nvResetConfig(int32_t serial, ResetNvType resetType) {
    RLOGD("RadioImpl::nvResetConfig: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_NV_RESET_CONFIG, 1, (int) resetType);
    return Void();
}

Return<void> RadioImpl::setUiccSubscription(int32_t serial, const SelectUiccSub& uiccSub) {
    RLOGD("RadioImpl::setUiccSubscription: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
            RIL_REQUEST_SET_UICC_SUBSCRIPTION);
    if (pRI == NULL) {
        return Void();
    }

    RIL_SelectUiccSub rilUiccSub = {};

    rilUiccSub.slot = uiccSub.slot;
    rilUiccSub.app_index = uiccSub.appIndex;
    rilUiccSub.sub_type = (RIL_SubscriptionType) uiccSub.subType;
    rilUiccSub.act_status = (RIL_UiccSubActStatus) uiccSub.actStatus;

    s_vendorFunctions->onRequest(pRI->pCI->requestNumber, &rilUiccSub, sizeof(rilUiccSub), pRI);
    return Void();
}

Return<void> RadioImpl::setDataAllowed(int32_t serial, bool allow) {
    RLOGD("RadioImpl::setDataAllowed: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_ALLOW_DATA, 1, BOOL_TO_INT(allow));
    return Void();
}

Return<void> RadioImpl::getHardwareConfig(int32_t serial) {
    RLOGD("RadioImpl::getHardwareConfig: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_HARDWARE_CONFIG);
    return Void();
}

Return<void> RadioImpl::requestIccSimAuthentication(int32_t serial, int32_t authContext,
        const hidl_string& authData, const hidl_string& aid) {
    RLOGD("RadioImpl::requestIccSimAuthentication: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_SIM_AUTHENTICATION);
    if (pRI == NULL) {
        return Void();
    }

    RIL_SimAuthentication pf;
    memset (&pf, 0, sizeof(pf));

    pf.authContext = authContext;

    int len;
    if (!copyHidlStringToRil(&pf.authData, authData, pRI)) {
        return Void();
    }

    if (!copyHidlStringToRil(&pf.aid, aid, pRI)) {
        memsetAndFreeStrings(1, pf.authData);
        return Void();
    }

    s_vendorFunctions->onRequest(pRI->pCI->requestNumber, &pf, sizeof(pf), pRI);

    memsetAndFreeStrings(2, pf.authData, pf.aid);
    return Void();
}

/**
 * @param numProfiles number of data profile
 * @param dataProfiles the pointer to the actual data profiles. The acceptable type is
          RIL_DataProfileInfo or RIL_DataProfileInfo_v15.
 * @param dataProfilePtrs the pointer to the pointers that point to each data profile structure
 * @param numfields number of string-type member in the data profile structure
 * @param ... the variadic parameters are pointers to each string-type member
 **/
template <typename T>
void freeSetDataProfileData(int numProfiles, T *dataProfiles, T **dataProfilePtrs,
                            int numfields, ...) {
    va_list args;
    va_start(args, numfields);

    // Iterate through each string-type field that need to be free.
    for (int i = 0; i < numfields; i++) {
        // Iterate through each data profile and free that specific string-type field.
        // The type 'char *T::*' is a type of pointer to a 'char *' member inside T structure.
        char *T::*ptr = va_arg(args, char *T::*);
        for (int j = 0; j < numProfiles; j++) {
            memsetAndFreeStrings(1, dataProfiles[j].*ptr);
        }
    }

    va_end(args);

#ifdef MEMSET_FREED
    memset(dataProfiles, 0, numProfiles * sizeof(T));
    memset(dataProfilePtrs, 0, numProfiles * sizeof(T *));
#endif
    free(dataProfiles);
    free(dataProfilePtrs);
}

Return<void> RadioImpl::setDataProfile(int32_t serial, const hidl_vec<DataProfileInfo>& profiles,
                                       bool isRoaming) {
    RLOGD("RadioImpl::setDataProfile: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_SET_DATA_PROFILE);
    if (pRI == NULL) {
        return Void();
    }

    size_t num = profiles.size();
    bool success = false;

    if (s_vendorFunctions->version <= 14) {

        RIL_DataProfileInfo *dataProfiles =
            (RIL_DataProfileInfo *) calloc(num, sizeof(RIL_DataProfileInfo));

        if (dataProfiles == NULL) {
            RLOGE("Memory allocation failed for request %s",
                    requestToString(pRI->pCI->requestNumber));
            sendErrorResponse(pRI, RIL_E_NO_MEMORY);
            return Void();
        }

        RIL_DataProfileInfo **dataProfilePtrs =
            (RIL_DataProfileInfo **) calloc(num, sizeof(RIL_DataProfileInfo *));
        if (dataProfilePtrs == NULL) {
            RLOGE("Memory allocation failed for request %s",
                    requestToString(pRI->pCI->requestNumber));
            free(dataProfiles);
            sendErrorResponse(pRI, RIL_E_NO_MEMORY);
            return Void();
        }

        for (size_t i = 0; i < num; i++) {
            dataProfilePtrs[i] = &dataProfiles[i];

            success = copyHidlStringToRil(&dataProfiles[i].apn, profiles[i].apn, pRI);

            const hidl_string &protocol =
                    (isRoaming ? profiles[i].roamingProtocol : profiles[i].protocol);

            if (success && !copyHidlStringToRil(&dataProfiles[i].protocol, protocol, pRI)) {
                success = false;
            }

            if (success && !copyHidlStringToRil(&dataProfiles[i].user, profiles[i].user, pRI)) {
                success = false;
            }
            if (success && !copyHidlStringToRil(&dataProfiles[i].password, profiles[i].password,
                    pRI)) {
                success = false;
            }

            if (!success) {
                freeSetDataProfileData(num, dataProfiles, dataProfilePtrs, 4,
                    &RIL_DataProfileInfo::apn, &RIL_DataProfileInfo::protocol,
                    &RIL_DataProfileInfo::user, &RIL_DataProfileInfo::password);
                return Void();
            }

            dataProfiles[i].profileId = (RIL_DataProfile) profiles[i].profileId;
            dataProfiles[i].authType = (int) profiles[i].authType;
            dataProfiles[i].type = (int) profiles[i].type;
            dataProfiles[i].maxConnsTime = profiles[i].maxConnsTime;
            dataProfiles[i].maxConns = profiles[i].maxConns;
            dataProfiles[i].waitTime = profiles[i].waitTime;
            dataProfiles[i].enabled = BOOL_TO_INT(profiles[i].enabled);
        }

        s_vendorFunctions->onRequest(RIL_REQUEST_SET_DATA_PROFILE, dataProfilePtrs,
                num * sizeof(RIL_DataProfileInfo *), pRI);

        freeSetDataProfileData(num, dataProfiles, dataProfilePtrs, 4,
                &RIL_DataProfileInfo::apn, &RIL_DataProfileInfo::protocol,
                &RIL_DataProfileInfo::user, &RIL_DataProfileInfo::password);
    } else {
        RIL_DataProfileInfo_v15 *dataProfiles =
            (RIL_DataProfileInfo_v15 *) calloc(num, sizeof(RIL_DataProfileInfo_v15));

        if (dataProfiles == NULL) {
            RLOGE("Memory allocation failed for request %s",
                    requestToString(pRI->pCI->requestNumber));
            sendErrorResponse(pRI, RIL_E_NO_MEMORY);
            return Void();
        }

        RIL_DataProfileInfo_v15 **dataProfilePtrs =
            (RIL_DataProfileInfo_v15 **) calloc(num, sizeof(RIL_DataProfileInfo_v15 *));
        if (dataProfilePtrs == NULL) {
            RLOGE("Memory allocation failed for request %s",
                    requestToString(pRI->pCI->requestNumber));
            free(dataProfiles);
            sendErrorResponse(pRI, RIL_E_NO_MEMORY);
            return Void();
        }

        for (size_t i = 0; i < num; i++) {
            dataProfilePtrs[i] = &dataProfiles[i];

            success = copyHidlStringToRil(&dataProfiles[i].apn, profiles[i].apn, pRI);
            if (success && !copyHidlStringToRil(&dataProfiles[i].protocol, profiles[i].protocol,
                    pRI)) {
                success = false;
            }
            if (success && !copyHidlStringToRil(&dataProfiles[i].roamingProtocol,
                    profiles[i].roamingProtocol, pRI)) {
                success = false;
            }
            if (success && !copyHidlStringToRil(&dataProfiles[i].user, profiles[i].user, pRI)) {
                success = false;
            }
            if (success && !copyHidlStringToRil(&dataProfiles[i].password, profiles[i].password,
                    pRI)) {
                success = false;
            }

            if (success && !copyHidlStringToRil(&dataProfiles[i].mvnoMatchData,
                    profiles[i].mvnoMatchData, pRI)) {
                success = false;
            }

            if (success) {
                dataProfiles[i].mvnoType = (char *) convertMvnoTypeToString(profiles[i].mvnoType);
                if (dataProfiles[i].mvnoType == NULL) {
                    sendErrorResponse(pRI, RIL_E_INVALID_ARGUMENTS);
                    success = false;
                }
            }

            if (!success) {
                freeSetDataProfileData(num, dataProfiles, dataProfilePtrs, 6,
                    &RIL_DataProfileInfo_v15::apn, &RIL_DataProfileInfo_v15::protocol,
                    &RIL_DataProfileInfo_v15::roamingProtocol, &RIL_DataProfileInfo_v15::user,
                    &RIL_DataProfileInfo_v15::password, &RIL_DataProfileInfo_v15::mvnoMatchData);
                return Void();
            }

            dataProfiles[i].profileId = (RIL_DataProfile) profiles[i].profileId;
            dataProfiles[i].authType = (int) profiles[i].authType;
            dataProfiles[i].type = (int) profiles[i].type;
            dataProfiles[i].maxConnsTime = profiles[i].maxConnsTime;
            dataProfiles[i].maxConns = profiles[i].maxConns;
            dataProfiles[i].waitTime = profiles[i].waitTime;
            dataProfiles[i].enabled = BOOL_TO_INT(profiles[i].enabled);
            dataProfiles[i].supportedTypesBitmask = profiles[i].supportedApnTypesBitmap;
            dataProfiles[i].bearerBitmask = profiles[i].bearerBitmap;
            dataProfiles[i].mtu = profiles[i].mtu;
        }

        s_vendorFunctions->onRequest(RIL_REQUEST_SET_DATA_PROFILE, dataProfilePtrs,
                num * sizeof(RIL_DataProfileInfo_v15 *), pRI);

        freeSetDataProfileData(num, dataProfiles, dataProfilePtrs, 6,
                &RIL_DataProfileInfo_v15::apn, &RIL_DataProfileInfo_v15::protocol,
                &RIL_DataProfileInfo_v15::roamingProtocol, &RIL_DataProfileInfo_v15::user,
                &RIL_DataProfileInfo_v15::password, &RIL_DataProfileInfo_v15::mvnoMatchData);
    }

    return Void();
}

Return<void> RadioImpl::requestShutdown(int32_t serial) {
    RLOGD("RadioImpl::requestShutdown: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_SHUTDOWN);
    return Void();
}

Return<void> RadioImpl::getRadioCapability(int32_t serial) {
    RLOGD("RadioImpl::getRadioCapability: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_RADIO_CAPABILITY);
    return Void();
}

Return<void> RadioImpl::setRadioCapability(int32_t serial, const RadioCapability& rc) {
    RLOGD("RadioImpl::setRadioCapability: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_SET_RADIO_CAPABILITY);
    if (pRI == NULL) {
        return Void();
    }

    RIL_RadioCapability rilRc;
    memset (&rilRc, 0, sizeof(rilRc));

    // TODO : set rilRc.version using HIDL version ?
    rilRc.session = rc.session;
    rilRc.phase = (int) rc.phase;
    rilRc.rat = (int) rc.raf;
    rilRc.status = (int) rc.status;
    strncpy(rilRc.logicalModemUuid, (const char *) rc.logicalModemUuid, MAX_UUID_LENGTH);

    s_vendorFunctions->onRequest(pRI->pCI->requestNumber, &rilRc, sizeof(rilRc), pRI);

    return Void();
}

Return<void> RadioImpl::startLceService(int32_t serial, int32_t reportInterval, bool pullMode) {
    RLOGD("RadioImpl::startLceService: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_START_LCE, 2, reportInterval,
            BOOL_TO_INT(pullMode));
    return Void();
}

Return<void> RadioImpl::stopLceService(int32_t serial) {
    RLOGD("RadioImpl::stopLceService: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_STOP_LCE);
    return Void();
}

Return<void> RadioImpl::pullLceData(int32_t serial) {
    RLOGD("RadioImpl::pullLceData: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_PULL_LCEDATA);
    return Void();
}

Return<void> RadioImpl::getModemActivityInfo(int32_t serial) {
    RLOGD("RadioImpl::getModemActivityInfo: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_ACTIVITY_INFO);
    return Void();
}

Return<void> RadioImpl::setAllowedCarriers(int32_t serial, bool allAllowed,
                                           const CarrierRestrictions& carriers) {
    RLOGD("RadioImpl::setAllowedCarriers: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
            RIL_REQUEST_SET_CARRIER_RESTRICTIONS);
    if (pRI == NULL) {
        return Void();
    }

    RIL_CarrierRestrictions cr = {};
    RIL_Carrier *allowedCarriers = NULL;
    RIL_Carrier *excludedCarriers = NULL;

    cr.len_allowed_carriers = carriers.allowedCarriers.size();
    allowedCarriers = (RIL_Carrier *)calloc(cr.len_allowed_carriers, sizeof(RIL_Carrier));
    if (allowedCarriers == NULL) {
        RLOGE("RadioImpl::setAllowedCarriers: Memory allocation failed for request %s",
                requestToString(pRI->pCI->requestNumber));
        sendErrorResponse(pRI, RIL_E_NO_MEMORY);
        return Void();
    }
    cr.allowed_carriers = allowedCarriers;

    cr.len_excluded_carriers = carriers.excludedCarriers.size();
    excludedCarriers = (RIL_Carrier *)calloc(cr.len_excluded_carriers, sizeof(RIL_Carrier));
    if (excludedCarriers == NULL) {
        RLOGE("RadioImpl::setAllowedCarriers: Memory allocation failed for request %s",
                requestToString(pRI->pCI->requestNumber));
        sendErrorResponse(pRI, RIL_E_NO_MEMORY);
#ifdef MEMSET_FREED
        memset(allowedCarriers, 0, cr.len_allowed_carriers * sizeof(RIL_Carrier));
#endif
        free(allowedCarriers);
        return Void();
    }
    cr.excluded_carriers = excludedCarriers;

    for (int i = 0; i < cr.len_allowed_carriers; i++) {
        allowedCarriers[i].mcc = (const char *) carriers.allowedCarriers[i].mcc;
        allowedCarriers[i].mnc = (const char *) carriers.allowedCarriers[i].mnc;
        allowedCarriers[i].match_type = (RIL_CarrierMatchType) carriers.allowedCarriers[i].matchType;
        allowedCarriers[i].match_data = (const char *) carriers.allowedCarriers[i].matchData;
    }

    for (int i = 0; i < cr.len_excluded_carriers; i++) {
        excludedCarriers[i].mcc = (const char *) carriers.excludedCarriers[i].mcc;
        excludedCarriers[i].mnc = (const char *) carriers.excludedCarriers[i].mnc;
        excludedCarriers[i].match_type =
                (RIL_CarrierMatchType) carriers.excludedCarriers[i].matchType;
        excludedCarriers[i].match_data = (const char *) carriers.excludedCarriers[i].matchData;
    }

    s_vendorFunctions->onRequest(pRI->pCI->requestNumber, &cr, sizeof(RIL_CarrierRestrictions), pRI);

#ifdef MEMSET_FREED
    memset(allowedCarriers, 0, cr.len_allowed_carriers * sizeof(RIL_Carrier));
    memset(excludedCarriers, 0, cr.len_excluded_carriers * sizeof(RIL_Carrier));
#endif
    free(allowedCarriers);
    free(excludedCarriers);
    return Void();
}

Return<void> RadioImpl::getAllowedCarriers(int32_t serial) {
    RLOGD("RadioImpl::getAllowedCarriers: serial %d", serial);
    dispatchVoid(serial, mSlotId, RIL_REQUEST_GET_CARRIER_RESTRICTIONS);
    return Void();
}

Return<void> RadioImpl::sendDeviceState(int32_t serial, DeviceStateType deviceStateType,
                                        bool state) {
    RLOGD("RadioImpl::sendDeviceState: serial %d", serial);
    if (s_vendorFunctions->version < 15) {
        if (deviceStateType ==  DeviceStateType::LOW_DATA_EXPECTED) {
            RLOGD("RadioImpl::sendDeviceState: calling screen state %d", BOOL_TO_INT(!state));
            dispatchInts(serial, mSlotId, RIL_REQUEST_SCREEN_STATE, 1, BOOL_TO_INT(!state));
        } else {
            RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
                    RIL_REQUEST_SEND_DEVICE_STATE);
            sendErrorResponse(pRI, RIL_E_REQUEST_NOT_SUPPORTED);
        }
        return Void();
    }
    dispatchInts(serial, mSlotId, RIL_REQUEST_SEND_DEVICE_STATE, 2, (int) deviceStateType,
            BOOL_TO_INT(state));
    return Void();
}

Return<void> RadioImpl::setIndicationFilter(int32_t serial, int32_t indicationFilter) {
    RLOGD("RadioImpl::setIndicationFilter: serial %d", serial);
    if (s_vendorFunctions->version < 15) {
        RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
                RIL_REQUEST_SET_UNSOLICITED_RESPONSE_FILTER);
        sendErrorResponse(pRI, RIL_E_REQUEST_NOT_SUPPORTED);
        return Void();
    }
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_UNSOLICITED_RESPONSE_FILTER, 1, indicationFilter);
    return Void();
}

Return<void> RadioImpl::setSimCardPower(int32_t serial, bool powerUp) {
    RLOGD("RadioImpl::setSimCardPower: serial %d", serial);
    dispatchInts(serial, mSlotId, RIL_REQUEST_SET_SIM_CARD_POWER, 1, BOOL_TO_INT(powerUp));
    return Void();
}

Return<void> RadioImpl::responseAcknowledgement() {
    android::releaseWakeLock();
    return Void();
}

Return<void> OemHookImpl::setResponseFunctions(
        const ::android::sp<IOemHookResponse>& oemHookResponseParam,
        const ::android::sp<IOemHookIndication>& oemHookIndicationParam) {
    RLOGD("OemHookImpl::setResponseFunctions");

    pthread_rwlock_t *radioServiceRwlockPtr = radio::getRadioServiceRwlock(mSlotId);
    int ret = pthread_rwlock_wrlock(radioServiceRwlockPtr);
    assert(ret == 0);

    mOemHookResponse = oemHookResponseParam;
    mOemHookIndication = oemHookIndicationParam;
    mCounter[mSlotId]++;

    ret = pthread_rwlock_unlock(radioServiceRwlockPtr);
    assert(ret == 0);

    return Void();
}

Return<void> OemHookImpl::sendRequestRaw(int32_t serial, const hidl_vec<uint8_t>& data) {
    RLOGD("OemHookImpl::sendRequestRaw: serial %d", serial);
    dispatchRaw(serial, mSlotId, RIL_REQUEST_OEM_HOOK_RAW, data);
    return Void();
}

Return<void> OemHookImpl::sendRequestStrings(int32_t serial,
        const hidl_vec<hidl_string>& data) {
    RLOGD("OemHookImpl::sendRequestStrings: serial %d", serial);
    dispatchStrings(serial, mSlotId, RIL_REQUEST_OEM_HOOK_STRINGS, data);
    return Void();
}

/***************************************************************************************************
 * RESPONSE FUNCTIONS
 * Functions above are used for requests going from framework to vendor code. The ones below are
 * responses for those requests coming back from the vendor code.
 **************************************************************************************************/

void radio::acknowledgeRequest(int slotId, int serial) {
    if (radioService[slotId]->mRadioResponse != NULL) {
        Return<void> retStatus = radioService[slotId]->mRadioResponse->acknowledgeRequest(serial);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::acknowledgeRequest: radioService[%d]->mRadioResponse == NULL", slotId);
    }
}

void populateResponseInfo(RadioResponseInfo& responseInfo, int serial, int responseType,
                         RIL_Errno e) {
    responseInfo.serial = serial;
    switch (responseType) {
        case RESPONSE_SOLICITED:
            responseInfo.type = RadioResponseType::SOLICITED;
            break;
        case RESPONSE_SOLICITED_ACK_EXP:
            responseInfo.type = RadioResponseType::SOLICITED_ACK_EXP;
            break;
    }
    responseInfo.error = (RadioError) e;
}

int responseInt(RadioResponseInfo& responseInfo, int serial, int responseType, RIL_Errno e,
               void *response, size_t responseLen) {
    populateResponseInfo(responseInfo, serial, responseType, e);
    int ret = -1;

    if (response == NULL || responseLen != sizeof(int)) {
        RLOGE("responseInt: Invalid response");
        if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
    } else {
        int *p_int = (int *) response;
        ret = p_int[0];
    }
    return ret;
}

int radio::getIccCardStatusResponse(android::Parcel &p, int slotId, int requestNumber,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        CardStatus cardStatus = {};
        if (response == NULL || responseLen != sizeof(RIL_CardStatus_v6)) {
            RLOGE("radio::getIccCardStatusResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            RIL_CardStatus_v6 *p_cur = ((RIL_CardStatus_v6 *) response);
            cardStatus.cardState = (CardState) p_cur->card_state;
            cardStatus.universalPinState = (PinState) p_cur->universal_pin_state;
            cardStatus.gsmUmtsSubscriptionAppIndex = p_cur->gsm_umts_subscription_app_index;
            cardStatus.cdmaSubscriptionAppIndex = p_cur->cdma_subscription_app_index;
            cardStatus.imsSubscriptionAppIndex = p_cur->ims_subscription_app_index;

            RIL_AppStatus *rilAppStatus = p_cur->applications;
            cardStatus.applications.resize(p_cur->num_applications);
            AppStatus *appStatus = cardStatus.applications.data();
            RLOGD("radio::getIccCardStatusResponse: num_applications %d", p_cur->num_applications);
            for (int i = 0; i < p_cur->num_applications; i++) {
                appStatus[i].appType = (AppType) rilAppStatus[i].app_type;
                appStatus[i].appState = (AppState) rilAppStatus[i].app_state;
                appStatus[i].persoSubstate = (PersoSubstate) rilAppStatus[i].perso_substate;
                appStatus[i].aidPtr = convertCharPtrToHidlString(rilAppStatus[i].aid_ptr);
                appStatus[i].appLabelPtr = convertCharPtrToHidlString(
                        rilAppStatus[i].app_label_ptr);
                appStatus[i].pin1Replaced = rilAppStatus[i].pin1_replaced;
                appStatus[i].pin1 = (PinState) rilAppStatus[i].pin1;
                appStatus[i].pin2 = (PinState) rilAppStatus[i].pin2;
            }
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                getIccCardStatusResponse(responseInfo, cardStatus);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getIccCardStatusResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::supplyIccPinForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("radio::supplyIccPinForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                supplyIccPinForAppResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::supplyIccPinForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::supplyIccPukForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("radio::supplyIccPukForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->supplyIccPukForAppResponse(
                responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::supplyIccPukForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::supplyIccPin2ForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen) {
    RLOGD("radio::supplyIccPin2ForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                supplyIccPin2ForAppResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::supplyIccPin2ForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::supplyIccPuk2ForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen) {
    RLOGD("radio::supplyIccPuk2ForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                supplyIccPuk2ForAppResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::supplyIccPuk2ForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::changeIccPinForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("radio::changeIccPinForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                changeIccPinForAppResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::changeIccPinForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::changeIccPin2ForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen) {
    RLOGD("radio::changeIccPin2ForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                changeIccPin2ForAppResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::changeIccPin2ForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::supplyNetworkDepersonalizationResponse(android::Parcel &p, int slotId, int requestNumber,
                                                 int responseType, int serial, RIL_Errno e,
                                                 void *response, size_t responseLen) {
    RLOGD("radio::supplyNetworkDepersonalizationResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                supplyNetworkDepersonalizationResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::supplyNetworkDepersonalizationResponse: radioService[%d]->mRadioResponse == "
                "NULL", slotId);
    }

    return 0;
}

int radio::getCurrentCallsResponse(android::Parcel &p, int slotId, int requestNumber,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("radio::getCurrentCallsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        hidl_vec<Call> calls;
        if (response == NULL || (responseLen % sizeof(RIL_Call *)) != 0) {
            RLOGE("radio::getCurrentCallsResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int num = responseLen / sizeof(RIL_Call *);
            calls.resize(num);

            for (int i = 0 ; i < num ; i++) {
                RIL_Call *p_cur = ((RIL_Call **) response)[i];
                /* each call info */
                calls[i].state = (CallState) p_cur->state;
                calls[i].index = p_cur->index;
                calls[i].toa = p_cur->toa;
                calls[i].isMpty = p_cur->isMpty;
                calls[i].isMT = p_cur->isMT;
                calls[i].als = p_cur->als;
                calls[i].isVoice = p_cur->isVoice;
                calls[i].isVoicePrivacy = p_cur->isVoicePrivacy;
                calls[i].number = convertCharPtrToHidlString(p_cur->number);
                calls[i].numberPresentation = (CallPresentation) p_cur->numberPresentation;
                calls[i].name = convertCharPtrToHidlString(p_cur->name);
                calls[i].namePresentation = (CallPresentation) p_cur->namePresentation;
                if (p_cur->uusInfo != NULL && p_cur->uusInfo->uusData != NULL) {
                    RIL_UUS_Info *uusInfo = p_cur->uusInfo;
                    calls[i].uusInfo[0].uusType = (UusType) uusInfo->uusType;
                    calls[i].uusInfo[0].uusDcs = (UusDcs) uusInfo->uusDcs;
                    // convert uusInfo->uusData to a null-terminated string
                    char *nullTermStr = strndup(uusInfo->uusData, uusInfo->uusLength);
                    calls[i].uusInfo[0].uusData = nullTermStr;
                    free(nullTermStr);
                }
            }
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                getCurrentCallsResponse(responseInfo, calls);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getCurrentCallsResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::dialResponse(android::Parcel &p, int slotId, int requestNumber,
                       int responseType, int serial, RIL_Errno e, void *response,
                       size_t responseLen) {
    RLOGD("radio::dialResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->dialResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::dialResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getIMSIForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                                int responseType, int serial, RIL_Errno e, void *response,
                                size_t responseLen) {
    RLOGD("radio::getIMSIForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->getIMSIForAppResponse(
                responseInfo, convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getIMSIForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::hangupConnectionResponse(android::Parcel &p, int slotId, int requestNumber,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
    RLOGD("radio::hangupConnectionResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->hangupConnectionResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::hangupConnectionResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::hangupWaitingOrBackgroundResponse(android::Parcel &p, int slotId, int requestNumber,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responseLen) {
    RLOGD("radio::hangupWaitingOrBackgroundResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus =
                radioService[slotId]->mRadioResponse->hangupWaitingOrBackgroundResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::hangupWaitingOrBackgroundResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::hangupForegroundResumeBackgroundResponse(android::Parcel &p, int slotId,
                                                   int requestNumber,
                                                   int responseType, int serial, RIL_Errno e,
                                                   void *response, size_t responseLen) {
    RLOGD("radio::hangupWaitingOrBackgroundResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus =
                radioService[slotId]->mRadioResponse->hangupWaitingOrBackgroundResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::hangupWaitingOrBackgroundResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::switchWaitingOrHoldingAndActiveResponse(android::Parcel &p, int slotId,
                                                  int requestNumber,
                                                  int responseType, int serial, RIL_Errno e,
                                                  void *response, size_t responseLen) {
    RLOGD("radio::switchWaitingOrHoldingAndActiveResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus =
                radioService[slotId]->mRadioResponse->switchWaitingOrHoldingAndActiveResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::switchWaitingOrHoldingAndActiveResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::conferenceResponse(android::Parcel &p, int slotId, int requestNumber, int responseType,
                             int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("radio::conferenceResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->conferenceResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::conferenceResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::rejectCallResponse(android::Parcel &p, int slotId, int requestNumber, int responseType,
                             int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("radio::rejectCallResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->rejectCallResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::rejectCallResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getLastCallFailCauseResponse(android::Parcel &p, int slotId, int requestNumber,
                                       int responseType, int serial, RIL_Errno e, void *response,
                                       size_t responseLen) {
    RLOGD("radio::getLastCallFailCauseResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        LastCallFailCauseInfo info = {};
        info.vendorCause = hidl_string();
        if (response == NULL) {
            RLOGE("radio::getCurrentCallsResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else if (responseLen == sizeof(int)) {
            int *pInt = (int *) response;
            info.causeCode = (LastCallFailCause) pInt[0];
        } else if (responseLen == sizeof(RIL_LastCallFailCauseInfo))  {
            RIL_LastCallFailCauseInfo *pFailCauseInfo = (RIL_LastCallFailCauseInfo *) response;
            info.causeCode = (LastCallFailCause) pFailCauseInfo->cause_code;
            info.vendorCause = convertCharPtrToHidlString(pFailCauseInfo->vendor_cause);
        } else {
            RLOGE("radio::getCurrentCallsResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponse->getLastCallFailCauseResponse(
                responseInfo, info);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getLastCallFailCauseResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getSignalStrengthResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("radio::getSignalStrengthResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        SignalStrength signalStrength = {};
        if (response == NULL || responseLen != sizeof(RIL_SignalStrength_v10)) {
            RLOGE("radio::getSignalStrengthResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            convertRilSignalStrengthToHal(response, responseLen, signalStrength);
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponse->getSignalStrengthResponse(
                responseInfo, signalStrength);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getSignalStrengthResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

RIL_CellInfoType getCellInfoTypeRadioTechnology(char *rat) {
    if (rat == NULL) {
        return RIL_CELL_INFO_TYPE_NONE;
    }

    int radioTech = atoi(rat);

    switch(radioTech) {

        case RADIO_TECH_GPRS:
        case RADIO_TECH_EDGE:
        case RADIO_TECH_GSM: {
            return RIL_CELL_INFO_TYPE_GSM;
        }

        case RADIO_TECH_UMTS:
        case RADIO_TECH_HSDPA:
        case RADIO_TECH_HSUPA:
        case RADIO_TECH_HSPA:
        case RADIO_TECH_HSPAP: {
            return RIL_CELL_INFO_TYPE_WCDMA;
        }

        case RADIO_TECH_IS95A:
        case RADIO_TECH_IS95B:
        case RADIO_TECH_1xRTT:
        case RADIO_TECH_EVDO_0:
        case RADIO_TECH_EVDO_A:
        case RADIO_TECH_EVDO_B:
        case RADIO_TECH_EHRPD: {
            return RIL_CELL_INFO_TYPE_CDMA;
        }

        case RADIO_TECH_LTE:
        case RADIO_TECH_LTE_CA: {
            return RIL_CELL_INFO_TYPE_LTE;
        }

        case RADIO_TECH_TD_SCDMA: {
            return RIL_CELL_INFO_TYPE_TD_SCDMA;
        }

        default: {
            break;
        }
    }

    return RIL_CELL_INFO_TYPE_NONE;

}

void fillCellIdentityResponse(CellIdentity &cellIdentity, RIL_CellIdentity_v16 &rilCellIdentity) {

    cellIdentity.cellIdentityGsm.resize(0);
    cellIdentity.cellIdentityWcdma.resize(0);
    cellIdentity.cellIdentityCdma.resize(0);
    cellIdentity.cellIdentityTdscdma.resize(0);
    cellIdentity.cellIdentityLte.resize(0);
    cellIdentity.cellInfoType = (CellInfoType)rilCellIdentity.cellInfoType;
    switch(rilCellIdentity.cellInfoType) {

        case RIL_CELL_INFO_TYPE_GSM: {
            cellIdentity.cellIdentityGsm.resize(1);
            cellIdentity.cellIdentityGsm[0].mcc =
                    std::to_string(rilCellIdentity.cellIdentityGsm.mcc);
            cellIdentity.cellIdentityGsm[0].mnc =
                    std::to_string(rilCellIdentity.cellIdentityGsm.mnc);
            cellIdentity.cellIdentityGsm[0].lac = rilCellIdentity.cellIdentityGsm.lac;
            cellIdentity.cellIdentityGsm[0].cid = rilCellIdentity.cellIdentityGsm.cid;
            cellIdentity.cellIdentityGsm[0].arfcn = rilCellIdentity.cellIdentityGsm.arfcn;
            cellIdentity.cellIdentityGsm[0].bsic = rilCellIdentity.cellIdentityGsm.bsic;
            break;
        }

        case RIL_CELL_INFO_TYPE_WCDMA: {
            cellIdentity.cellIdentityWcdma.resize(1);
            cellIdentity.cellIdentityWcdma[0].mcc =
                    std::to_string(rilCellIdentity.cellIdentityWcdma.mcc);
            cellIdentity.cellIdentityWcdma[0].mnc =
                    std::to_string(rilCellIdentity.cellIdentityWcdma.mnc);
            cellIdentity.cellIdentityWcdma[0].lac = rilCellIdentity.cellIdentityWcdma.lac;
            cellIdentity.cellIdentityWcdma[0].cid = rilCellIdentity.cellIdentityWcdma.cid;
            cellIdentity.cellIdentityWcdma[0].psc = rilCellIdentity.cellIdentityWcdma.psc;
            cellIdentity.cellIdentityWcdma[0].uarfcn = rilCellIdentity.cellIdentityWcdma.uarfcn;
            break;
        }

        case RIL_CELL_INFO_TYPE_CDMA: {
            cellIdentity.cellIdentityCdma.resize(1);
            cellIdentity.cellIdentityCdma[0].networkId = rilCellIdentity.cellIdentityCdma.networkId;
            cellIdentity.cellIdentityCdma[0].systemId = rilCellIdentity.cellIdentityCdma.systemId;
            cellIdentity.cellIdentityCdma[0].baseStationId =
                    rilCellIdentity.cellIdentityCdma.basestationId;
            cellIdentity.cellIdentityCdma[0].longitude = rilCellIdentity.cellIdentityCdma.longitude;
            cellIdentity.cellIdentityCdma[0].latitude = rilCellIdentity.cellIdentityCdma.latitude;
            break;
        }

        case RIL_CELL_INFO_TYPE_LTE: {
            cellIdentity.cellIdentityLte.resize(1);
            cellIdentity.cellIdentityLte[0].mcc =
                    std::to_string(rilCellIdentity.cellIdentityLte.mcc);
            cellIdentity.cellIdentityLte[0].mnc =
                    std::to_string(rilCellIdentity.cellIdentityLte.mnc);
            cellIdentity.cellIdentityLte[0].ci = rilCellIdentity.cellIdentityLte.ci;
            cellIdentity.cellIdentityLte[0].pci = rilCellIdentity.cellIdentityLte.pci;
            cellIdentity.cellIdentityLte[0].tac = rilCellIdentity.cellIdentityLte.tac;
            cellIdentity.cellIdentityLte[0].earfcn = rilCellIdentity.cellIdentityLte.earfcn;
            break;
        }

        case RIL_CELL_INFO_TYPE_TD_SCDMA: {
            cellIdentity.cellIdentityTdscdma.resize(1);
            cellIdentity.cellIdentityTdscdma[0].mcc =
                    std::to_string(rilCellIdentity.cellIdentityTdscdma.mcc);
            cellIdentity.cellIdentityTdscdma[0].mnc =
                    std::to_string(rilCellIdentity.cellIdentityTdscdma.mnc);
            cellIdentity.cellIdentityTdscdma[0].lac = rilCellIdentity.cellIdentityTdscdma.lac;
            cellIdentity.cellIdentityTdscdma[0].cid = rilCellIdentity.cellIdentityTdscdma.cid;
            cellIdentity.cellIdentityTdscdma[0].cpid = rilCellIdentity.cellIdentityTdscdma.cpid;
            break;
        }

        default: {
            break;
        }
    }
}

int convertResponseStringEntryToInt(char **response, int index, int numStrings) {
    if ((response != NULL) &&  (numStrings > index) && (response[index] != NULL)) {
        return atoi(response[index]);
    }

    return -1;
}

void fillCellIdentityFromVoiceRegStateResponseString(CellIdentity &cellIdentity,
        int numStrings, char** response) {

    RIL_CellIdentity_v16 rilCellIdentity;
    int32_t *tmp = (int32_t*)&rilCellIdentity;

    for (int i = 0; i < sizeof(RIL_CellIdentity_v16)/sizeof(int32_t); i++) {
        tmp[i] = -1;
    }

    rilCellIdentity.cellInfoType = getCellInfoTypeRadioTechnology(response[3]);
    switch(rilCellIdentity.cellInfoType) {

        case RIL_CELL_INFO_TYPE_GSM: {
            rilCellIdentity.cellIdentityGsm.lac =
                    convertResponseStringEntryToInt(response, 1, numStrings);
            rilCellIdentity.cellIdentityGsm.cid =
                    convertResponseStringEntryToInt(response, 2, numStrings);
            break;
        }

        case RIL_CELL_INFO_TYPE_WCDMA: {
            rilCellIdentity.cellIdentityWcdma.lac =
                    convertResponseStringEntryToInt(response, 1, numStrings);
            rilCellIdentity.cellIdentityWcdma.cid =
                    convertResponseStringEntryToInt(response, 2, numStrings);
            rilCellIdentity.cellIdentityWcdma.psc =
                    convertResponseStringEntryToInt(response, 14, numStrings);
            break;
        }

        case RIL_CELL_INFO_TYPE_TD_SCDMA:{
            rilCellIdentity.cellIdentityTdscdma.lac =
                    convertResponseStringEntryToInt(response, 1, numStrings);
            rilCellIdentity.cellIdentityTdscdma.cid =
                    convertResponseStringEntryToInt(response, 2, numStrings);
            break;
        }

        case RIL_CELL_INFO_TYPE_CDMA:{
            rilCellIdentity.cellIdentityCdma.basestationId =
                    convertResponseStringEntryToInt(response, 4, numStrings);
            rilCellIdentity.cellIdentityCdma.longitude =
                    convertResponseStringEntryToInt(response, 5, numStrings);
            rilCellIdentity.cellIdentityCdma.latitude =
                    convertResponseStringEntryToInt(response, 6, numStrings);
            rilCellIdentity.cellIdentityCdma.systemId =
                    convertResponseStringEntryToInt(response, 8, numStrings);
            rilCellIdentity.cellIdentityCdma.networkId =
                    convertResponseStringEntryToInt(response, 9, numStrings);
            break;
        }

        default: {
            break;
        }
    }

    fillCellIdentityResponse(cellIdentity, rilCellIdentity);
}

void fillCellIdentityFromDataRegStateResponseString(CellIdentity &cellIdentity,
        int numStrings, char** response) {

    RIL_CellIdentity_v16 rilCellIdentity;
    int32_t *tmp = (int32_t*)&rilCellIdentity;

    for (int i = 0; i < sizeof(RIL_CellIdentity_v16)/sizeof(int32_t); i++) {
        tmp[i] = -1;
    }

    rilCellIdentity.cellInfoType = getCellInfoTypeRadioTechnology(response[3]);
    switch(rilCellIdentity.cellInfoType) {
        case RIL_CELL_INFO_TYPE_GSM: {
            rilCellIdentity.cellIdentityGsm.lac =
                    convertResponseStringEntryToInt(response, 1, numStrings);
            rilCellIdentity.cellIdentityGsm.cid =
                    convertResponseStringEntryToInt(response, 2, numStrings);
            break;
        }
        case RIL_CELL_INFO_TYPE_WCDMA: {
            rilCellIdentity.cellIdentityWcdma.lac =
                    convertResponseStringEntryToInt(response, 1, numStrings);
            rilCellIdentity.cellIdentityWcdma.cid =
                    convertResponseStringEntryToInt(response, 2, numStrings);
            break;
        }
        case RIL_CELL_INFO_TYPE_TD_SCDMA:{
            rilCellIdentity.cellIdentityTdscdma.lac =
                    convertResponseStringEntryToInt(response, 1, numStrings);
            rilCellIdentity.cellIdentityTdscdma.cid =
                    convertResponseStringEntryToInt(response, 2, numStrings);
            break;
        }
        case RIL_CELL_INFO_TYPE_LTE: {
            rilCellIdentity.cellIdentityLte.tac =
                    convertResponseStringEntryToInt(response, 6, numStrings);
            rilCellIdentity.cellIdentityLte.pci =
                    convertResponseStringEntryToInt(response, 7, numStrings);
            rilCellIdentity.cellIdentityLte.ci =
                    convertResponseStringEntryToInt(response, 8, numStrings);
            break;
        }
        default: {
            break;
        }
    }

    fillCellIdentityResponse(cellIdentity, rilCellIdentity);
}

int radio::getVoiceRegistrationStateResponse(android::Parcel &p, int slotId, int requestNumber,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responseLen) {
    RLOGD("radio::getVoiceRegistrationStateResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        VoiceRegStateResult voiceRegResponse = {};
        int numStrings = responseLen / sizeof(char *);
        if (response == NULL) {
               RLOGE("radio::getVoiceRegistrationStateResponse Invalid response: NULL");
               if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else if (s_vendorFunctions->version <= 14) {
            if (numStrings != 15) {
                RLOGE("radio::getVoiceRegistrationStateResponse Invalid response: NULL");
                if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            } else {
                char **resp = (char **) response;
                voiceRegResponse.regState = (RegState) ATOI_NULL_HANDLED_DEF(resp[0], 4);
                voiceRegResponse.rat = ATOI_NULL_HANDLED(resp[3]);
                voiceRegResponse.cssSupported = ATOI_NULL_HANDLED_DEF(resp[7], 0);
                voiceRegResponse.roamingIndicator = ATOI_NULL_HANDLED(resp[10]);
                voiceRegResponse.systemIsInPrl = ATOI_NULL_HANDLED_DEF(resp[11], 0);
                voiceRegResponse.defaultRoamingIndicator = ATOI_NULL_HANDLED_DEF(resp[12], 0);
                voiceRegResponse.reasonForDenial = ATOI_NULL_HANDLED_DEF(resp[13], 0);
                fillCellIdentityFromVoiceRegStateResponseString(voiceRegResponse.cellIdentity,
                        numStrings, resp);
            }
        } else {
            RIL_VoiceRegistrationStateResponse *voiceRegState =
                    (RIL_VoiceRegistrationStateResponse *)response;

            if (responseLen != sizeof(RIL_VoiceRegistrationStateResponse)) {
                RLOGE("radio::getVoiceRegistrationStateResponse Invalid response: NULL");
                if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            } else {
                voiceRegResponse.regState = (RegState) voiceRegState->regState;
                voiceRegResponse.rat = voiceRegState->rat;;
                voiceRegResponse.cssSupported = voiceRegState->cssSupported;
                voiceRegResponse.roamingIndicator = voiceRegState->roamingIndicator;
                voiceRegResponse.systemIsInPrl = voiceRegState->systemIsInPrl;
                voiceRegResponse.defaultRoamingIndicator = voiceRegState->defaultRoamingIndicator;
                voiceRegResponse.reasonForDenial = voiceRegState->reasonForDenial;
                fillCellIdentityResponse(voiceRegResponse.cellIdentity,
                        voiceRegState->cellIdentity);
            }
        }

        Return<void> retStatus =
                radioService[slotId]->mRadioResponse->getVoiceRegistrationStateResponse(
                responseInfo, voiceRegResponse);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getVoiceRegistrationStateResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getDataRegistrationStateResponse(android::Parcel &p, int slotId, int requestNumber,
                                           int responseType, int serial, RIL_Errno e,
                                           void *response, size_t responseLen) {
    RLOGD("radio::getDataRegistrationStateResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        DataRegStateResult dataRegResponse = {};
        if (response == NULL) {
            RLOGE("radio::getDataRegistrationStateResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else if (s_vendorFunctions->version <= 14) {
            int numStrings = responseLen / sizeof(char *);
            if ((numStrings != 6) && (numStrings != 11)) {
                RLOGE("radio::getDataRegistrationStateResponse Invalid response: NULL");
                if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            } else {
                char **resp = (char **) response;
                dataRegResponse.regState = (RegState) ATOI_NULL_HANDLED_DEF(resp[0], 4);
                dataRegResponse.rat =  ATOI_NULL_HANDLED_DEF(resp[3], 0);
                dataRegResponse.reasonDataDenied =  ATOI_NULL_HANDLED(resp[4]);
                dataRegResponse.maxDataCalls =  ATOI_NULL_HANDLED_DEF(resp[5], 1);
                fillCellIdentityFromDataRegStateResponseString(dataRegResponse.cellIdentity,
                        numStrings, resp);
            }
        } else {
            RIL_DataRegistrationStateResponse *dataRegState =
                    (RIL_DataRegistrationStateResponse *)response;

            if (responseLen != sizeof(RIL_DataRegistrationStateResponse)) {
                RLOGE("radio::getDataRegistrationStateResponse Invalid response: NULL");
                if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            } else {
                dataRegResponse.regState = (RegState) dataRegState->regState;
                dataRegResponse.rat = dataRegState->rat;;
                dataRegResponse.reasonDataDenied = dataRegState->reasonDataDenied;
                dataRegResponse.maxDataCalls = dataRegState->maxDataCalls;
                fillCellIdentityResponse(dataRegResponse.cellIdentity, dataRegState->cellIdentity);
            }
        }

        Return<void> retStatus =
                radioService[slotId]->mRadioResponse->getDataRegistrationStateResponse(responseInfo,
                dataRegResponse);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getDataRegistrationStateResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getOperatorResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responseLen) {
    RLOGD("radio::getOperatorResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_string longName;
        hidl_string shortName;
        hidl_string numeric;
        int numStrings = responseLen / sizeof(char *);
        if (response == NULL || numStrings != 3) {
            RLOGE("radio::getOperatorResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;

        } else {
            char **resp = (char **) response;
            longName = convertCharPtrToHidlString(resp[0]);
            shortName = convertCharPtrToHidlString(resp[1]);
            numeric = convertCharPtrToHidlString(resp[2]);
        }
        Return<void> retStatus = radioService[slotId]->mRadioResponse->getOperatorResponse(
                responseInfo, longName, shortName, numeric);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getOperatorResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setRadioPowerResponse(android::Parcel &p, int slotId, int requestNumber,
                                int responseType, int serial, RIL_Errno e, void *response,
                                size_t responseLen) {
    RLOGD("radio::setRadioPowerResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->setRadioPowerResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setRadioPowerResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::sendDtmfResponse(android::Parcel &p, int slotId, int requestNumber,
                           int responseType, int serial, RIL_Errno e, void *response,
                           size_t responseLen) {
    RLOGD("radio::sendDtmfResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->sendDtmfResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::sendDtmfResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

SendSmsResult makeSendSmsResult(RadioResponseInfo& responseInfo, int serial, int responseType,
                                RIL_Errno e, void *response, size_t responseLen) {
    populateResponseInfo(responseInfo, serial, responseType, e);
    SendSmsResult result = {};

    if (response == NULL || responseLen != sizeof(RIL_SMS_Response)) {
        RLOGE("Invalid response: NULL");
        if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        result.ackPDU = hidl_string();
    } else {
        RIL_SMS_Response *resp = (RIL_SMS_Response *) response;
        result.messageRef = resp->messageRef;
        result.ackPDU = convertCharPtrToHidlString(resp->ackPDU);
        result.errorCode = resp->errorCode;
    }
    return result;
}

int radio::sendSmsResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responseLen) {
    RLOGD("radio::sendSmsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        SendSmsResult result = makeSendSmsResult(responseInfo, serial, responseType, e, response,
                responseLen);

        Return<void> retStatus = radioService[slotId]->mRadioResponse->sendSmsResponse(responseInfo,
                result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::sendSmsResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::sendSMSExpectMoreResponse(android::Parcel &p, int slotId, int requestNumber,
                                    int responseType, int serial, RIL_Errno e, void *response,
                                    size_t responseLen) {
    RLOGD("radio::sendSMSExpectMoreResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        SendSmsResult result = makeSendSmsResult(responseInfo, serial, responseType, e, response,
                responseLen);

        Return<void> retStatus = radioService[slotId]->mRadioResponse->sendSMSExpectMoreResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::sendSMSExpectMoreResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setupDataCallResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responseLen) {
    RLOGD("radio::setupDataCallResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        SetupDataCallResult result = {};
        if (response == NULL || responseLen != sizeof(RIL_Data_Call_Response_v11)) {
            RLOGE("radio::setupDataCallResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            result.status = DataCallFailCause::ERROR_UNSPECIFIED;
            result.type = hidl_string();
            result.ifname = hidl_string();
            result.addresses = hidl_string();
            result.dnses = hidl_string();
            result.gateways = hidl_string();
            result.pcscf = hidl_string();
        } else {
            convertRilDataCallToHal((RIL_Data_Call_Response_v11 *) response, result);
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponse->setupDataCallResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setupDataCallResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

IccIoResult responseIccIo(RadioResponseInfo& responseInfo, int serial, int responseType,
                           RIL_Errno e, void *response, size_t responseLen) {
    populateResponseInfo(responseInfo, serial, responseType, e);
    IccIoResult result = {};

    if (response == NULL || responseLen != sizeof(RIL_SIM_IO_Response)) {
        RLOGE("Invalid response: NULL");
        if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        result.simResponse = hidl_string();
    } else {
        RIL_SIM_IO_Response *resp = (RIL_SIM_IO_Response *) response;
        result.sw1 = resp->sw1;
        result.sw2 = resp->sw2;
        result.simResponse = convertCharPtrToHidlString(resp->simResponse);
    }
    return result;
}

int radio::iccIOForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                      int responseType, int serial, RIL_Errno e, void *response,
                      size_t responseLen) {
    RLOGD("radio::radio::iccIOForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        IccIoResult result = responseIccIo(responseInfo, serial, responseType, e, response,
                responseLen);

        Return<void> retStatus = radioService[slotId]->mRadioResponse->iccIOForAppResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::iccIOForAppResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::sendUssdResponse(android::Parcel &p, int slotId, int requestNumber,
                           int responseType, int serial, RIL_Errno e, void *response,
                           size_t responseLen) {
    RLOGD("radio::sendUssdResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->sendUssdResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::sendUssdResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::cancelPendingUssdResponse(android::Parcel &p, int slotId, int requestNumber,
                                    int responseType, int serial, RIL_Errno e, void *response,
                                    size_t responseLen) {
    RLOGD("radio::cancelPendingUssdResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->cancelPendingUssdResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::cancelPendingUssdResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getClirResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responseLen) {
    RLOGD("radio::getClirResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        int n = -1, m = -1;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || numInts != 2) {
            RLOGE("radio::getClirResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            n = pInt[0];
            m = pInt[1];
        }
        Return<void> retStatus = radioService[slotId]->mRadioResponse->getClirResponse(responseInfo,
                n, m);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getClirResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setClirResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responseLen) {
    RLOGD("radio::setClirResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->setClirResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setClirResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getCallForwardStatusResponse(android::Parcel &p, int slotId, int requestNumber,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responseLen) {
    RLOGD("radio::getCallForwardStatusResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<CallForwardInfo> callForwardInfos;

        if (response == NULL || responseLen % sizeof(RIL_CallForwardInfo *) != 0) {
            RLOGE("radio::getCallForwardStatusResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int num = responseLen / sizeof(RIL_CallForwardInfo *);
            callForwardInfos.resize(num);
            for (int i = 0 ; i < num; i++) {
                RIL_CallForwardInfo *resp = ((RIL_CallForwardInfo **) response)[i];
                callForwardInfos[i].status = (CallForwardInfoStatus) resp->status;
                callForwardInfos[i].reason = resp->reason;
                callForwardInfos[i].serviceClass = resp->serviceClass;
                callForwardInfos[i].toa = resp->toa;
                callForwardInfos[i].number = convertCharPtrToHidlString(resp->number);
                callForwardInfos[i].timeSeconds = resp->timeSeconds;
            }
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponse->getCallForwardStatusResponse(
                responseInfo, callForwardInfos);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getCallForwardStatusResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setCallForwardResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responseLen) {
    RLOGD("radio::setCallForwardResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->setCallForwardResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setCallForwardResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getCallWaitingResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responseLen) {
    RLOGD("radio::getCallWaitingResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        bool enable = false;
        int serviceClass = -1;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || numInts != 2) {
            RLOGE("radio::getCallWaitingResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            enable = pInt[0] == 1 ? true : false;
            serviceClass = pInt[1];
        }
        Return<void> retStatus = radioService[slotId]->mRadioResponse->getClirResponse(responseInfo,
                enable, serviceClass);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getCallWaitingResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setCallWaitingResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responseLen) {
    RLOGD("radio::setCallWaitingResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->setCallWaitingResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setCallWaitingResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::acknowledgeLastIncomingGsmSmsResponse(android::Parcel &p, int slotId, int requestNumber,
                                                int responseType, int serial, RIL_Errno e,
                                                void *response, size_t responseLen) {
    RLOGD("radio::acknowledgeLastIncomingGsmSmsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus =
                radioService[slotId]->mRadioResponse->acknowledgeLastIncomingGsmSmsResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::acknowledgeLastIncomingGsmSmsResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::acceptCallResponse(android::Parcel &p, int slotId, int requestNumber,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responseLen) {
    RLOGD("radio::acceptCallResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->acceptCallResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::acceptCallResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::deactivateDataCallResponse(android::Parcel &p, int slotId, int requestNumber,
                                                int responseType, int serial, RIL_Errno e,
                                                void *response, size_t responseLen) {
    RLOGD("radio::deactivateDataCallResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->deactivateDataCallResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::deactivateDataCallResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getFacilityLockForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                                        int responseType, int serial, RIL_Errno e,
                                        void *response, size_t responseLen) {
    RLOGD("radio::getFacilityLockForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                getFacilityLockForAppResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getFacilityLockForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setFacilityLockForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen) {
    RLOGD("radio::setFacilityLockForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setFacilityLockForAppResponse(responseInfo,
                ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setFacilityLockForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setBarringPasswordResponse(android::Parcel &p, int slotId, int requestNumber,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responseLen) {
    RLOGD("radio::acceptCallResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setBarringPasswordResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setBarringPasswordResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getNetworkSelectionModeResponse(android::Parcel &p, int slotId, int requestNumber,
                                          int responseType, int serial, RIL_Errno e, void *response,
                                          size_t responseLen) {
    RLOGD("radio::getNetworkSelectionModeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        bool manual = false;
        int serviceClass;
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("radio::getNetworkSelectionModeResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            manual = pInt[0] == 1 ? true : false;
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getNetworkSelectionModeResponse(
                responseInfo,
                manual);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getNetworkSelectionModeResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setNetworkSelectionModeAutomaticResponse(android::Parcel &p, int slotId,
                             int requestNumber,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responseLen) {
    RLOGD("radio::setNetworkSelectionModeAutomaticResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setNetworkSelectionModeAutomaticResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setNetworkSelectionModeAutomaticResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::setNetworkSelectionModeManualResponse(android::Parcel &p, int slotId, int requestNumber,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responseLen) {
    RLOGD("radio::setNetworkSelectionModeManualResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setNetworkSelectionModeManualResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::acceptCallResponse: radioService[%d]->setNetworkSelectionModeManualResponse "
                "== NULL", slotId);
    }

    return 0;
}

int convertOperatorStatusToInt(const char *str) {
    if (strncmp("unknown", str, 9) == 0) {
        return (int) OperatorStatus::UNKNOWN;
    } else if (strncmp("available", str, 9) == 0) {
        return (int) OperatorStatus::AVAILABLE;
    } else if (strncmp("current", str, 9) == 0) {
        return (int) OperatorStatus::CURRENT;
    } else if (strncmp("forbidden", str, 9) == 0) {
        return (int) OperatorStatus::FORBIDDEN;
    } else {
        return -1;
    }
}

int radio::getAvailableNetworksResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responseLen) {
    RLOGD("radio::getAvailableNetworksResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<OperatorInfo> networks;
        if (response == NULL || responseLen % (4 * sizeof(char *))!= 0) {
            RLOGE("radio::getAvailableNetworksResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            char **resp = (char **) response;
            int numStrings = responseLen / sizeof(char *);
            networks.resize(numStrings/4);
            for (int i = 0; i < numStrings; i = i + 4) {
                networks[i].alphaLong = convertCharPtrToHidlString(resp[i]);
                networks[i].alphaShort = convertCharPtrToHidlString(resp[i + 1]);
                networks[i].operatorNumeric = convertCharPtrToHidlString(resp[i + 2]);
                int status = convertOperatorStatusToInt(resp[i + 3]);
                if (status == -1) {
                    if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
                } else {
                    networks[i].status = (OperatorStatus) status;
                }
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getAvailableNetworksResponse(responseInfo,
                networks);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getAvailableNetworksResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::startDtmfResponse(android::Parcel &p, int slotId, int requestNumber,
                            int responseType, int serial, RIL_Errno e,
                            void *response, size_t responseLen) {
    RLOGD("radio::startDtmfResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->startDtmfResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::startDtmfResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::stopDtmfResponse(android::Parcel &p, int slotId, int requestNumber,
                           int responseType, int serial, RIL_Errno e,
                           void *response, size_t responseLen) {
    RLOGD("radio::stopDtmfResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->stopDtmfResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::stopDtmfResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getBasebandVersionResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("radio::getBasebandVersionResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getBasebandVersionResponse(responseInfo,
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getBasebandVersionResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::separateConnectionResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("radio::separateConnectionResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->separateConnectionResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::separateConnectionResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setMuteResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responseLen) {
    RLOGD("radio::setMuteResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setMuteResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setMuteResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getMuteResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responseLen) {
    RLOGD("radio::getMuteResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        bool enable = false;
        int serviceClass;
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("radio::getMuteResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            enable = pInt[0] == 1 ? true : false;
        }
        Return<void> retStatus = radioService[slotId]->mRadioResponse->getMuteResponse(responseInfo,
                enable);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getMuteResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getClipResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responseLen) {
    RLOGD("radio::getClipResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->getClipResponse(responseInfo,
                (ClipStatus) ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getClipResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getDataCallListResponse(android::Parcel &p, int slotId, int requestNumber,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
    RLOGD("radio::getDataCallListResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        hidl_vec<SetupDataCallResult> ret;
        if (response == NULL || responseLen % sizeof(RIL_Data_Call_Response_v11) != 0) {
            RLOGE("radio::getDataCallListResponse: invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            convertRilDataCallListToHal(response, responseLen, ret);
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponse->getDataCallListResponse(
                responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getDataCallListResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setSuppServiceNotificationsResponse(android::Parcel &p, int slotId, int requestNumber,
                                              int responseType, int serial, RIL_Errno e,
                                              void *response, size_t responseLen) {
    RLOGD("radio::setSuppServiceNotificationsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setSuppServiceNotificationsResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setSuppServiceNotificationsResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::deleteSmsOnSimResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("radio::deleteSmsOnSimResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->deleteSmsOnSimResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::deleteSmsOnSimResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setBandModeResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responseLen) {
    RLOGD("radio::setBandModeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setBandModeResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setBandModeResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::writeSmsToSimResponse(android::Parcel &p, int slotId, int requestNumber,
                                int responseType, int serial, RIL_Errno e,
                                void *response, size_t responseLen) {
    RLOGD("radio::writeSmsToSimResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->writeSmsToSimResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::writeSmsToSimResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getAvailableBandModesResponse(android::Parcel &p, int slotId, int requestNumber,
                                        int responseType, int serial, RIL_Errno e, void *response,
                                        size_t responseLen) {
    RLOGD("radio::getAvailableBandModesResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<RadioBandMode> modes;
        if (response == NULL || responseLen % sizeof(int) != 0) {
            RLOGE("radio::getAvailableBandModesResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            int numInts = responseLen / sizeof(int);
            modes.resize(numInts);
            for (int i = 0; i < numInts; i++) {
                modes[i] = (RadioBandMode) pInt[i];
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getAvailableBandModesResponse(responseInfo,
                modes);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getAvailableBandModesResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::sendEnvelopeResponse(android::Parcel &p, int slotId, int requestNumber,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen) {
    RLOGD("radio::sendEnvelopeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->sendEnvelopeResponse(responseInfo,
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::sendEnvelopeResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::sendTerminalResponseToSimResponse(android::Parcel &p, int slotId, int requestNumber,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responseLen) {
    RLOGD("radio::sendTerminalResponseToSimResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->sendTerminalResponseToSimResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::sendTerminalResponseToSimResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::handleStkCallSetupRequestFromSimResponse(android::Parcel &p, int slotId,
                                                   int requestNumber, int responseType, int serial,
                                                   RIL_Errno e, void *response,
                                                   size_t responseLen) {
    RLOGD("radio::handleStkCallSetupRequestFromSimResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->handleStkCallSetupRequestFromSimResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::handleStkCallSetupRequestFromSimResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::explicitCallTransferResponse(android::Parcel &p, int slotId, int requestNumber,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responseLen) {
    RLOGD("radio::explicitCallTransferResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->explicitCallTransferResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::explicitCallTransferResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setPreferredNetworkTypeResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("radio::setPreferredNetworkTypeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setPreferredNetworkTypeResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setPreferredNetworkTypeResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}


int radio::getPreferredNetworkTypeResponse(android::Parcel &p, int slotId, int requestNumber,
                                          int responseType, int serial, RIL_Errno e,
                                          void *response, size_t responseLen) {
    RLOGD("radio::getPreferredNetworkTypeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getPreferredNetworkTypeResponse(
                responseInfo, (PreferredNetworkType) ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getPreferredNetworkTypeResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getNeighboringCidsResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("radio::getNeighboringCidsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<NeighboringCell> cells;

        if (response == NULL || responseLen % sizeof(RIL_NeighboringCell *) != 0) {
            RLOGE("radio::getNeighboringCidsResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int num = responseLen / sizeof(RIL_NeighboringCell *);
            cells.resize(num);
            for (int i = 0 ; i < num; i++) {
                RIL_NeighboringCell *resp = ((RIL_NeighboringCell **) response)[i];
                cells[i].cid = convertCharPtrToHidlString(resp->cid);
                cells[i].rssi = resp->rssi;
            }
        }

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getNeighboringCidsResponse(responseInfo,
                cells);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getNeighboringCidsResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setLocationUpdatesResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("radio::setLocationUpdatesResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setLocationUpdatesResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setLocationUpdatesResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setCdmaSubscriptionSourceResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("radio::setCdmaSubscriptionSourceResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setCdmaSubscriptionSourceResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setCdmaSubscriptionSourceResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setCdmaRoamingPreferenceResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("radio::setCdmaRoamingPreferenceResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setCdmaRoamingPreferenceResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setCdmaRoamingPreferenceResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getCdmaRoamingPreferenceResponse(android::Parcel &p, int slotId, int requestNumber,
                                           int responseType, int serial, RIL_Errno e,
                                           void *response, size_t responseLen) {
    RLOGD("radio::getCdmaRoamingPreferenceResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getCdmaRoamingPreferenceResponse(
                responseInfo, (CdmaRoamingType) ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getCdmaRoamingPreferenceResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setTTYModeResponse(android::Parcel &p, int slotId, int requestNumber,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responseLen) {
    RLOGD("radio::setTTYModeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setTTYModeResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setTTYModeResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getTTYModeResponse(android::Parcel &p, int slotId, int requestNumber,
                             int responseType, int serial, RIL_Errno e,
                             void *response, size_t responseLen) {
    RLOGD("radio::getTTYModeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getTTYModeResponse(responseInfo,
                (TtyMode) ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getTTYModeResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setPreferredVoicePrivacyResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("radio::setPreferredVoicePrivacyResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setPreferredVoicePrivacyResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setPreferredVoicePrivacyResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getPreferredVoicePrivacyResponse(android::Parcel &p, int slotId, int requestNumber,
                                           int responseType, int serial, RIL_Errno e,
                                           void *response, size_t responseLen) {
    RLOGD("radio::getPreferredVoicePrivacyResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        bool enable = false;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || numInts != 1) {
            RLOGE("radio::getPreferredVoicePrivacyResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            enable = pInt[0] == 1 ? true : false;
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getPreferredVoicePrivacyResponse(
                responseInfo, enable);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getPreferredVoicePrivacyResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::sendCDMAFeatureCodeResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("radio::sendCDMAFeatureCodeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->sendCDMAFeatureCodeResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::sendCDMAFeatureCodeResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::sendBurstDtmfResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("radio::sendBurstDtmfResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->sendBurstDtmfResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::sendBurstDtmfResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::sendCdmaSmsResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responseLen) {
    RLOGD("radio::sendCdmaSmsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        SendSmsResult result = makeSendSmsResult(responseInfo, serial, responseType, e, response,
                responseLen);

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->sendCdmaSmsResponse(responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::sendCdmaSmsResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::acknowledgeLastIncomingCdmaSmsResponse(android::Parcel &p, int slotId, int requestNumber,
                                                 int responseType, int serial, RIL_Errno e,
                                                 void *response, size_t responseLen) {
    RLOGD("radio::acknowledgeLastIncomingCdmaSmsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->acknowledgeLastIncomingCdmaSmsResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::acknowledgeLastIncomingCdmaSmsResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::getGsmBroadcastConfigResponse(android::Parcel &p, int slotId, int requestNumber,
                                        int responseType, int serial, RIL_Errno e,
                                        void *response, size_t responseLen) {
    RLOGD("radio::getGsmBroadcastConfigResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<GsmBroadcastSmsConfigInfo> configs;

        if (response == NULL || responseLen % sizeof(RIL_GSM_BroadcastSmsConfigInfo *) != 0) {
            RLOGE("radio::getGsmBroadcastConfigResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int num = responseLen / sizeof(RIL_GSM_BroadcastSmsConfigInfo *);
            configs.resize(num);
            for (int i = 0 ; i < num; i++) {
                RIL_GSM_BroadcastSmsConfigInfo *resp =
                        ((RIL_GSM_BroadcastSmsConfigInfo **) response)[i];
                configs[i].fromServiceId = resp->fromServiceId;
                configs[i].toServiceId = resp->toServiceId;
                configs[i].fromCodeScheme = resp->fromCodeScheme;
                configs[i].toCodeScheme = resp->toCodeScheme;
                configs[i].selected = resp->selected == 1 ? true : false;
            }
        }

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getGsmBroadcastConfigResponse(responseInfo,
                configs);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getGsmBroadcastConfigResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setGsmBroadcastConfigResponse(android::Parcel &p, int slotId, int requestNumber,
                                        int responseType, int serial, RIL_Errno e,
                                        void *response, size_t responseLen) {
    RLOGD("radio::setGsmBroadcastConfigResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setGsmBroadcastConfigResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setGsmBroadcastConfigResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setGsmBroadcastActivationResponse(android::Parcel &p, int slotId, int requestNumber,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responseLen) {
    RLOGD("radio::setGsmBroadcastActivationResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setGsmBroadcastActivationResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setGsmBroadcastActivationResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getCdmaBroadcastConfigResponse(android::Parcel &p, int slotId, int requestNumber,
                                         int responseType, int serial, RIL_Errno e,
                                         void *response, size_t responseLen) {
    RLOGD("radio::getCdmaBroadcastConfigResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<CdmaBroadcastSmsConfigInfo> configs;

        if (response == NULL || responseLen % sizeof(RIL_CDMA_BroadcastSmsConfigInfo *) != 0) {
            RLOGE("radio::getCdmaBroadcastConfigResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int num = responseLen / sizeof(RIL_CDMA_BroadcastSmsConfigInfo *);
            configs.resize(num);
            for (int i = 0 ; i < num; i++) {
                RIL_CDMA_BroadcastSmsConfigInfo *resp =
                        ((RIL_CDMA_BroadcastSmsConfigInfo **) response)[i];
                configs[i].serviceCategory = resp->service_category;
                configs[i].language = resp->language;
                configs[i].selected = resp->selected == 1 ? true : false;
            }
        }

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getCdmaBroadcastConfigResponse(responseInfo,
                configs);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getCdmaBroadcastConfigResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setCdmaBroadcastConfigResponse(android::Parcel &p, int slotId, int requestNumber,
                                         int responseType, int serial, RIL_Errno e,
                                         void *response, size_t responseLen) {
    RLOGD("radio::setCdmaBroadcastConfigResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setCdmaBroadcastConfigResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setCdmaBroadcastConfigResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setCdmaBroadcastActivationResponse(android::Parcel &p, int slotId, int requestNumber,
                                             int responseType, int serial, RIL_Errno e,
                                             void *response, size_t responseLen) {
    RLOGD("radio::setCdmaBroadcastActivationResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setCdmaBroadcastActivationResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setCdmaBroadcastActivationResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getCDMASubscriptionResponse(android::Parcel &p, int slotId, int requestNumber,
                                      int responseType, int serial, RIL_Errno e, void *response,
                                      size_t responseLen) {
    RLOGD("radio::getCDMASubscriptionResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        int numStrings = responseLen / sizeof(char *);
        hidl_string emptyString;
        if (response == NULL || numStrings != 5) {
            RLOGE("radio::getOperatorResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponse->getCDMASubscriptionResponse(
                    responseInfo, emptyString, emptyString, emptyString, emptyString, emptyString);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            char **resp = (char **) response;
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponse->getCDMASubscriptionResponse(
                    responseInfo,
                    convertCharPtrToHidlString(resp[0]),
                    convertCharPtrToHidlString(resp[1]),
                    convertCharPtrToHidlString(resp[2]),
                    convertCharPtrToHidlString(resp[3]),
                    convertCharPtrToHidlString(resp[4]));
            radioService[slotId]->checkReturnStatus(retStatus);
        }
    } else {
        RLOGE("radio::getCDMASubscriptionResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::writeSmsToRuimResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("radio::writeSmsToRuimResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->writeSmsToRuimResponse(responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::writeSmsToRuimResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::deleteSmsOnRuimResponse(android::Parcel &p, int slotId, int requestNumber,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("radio::deleteSmsOnRuimResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->deleteSmsOnRuimResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::deleteSmsOnRuimResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getDeviceIdentityResponse(android::Parcel &p, int slotId, int requestNumber,
                                    int responseType, int serial, RIL_Errno e, void *response,
                                    size_t responseLen) {
    RLOGD("radio::getDeviceIdentityResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        int numStrings = responseLen / sizeof(char *);
        hidl_string emptyString;
        if (response == NULL || numStrings != 4) {
            RLOGE("radio::getDeviceIdentityResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponse->getDeviceIdentityResponse(responseInfo,
                    emptyString, emptyString, emptyString, emptyString);
            radioService[slotId]->checkReturnStatus(retStatus);
        } else {
            char **resp = (char **) response;
            Return<void> retStatus
                    = radioService[slotId]->mRadioResponse->getDeviceIdentityResponse(responseInfo,
                    convertCharPtrToHidlString(resp[0]),
                    convertCharPtrToHidlString(resp[1]),
                    convertCharPtrToHidlString(resp[2]),
                    convertCharPtrToHidlString(resp[3]));
            radioService[slotId]->checkReturnStatus(retStatus);
        }
    } else {
        RLOGE("radio::getDeviceIdentityResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::exitEmergencyCallbackModeResponse(android::Parcel &p, int slotId, int requestNumber,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responseLen) {
    RLOGD("radio::exitEmergencyCallbackModeResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->exitEmergencyCallbackModeResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::exitEmergencyCallbackModeResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getSmscAddressResponse(android::Parcel &p, int slotId, int requestNumber,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("radio::getSmscAddressResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getSmscAddressResponse(responseInfo,
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getSmscAddressResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setSmscAddressResponse(android::Parcel &p, int slotId, int requestNumber,
                                             int responseType, int serial, RIL_Errno e,
                                             void *response, size_t responseLen) {
    RLOGD("radio::setSmscAddressResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setSmscAddressResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setSmscAddressResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::reportSmsMemoryStatusResponse(android::Parcel &p, int slotId, int requestNumber,
                                        int responseType, int serial, RIL_Errno e,
                                        void *response, size_t responseLen) {
    RLOGD("radio::reportSmsMemoryStatusResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->reportSmsMemoryStatusResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::reportSmsMemoryStatusResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::reportStkServiceIsRunningResponse(android::Parcel &p, int slotId, int requestNumber,
                                             int responseType, int serial, RIL_Errno e,
                                             void *response, size_t responseLen) {
    RLOGD("radio::reportStkServiceIsRunningResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->
                reportStkServiceIsRunningResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::reportStkServiceIsRunningResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getCdmaSubscriptionSourceResponse(android::Parcel &p, int slotId, int requestNumber,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responseLen) {
    RLOGD("radio::getCdmaSubscriptionSourceResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getCdmaSubscriptionSourceResponse(
                responseInfo, (CdmaSubscriptionSource) ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getCdmaSubscriptionSourceResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::requestIsimAuthenticationResponse(android::Parcel &p, int slotId, int requestNumber,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responseLen) {
    RLOGD("radio::requestIsimAuthenticationResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->requestIsimAuthenticationResponse(
                responseInfo,
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::requestIsimAuthenticationResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::acknowledgeIncomingGsmSmsWithPduResponse(android::Parcel &p, int slotId,
                                                   int requestNumber, int responseType,
                                                   int serial, RIL_Errno e, void *response,
                                                   size_t responseLen) {
    RLOGD("radio::acknowledgeIncomingGsmSmsWithPduResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->acknowledgeIncomingGsmSmsWithPduResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::acknowledgeIncomingGsmSmsWithPduResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::sendEnvelopeWithStatusResponse(android::Parcel &p, int slotId, int requestNumber,
                                         int responseType, int serial, RIL_Errno e, void *response,
                                         size_t responseLen) {
    RLOGD("radio::sendEnvelopeWithStatusResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        IccIoResult result = responseIccIo(responseInfo, serial, responseType, e,
                response, responseLen);

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->sendEnvelopeWithStatusResponse(responseInfo,
                result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::sendEnvelopeWithStatusResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getVoiceRadioTechnologyResponse(android::Parcel &p, int slotId, int requestNumber,
                                          int responseType, int serial, RIL_Errno e,
                                          void *response, size_t responseLen) {
    RLOGD("radio::getVoiceRadioTechnologyResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getVoiceRadioTechnologyResponse(
                responseInfo, (RadioTechnology) ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getVoiceRadioTechnologyResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getCellInfoListResponse(android::Parcel &p, int slotId,
                                   int requestNumber, int responseType,
                                   int serial, RIL_Errno e, void *response,
                                   size_t responseLen) {
    RLOGD("radio::getCellInfoListResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        hidl_vec<CellInfo> ret;
        if (response == NULL || responseLen % sizeof(RIL_CellInfo_v12) != 0) {
            RLOGE("radio::getCellInfoListResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            convertRilCellInfoListToHal(response, responseLen, ret);
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponse->getCellInfoListResponse(
                responseInfo, ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getCellInfoListResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setCellInfoListRateResponse(android::Parcel &p, int slotId,
                                       int requestNumber, int responseType,
                                       int serial, RIL_Errno e, void *response,
                                       size_t responseLen) {
    RLOGD("radio::setCellInfoListRateResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setCellInfoListRateResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setCellInfoListRateResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setInitialAttachApnResponse(android::Parcel &p, int slotId, int requestNumber,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responseLen) {
    RLOGD("radio::setInitialAttachApnResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setInitialAttachApnResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setInitialAttachApnResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getImsRegistrationStateResponse(android::Parcel &p, int slotId, int requestNumber,
                                           int responseType, int serial, RIL_Errno e,
                                           void *response, size_t responseLen) {
    RLOGD("radio::getImsRegistrationStateResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        bool isRegistered = false;
        int ratFamily = 0;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || numInts != 2) {
            RLOGE("radio::getImsRegistrationStateResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            isRegistered = pInt[0] == 1 ? true : false;
            ratFamily = pInt[1];
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getImsRegistrationStateResponse(
                responseInfo, isRegistered, (RadioTechnologyFamily) ratFamily);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getImsRegistrationStateResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::sendImsSmsResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responseLen) {
    RLOGD("radio::sendImsSmsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        SendSmsResult result = makeSendSmsResult(responseInfo, serial, responseType, e, response,
                responseLen);

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->sendImsSmsResponse(responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::sendSmsResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::iccTransmitApduBasicChannelResponse(android::Parcel &p, int slotId, int requestNumber,
                                               int responseType, int serial, RIL_Errno e,
                                               void *response, size_t responseLen) {
    RLOGD("radio::iccTransmitApduBasicChannelResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        IccIoResult result = responseIccIo(responseInfo, serial, responseType, e, response,
                responseLen);

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->iccTransmitApduBasicChannelResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::iccTransmitApduBasicChannelResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::iccOpenLogicalChannelResponse(android::Parcel &p, int slotId, int requestNumber,
                                         int responseType, int serial, RIL_Errno e, void *response,
                                         size_t responseLen) {
    RLOGD("radio::iccOpenLogicalChannelResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        int channelId = -1;
        hidl_vec<int8_t> selectResponse;
        int numInts = responseLen / sizeof(int);
        if (response == NULL || responseLen % sizeof(int) != 0) {
            RLOGE("radio::iccOpenLogicalChannelResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            int *pInt = (int *) response;
            channelId = pInt[0];
            selectResponse.resize(numInts - 1);
            for (int i = 1; i < numInts; i++) {
                selectResponse[i - 1] = (int8_t) pInt[i];
            }
        }
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->iccOpenLogicalChannelResponse(responseInfo,
                channelId, selectResponse);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::iccOpenLogicalChannelResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::iccCloseLogicalChannelResponse(android::Parcel &p, int slotId, int requestNumber,
                                          int responseType, int serial, RIL_Errno e,
                                          void *response, size_t responseLen) {
    RLOGD("radio::iccCloseLogicalChannelResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->iccCloseLogicalChannelResponse(
                responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::iccCloseLogicalChannelResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::iccTransmitApduLogicalChannelResponse(android::Parcel &p, int slotId, int requestNumber,
                                                 int responseType, int serial, RIL_Errno e,
                                                 void *response, size_t responseLen) {
    RLOGD("radio::iccTransmitApduLogicalChannelResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        IccIoResult result = responseIccIo(responseInfo, serial, responseType, e, response,
                responseLen);

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->iccTransmitApduLogicalChannelResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::iccTransmitApduLogicalChannelResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::nvReadItemResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responseLen) {
    RLOGD("radio::nvReadItemResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->nvReadItemResponse(
                responseInfo,
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::nvReadItemResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::nvWriteItemResponse(android::Parcel &p, int slotId, int requestNumber,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen) {
    RLOGD("radio::nvWriteItemResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->nvWriteItemResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::nvWriteItemResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::nvWriteCdmaPrlResponse(android::Parcel &p, int slotId, int requestNumber,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("radio::nvWriteCdmaPrlResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->nvWriteCdmaPrlResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::nvWriteCdmaPrlResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::nvResetConfigResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responseLen) {
    RLOGD("radio::nvResetConfigResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->nvResetConfigResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::nvResetConfigResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setUiccSubscriptionResponse(android::Parcel &p, int slotId, int requestNumber,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responseLen) {
    RLOGD("radio::setUiccSubscriptionResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setUiccSubscriptionResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setUiccSubscriptionResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setDataAllowedResponse(android::Parcel &p, int slotId, int requestNumber,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("radio::setDataAllowedResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setDataAllowedResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setDataAllowedResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getHardwareConfigResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("radio::getHardwareConfigResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        hidl_vec<HardwareConfig> result;
        if (response == NULL || responseLen % sizeof(RIL_HardwareConfig) != 0) {
            RLOGE("radio::hardwareConfigChangedInd: invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            convertRilHardwareConfigListToHal(response, responseLen, result);
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponse->getHardwareConfigResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getHardwareConfigResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::requestIccSimAuthenticationResponse(android::Parcel &p, int slotId, int requestNumber,
                                               int responseType, int serial, RIL_Errno e,
                                               void *response, size_t responseLen) {
    RLOGD("radio::requestIccSimAuthenticationResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        IccIoResult result = responseIccIo(responseInfo, serial, responseType, e, response,
                responseLen);

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->requestIccSimAuthenticationResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::requestIccSimAuthenticationResponse: radioService[%d]->mRadioResponse "
                "== NULL", slotId);
    }

    return 0;
}

int radio::setDataProfileResponse(android::Parcel &p, int slotId, int requestNumber,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("radio::setDataProfileResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setDataProfileResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setDataProfileResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::requestShutdownResponse(android::Parcel &p, int slotId, int requestNumber,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("radio::requestShutdownResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->requestShutdownResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::requestShutdownResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

void responseRadioCapability(RadioResponseInfo& responseInfo, int serial,
        int responseType, RIL_Errno e, void *response, size_t responseLen, RadioCapability& rc) {
    populateResponseInfo(responseInfo, serial, responseType, e);

    if (response == NULL || responseLen != sizeof(RIL_RadioCapability)) {
        RLOGE("responseRadioCapability: Invalid response");
        if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        rc.logicalModemUuid = hidl_string();
    } else {
        convertRilRadioCapabilityToHal(response, responseLen, rc);
    }
}

int radio::getRadioCapabilityResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("radio::getRadioCapabilityResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        RadioCapability result = {};
        responseRadioCapability(responseInfo, serial, responseType, e, response, responseLen,
                result);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->getRadioCapabilityResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getRadioCapabilityResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setRadioCapabilityResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e,
                                     void *response, size_t responseLen) {
    RLOGD("radio::setRadioCapabilityResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        RadioCapability result = {};
        responseRadioCapability(responseInfo, serial, responseType, e, response, responseLen,
                result);
        Return<void> retStatus = radioService[slotId]->mRadioResponse->setRadioCapabilityResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setRadioCapabilityResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

LceStatusInfo responseLceStatusInfo(RadioResponseInfo& responseInfo, int serial, int responseType,
                                    RIL_Errno e, void *response, size_t responseLen) {
    populateResponseInfo(responseInfo, serial, responseType, e);
    LceStatusInfo result = {};

    if (response == NULL || responseLen != sizeof(RIL_LceStatusInfo)) {
        RLOGE("Invalid response: NULL");
        if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
    } else {
        RIL_LceStatusInfo *resp = (RIL_LceStatusInfo *) response;
        result.lceStatus = (LceStatus) resp->lce_status;
        result.actualIntervalMs = (uint8_t) resp->actual_interval_ms;
    }
    return result;
}

int radio::startLceServiceResponse(android::Parcel &p, int slotId, int requestNumber,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
    RLOGD("radio::startLceServiceResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        LceStatusInfo result = responseLceStatusInfo(responseInfo, serial, responseType, e,
                response, responseLen);

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->startLceServiceResponse(responseInfo,
                result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::startLceServiceResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::stopLceServiceResponse(android::Parcel &p, int slotId, int requestNumber,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
    RLOGD("radio::stopLceServiceResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        LceStatusInfo result = responseLceStatusInfo(responseInfo, serial, responseType, e,
                response, responseLen);

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->stopLceServiceResponse(responseInfo,
                result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::stopLceServiceResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::pullLceDataResponse(android::Parcel &p, int slotId, int requestNumber,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen) {
    RLOGD("radio::pullLceDataResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);

        LceDataInfo result = {};
        if (response == NULL || responseLen != sizeof(RIL_LceDataInfo)) {
            RLOGE("radio::pullLceDataResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            convertRilLceDataInfoToHal(response, responseLen, result);
        }

        Return<void> retStatus = radioService[slotId]->mRadioResponse->pullLceDataResponse(
                responseInfo, result);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::pullLceDataResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::getModemActivityInfoResponse(android::Parcel &p, int slotId, int requestNumber,
                                        int responseType, int serial, RIL_Errno e,
                                        void *response, size_t responseLen) {
    RLOGD("radio::getModemActivityInfoResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        ActivityStatsInfo info;
        if (response == NULL || responseLen != sizeof(RIL_ActivityStatsInfo)) {
            RLOGE("radio::getModemActivityInfoResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            RIL_ActivityStatsInfo *resp = (RIL_ActivityStatsInfo *)response;
            info.sleepModeTimeMs = resp->sleep_mode_time_ms;
            info.idleModeTimeMs = resp->idle_mode_time_ms;
            for(int i = 0; i < RIL_NUM_TX_POWER_LEVELS; i++) {
                info.txmModetimeMs[i] = resp->tx_mode_time_ms[i];
            }
            info.rxModeTimeMs = resp->rx_mode_time_ms;
        }

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getModemActivityInfoResponse(responseInfo,
                info);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getModemActivityInfoResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::setAllowedCarriersResponse(android::Parcel &p, int slotId, int requestNumber,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen) {
    RLOGD("radio::setAllowedCarriersResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setAllowedCarriersResponse(responseInfo,
                ret);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setAllowedCarriersResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::getAllowedCarriersResponse(android::Parcel &p, int slotId, int requestNumber,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen) {
    RLOGD("radio::getAllowedCarriersResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        CarrierRestrictions carrierInfo = {};
        bool allAllowed = true;
        if (response == NULL || responseLen != sizeof(RIL_CarrierRestrictions)) {
            RLOGE("radio::getAllowedCarriersResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            RIL_CarrierRestrictions *pCr = (RIL_CarrierRestrictions *)response;
            if (pCr->len_allowed_carriers > 0 || pCr->len_excluded_carriers > 0) {
                allAllowed = false;
            }

            carrierInfo.allowedCarriers.resize(pCr->len_allowed_carriers);
            for(int i = 0; i < pCr->len_allowed_carriers; i++) {
                RIL_Carrier *carrier = pCr->allowed_carriers + i;
                carrierInfo.allowedCarriers[i].mcc = convertCharPtrToHidlString(carrier->mcc);
                carrierInfo.allowedCarriers[i].mnc = convertCharPtrToHidlString(carrier->mnc);
                carrierInfo.allowedCarriers[i].matchType = (CarrierMatchType) carrier->match_type;
                carrierInfo.allowedCarriers[i].matchData =
                        convertCharPtrToHidlString(carrier->match_data);
            }

            carrierInfo.excludedCarriers.resize(pCr->len_excluded_carriers);
            for(int i = 0; i < pCr->len_excluded_carriers; i++) {
                RIL_Carrier *carrier = pCr->excluded_carriers + i;
                carrierInfo.excludedCarriers[i].mcc = convertCharPtrToHidlString(carrier->mcc);
                carrierInfo.excludedCarriers[i].mnc = convertCharPtrToHidlString(carrier->mnc);
                carrierInfo.excludedCarriers[i].matchType = (CarrierMatchType) carrier->match_type;
                carrierInfo.excludedCarriers[i].matchData =
                        convertCharPtrToHidlString(carrier->match_data);
            }
        }

        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->getAllowedCarriersResponse(responseInfo,
                allAllowed, carrierInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::getAllowedCarriersResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::sendDeviceStateResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responselen) {
    RLOGD("radio::sendDeviceStateResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->sendDeviceStateResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::sendDeviceStateResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::setIndicationFilterResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responselen) {
    RLOGD("radio::setIndicationFilterResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setIndicationFilterResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setIndicationFilterResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}


int radio::setSimCardPowerResponse(android::Parcel &p, int slotId, int requestNumber,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responseLen) {
    RLOGD("radio::setSimCardPowerResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        Return<void> retStatus
                = radioService[slotId]->mRadioResponse->setSimCardPowerResponse(responseInfo);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::setSimCardPowerResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::sendRequestRawResponse(android::Parcel &p, int slotId, int requestNumber,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responseLen) {
   RLOGD("radio::sendRequestRawResponse: serial %d", serial);

    if (oemHookService[slotId]->mOemHookResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<uint8_t> data;

        if (response == NULL) {
            RLOGE("radio::sendRequestRawResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            data.setToExternal((uint8_t *) response, responseLen);
        }
        Return<void> retStatus = oemHookService[slotId]->mOemHookResponse->
                sendRequestRawResponse(responseInfo, data);
        checkReturnStatus(slotId, retStatus);
    } else {
        RLOGE("radio::sendRequestRawResponse: oemHookService[%d]->mOemHookResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::sendRequestStringsResponse(android::Parcel &p, int slotId, int requestNumber,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen) {
    RLOGD("radio::sendRequestStringsResponse: serial %d", serial);

    if (oemHookService[slotId]->mOemHookResponse != NULL) {
        RadioResponseInfo responseInfo = {};
        populateResponseInfo(responseInfo, serial, responseType, e);
        hidl_vec<hidl_string> data;

        if (response == NULL || responseLen % sizeof(char *) != 0) {
            RLOGE("radio::sendRequestStringsResponse Invalid response: NULL");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
        } else {
            char **resp = (char **) response;
            int numStrings = responseLen / sizeof(char *);
            data.resize(numStrings);
            for (int i = 0; i < numStrings; i++) {
                data[i] = convertCharPtrToHidlString(resp[i]);
            }
        }
        Return<void> retStatus
                = oemHookService[slotId]->mOemHookResponse->sendRequestStringsResponse(
                responseInfo, data);
        checkReturnStatus(slotId, retStatus);
    } else {
        RLOGE("radio::sendRequestStringsResponse: oemHookService[%d]->mOemHookResponse == "
                "NULL", slotId);
    }

    return 0;
}

// Radio Indication functions

RadioIndicationType convertIntToRadioIndicationType(int indicationType) {
    return indicationType == RESPONSE_UNSOLICITED ? (RadioIndicationType::UNSOLICITED) :
            (RadioIndicationType::UNSOLICITED_ACK_EXP);
}

void radio::radioStateChangedInd(int slotId, int indicationType, RIL_RadioState radioState) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::radioStateChangedInd: radioState %d", radioState);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->radioStateChanged(
                convertIntToRadioIndicationType(indicationType), (RadioState) radioState);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::radioStateChangedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }
}

int radio::callStateChangedInd(android::Parcel &p, int slotId, int requestNumber,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::callStateChangedInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->callStateChanged(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::callStateChangedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::networkStateChangedInd(android::Parcel &p, int slotId, int requestNumber,
                                       int indicationType, int token, RIL_Errno e, void *response,
                                       size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::networkStateChangedInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->networkStateChanged(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::networkStateChangedInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

uint8_t hexCharToInt(uint8_t c) {
    if (c >= '0' && c <= '9') return (c - '0');
    if (c >= 'A' && c <= 'F') return (c - 'A' + 10);
    if (c >= 'a' && c <= 'f') return (c - 'a' + 10);

    return INVALID_HEX_CHAR;
}

uint8_t * convertHexStringToBytes(void *response, size_t responseLen) {
    if (responseLen % 2 != 0) {
        return NULL;
    }

    uint8_t *bytes = (uint8_t *)calloc(responseLen/2, sizeof(uint8_t));
    if (bytes == NULL) {
        RLOGE("convertHexStringToBytes: cannot allocate memory for bytes string");
        return NULL;
    }
    uint8_t *hexString = (uint8_t *)response;

    for (size_t i = 0; i < responseLen; i += 2) {
        uint8_t hexChar1 = hexCharToInt(hexString[i]);
        uint8_t hexChar2 = hexCharToInt(hexString[i + 1]);

        if (hexChar1 == INVALID_HEX_CHAR || hexChar2 == INVALID_HEX_CHAR) {
            RLOGE("convertHexStringToBytes: invalid hex char %d %d",
                    hexString[i], hexString[i + 1]);
            free(bytes);
            return NULL;
        }
        bytes[i/2] = ((hexChar1 << 4) | hexChar2);
    }

    return bytes;
}

int radio::newSmsInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                     int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("radio::newSmsInd: invalid response");
            return 0;
        }

        uint8_t *bytes = convertHexStringToBytes(response, responseLen);
        if (bytes == NULL) {
            RLOGE("radio::newSmsInd: convertHexStringToBytes failed");
            return 0;
        }

        hidl_vec<uint8_t> pdu;
        pdu.setToExternal(bytes, responseLen/2);
        RLOGD("radio::newSmsInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->newSms(
                convertIntToRadioIndicationType(indicationType), pdu);
        radioService[slotId]->checkReturnStatus(retStatus);
        free(bytes);
    } else {
        RLOGE("radio::newSmsInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::newSmsStatusReportInd(android::Parcel &p, int slotId, int requestNumber,
                                 int indicationType, int token, RIL_Errno e, void *response,
                                 size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("radio::newSmsStatusReportInd: invalid response");
            return 0;
        }

        uint8_t *bytes = convertHexStringToBytes(response, responseLen);
        if (bytes == NULL) {
            RLOGE("radio::newSmsStatusReportInd: convertHexStringToBytes failed");
            return 0;
        }

        hidl_vec<uint8_t> pdu;
        pdu.setToExternal(bytes, responseLen/2);
        RLOGD("radio::newSmsStatusReportInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->newSmsStatusReport(
                convertIntToRadioIndicationType(indicationType), pdu);
        radioService[slotId]->checkReturnStatus(retStatus);
        free(bytes);
    } else {
        RLOGE("radio::newSmsStatusReportInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::newSmsOnSimInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                          int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("radio::newSmsOnSimInd: invalid response");
            return 0;
        }
        int32_t recordNumber = ((int32_t *) response)[0];
        RLOGD("radio::newSmsOnSimInd: slotIndex %d", recordNumber);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->newSmsOnSim(
                convertIntToRadioIndicationType(indicationType), recordNumber);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::newSmsOnSimInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::onUssdInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                     int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != 2 * sizeof(char *)) {
            RLOGE("radio::onUssdInd: invalid response");
            return 0;
        }
        char **strings = (char **) response;
        char *mode = strings[0];
        hidl_string msg = convertCharPtrToHidlString(strings[1]);
        UssdModeType modeType = (UssdModeType) atoi(mode);
        RLOGD("radio::onUssdInd: mode %s", mode);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->onUssd(
                convertIntToRadioIndicationType(indicationType), modeType, msg);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::onUssdInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::nitzTimeReceivedInd(android::Parcel &p, int slotId, int requestNumber,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("radio::nitzTimeReceivedInd: invalid response");
            return 0;
        }
        hidl_string nitzTime = convertCharPtrToHidlString((char *) response);
        int64_t timeReceived = android::elapsedRealtime();
        RLOGD("radio::nitzTimeReceivedInd: nitzTime %s receivedTime %" PRId64, nitzTime.c_str(),
                timeReceived);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->nitzTimeReceived(
                convertIntToRadioIndicationType(indicationType), nitzTime, timeReceived);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::nitzTimeReceivedInd: radioService[%d]->mRadioIndication == NULL", slotId);
        return -1;
    }

    return 0;
}

void convertRilSignalStrengthToHal(void *response, size_t responseLen,
        SignalStrength& signalStrength) {
    RIL_SignalStrength_v10 *rilSignalStrength = (RIL_SignalStrength_v10 *) response;

    // Fixup LTE for backwards compatibility
    // signalStrength: -1 -> 99
    if (rilSignalStrength->LTE_SignalStrength.signalStrength == -1) {
        rilSignalStrength->LTE_SignalStrength.signalStrength = 99;
    }
    // rsrp: -1 -> INT_MAX all other negative value to positive.
    // So remap here
    if (rilSignalStrength->LTE_SignalStrength.rsrp == -1) {
        rilSignalStrength->LTE_SignalStrength.rsrp = INT_MAX;
    } else if (rilSignalStrength->LTE_SignalStrength.rsrp < -1) {
        rilSignalStrength->LTE_SignalStrength.rsrp = -rilSignalStrength->LTE_SignalStrength.rsrp;
    }
    // rsrq: -1 -> INT_MAX
    if (rilSignalStrength->LTE_SignalStrength.rsrq == -1) {
        rilSignalStrength->LTE_SignalStrength.rsrq = INT_MAX;
    }
    // Not remapping rssnr is already using INT_MAX
    // cqi: -1 -> INT_MAX
    if (rilSignalStrength->LTE_SignalStrength.cqi == -1) {
        rilSignalStrength->LTE_SignalStrength.cqi = INT_MAX;
    }

    signalStrength.gw.signalStrength = rilSignalStrength->GW_SignalStrength.signalStrength;
    signalStrength.gw.bitErrorRate = rilSignalStrength->GW_SignalStrength.bitErrorRate;
    signalStrength.cdma.dbm = rilSignalStrength->CDMA_SignalStrength.dbm;
    signalStrength.cdma.ecio = rilSignalStrength->CDMA_SignalStrength.ecio;
    signalStrength.evdo.dbm = rilSignalStrength->EVDO_SignalStrength.dbm;
    signalStrength.evdo.ecio = rilSignalStrength->EVDO_SignalStrength.ecio;
    signalStrength.evdo.signalNoiseRatio =
            rilSignalStrength->EVDO_SignalStrength.signalNoiseRatio;
    signalStrength.lte.signalStrength = rilSignalStrength->LTE_SignalStrength.signalStrength;
    signalStrength.lte.rsrp = rilSignalStrength->LTE_SignalStrength.rsrp;
    signalStrength.lte.rsrq = rilSignalStrength->LTE_SignalStrength.rsrq;
    signalStrength.lte.rssnr = rilSignalStrength->LTE_SignalStrength.rssnr;
    signalStrength.lte.cqi = rilSignalStrength->LTE_SignalStrength.cqi;
    signalStrength.lte.timingAdvance = rilSignalStrength->LTE_SignalStrength.timingAdvance;
    signalStrength.tdScdma.rscp = rilSignalStrength->TD_SCDMA_SignalStrength.rscp;
}

int radio::currentSignalStrengthInd(android::Parcel &p, int slotId, int requestNumber,
                                    int indicationType, int token, RIL_Errno e,
                                    void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_SignalStrength_v10)) {
            RLOGE("radio::currentSignalStrengthInd: invalid response");
            return 0;
        }

        SignalStrength signalStrength = {};
        convertRilSignalStrengthToHal(response, responseLen, signalStrength);

        RLOGD("radio::currentSignalStrengthInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->currentSignalStrength(
                convertIntToRadioIndicationType(indicationType), signalStrength);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::currentSignalStrengthInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

void convertRilDataCallToHal(RIL_Data_Call_Response_v11 *dcResponse,
        SetupDataCallResult& dcResult) {
    dcResult.status = (DataCallFailCause) dcResponse->status;
    dcResult.suggestedRetryTime = dcResponse->suggestedRetryTime;
    dcResult.cid = dcResponse->cid;
    dcResult.active = dcResponse->active;
    dcResult.type = convertCharPtrToHidlString(dcResponse->type);
    dcResult.ifname = convertCharPtrToHidlString(dcResponse->ifname);
    dcResult.addresses = convertCharPtrToHidlString(dcResponse->addresses);
    dcResult.dnses = convertCharPtrToHidlString(dcResponse->dnses);
    dcResult.gateways = convertCharPtrToHidlString(dcResponse->gateways);
    dcResult.pcscf = convertCharPtrToHidlString(dcResponse->pcscf);
    dcResult.mtu = dcResponse->mtu;
}

void convertRilDataCallListToHal(void *response, size_t responseLen,
        hidl_vec<SetupDataCallResult>& dcResultList) {
    int num = responseLen / sizeof(RIL_Data_Call_Response_v11);

    RIL_Data_Call_Response_v11 *dcResponse = (RIL_Data_Call_Response_v11 *) response;
    dcResultList.resize(num);
    for (int i = 0; i < num; i++) {
        convertRilDataCallToHal(&dcResponse[i], dcResultList[i]);
    }
}

int radio::dataCallListChangedInd(android::Parcel &p, int slotId, int requestNumber,
                                  int indicationType, int token, RIL_Errno e, void *response,
                                  size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen % sizeof(RIL_Data_Call_Response_v11) != 0) {
            RLOGE("radio::dataCallListChangedInd: invalid response");
            return 0;
        }
        hidl_vec<SetupDataCallResult> dcList;
        convertRilDataCallListToHal(response, responseLen, dcList);
        RLOGD("radio::dataCallListChangedInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->dataCallListChanged(
                convertIntToRadioIndicationType(indicationType), dcList);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::dataCallListChangedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::suppSvcNotifyInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                            int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_SuppSvcNotification)) {
            RLOGE("radio::suppSvcNotifyInd: invalid response");
            return 0;
        }

        SuppSvcNotification suppSvc = {};
        RIL_SuppSvcNotification *ssn = (RIL_SuppSvcNotification *) response;
        suppSvc.isMT = ssn->notificationType;
        suppSvc.code = ssn->code;
        suppSvc.index = ssn->index;
        suppSvc.type = ssn->type;
        suppSvc.number = convertCharPtrToHidlString(ssn->number);

        RLOGD("radio::suppSvcNotifyInd: isMT %d code %d index %d type %d",
                suppSvc.isMT, suppSvc.code, suppSvc.index, suppSvc.type);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->suppSvcNotify(
                convertIntToRadioIndicationType(indicationType), suppSvc);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::suppSvcNotifyInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::stkSessionEndInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                            int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::stkSessionEndInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->stkSessionEnd(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::stkSessionEndInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::stkProactiveCommandInd(android::Parcel &p, int slotId, int requestNumber,
                                  int indicationType, int token, RIL_Errno e, void *response,
                                  size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("radio::stkProactiveCommandInd: invalid response");
            return 0;
        }
        RLOGD("radio::stkProactiveCommandInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->stkProactiveCommand(
                convertIntToRadioIndicationType(indicationType),
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::stkProactiveCommandInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::stkEventNotifyInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                             int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("radio::stkEventNotifyInd: invalid response");
            return 0;
        }
        RLOGD("radio::stkEventNotifyInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->stkEventNotify(
                convertIntToRadioIndicationType(indicationType),
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::stkEventNotifyInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::stkCallSetupInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                           int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("radio::stkCallSetupInd: invalid response");
            return 0;
        }
        int32_t timeout = ((int32_t *) response)[0];
        RLOGD("radio::stkCallSetupInd: timeout %d", timeout);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->stkCallSetup(
                convertIntToRadioIndicationType(indicationType), timeout);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::stkCallSetupInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::simSmsStorageFullInd(android::Parcel &p, int slotId, int requestNumber,
                                int indicationType, int token, RIL_Errno e, void *response,
                                size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::simSmsStorageFullInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->simSmsStorageFull(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::simSmsStorageFullInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::simRefreshInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                         int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_SimRefreshResponse_v7)) {
            RLOGE("radio::simRefreshInd: invalid response");
            return 0;
        }

        SimRefreshResult refreshResult = {};
        RIL_SimRefreshResponse_v7 *simRefreshResponse = ((RIL_SimRefreshResponse_v7 *) response);
        refreshResult.type =
                (android::hardware::radio::V1_0::SimRefreshType) simRefreshResponse->result;
        refreshResult.efId = simRefreshResponse->ef_id;
        refreshResult.aid = convertCharPtrToHidlString(simRefreshResponse->aid);

        RLOGD("radio::simRefreshInd: type %d efId %d", refreshResult.type, refreshResult.efId);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->simRefresh(
                convertIntToRadioIndicationType(indicationType), refreshResult);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::simRefreshInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

void convertRilCdmaSignalInfoRecordToHal(RIL_CDMA_SignalInfoRecord *signalInfoRecord,
        CdmaSignalInfoRecord& record) {
    record.isPresent = signalInfoRecord->isPresent;
    record.signalType = signalInfoRecord->signalType;
    record.alertPitch = signalInfoRecord->alertPitch;
    record.signal = signalInfoRecord->signal;
}

int radio::callRingInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                       int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        bool isGsm;
        CdmaSignalInfoRecord record = {};
        if (response == NULL || responseLen == 0) {
            isGsm = true;
        } else {
            isGsm = false;
            if (responseLen != sizeof (RIL_CDMA_SignalInfoRecord)) {
                RLOGE("radio::callRingInd: invalid response");
                return 0;
            }
            convertRilCdmaSignalInfoRecordToHal((RIL_CDMA_SignalInfoRecord *) response, record);
        }

        RLOGD("radio::callRingInd: isGsm %d", isGsm);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->callRing(
                convertIntToRadioIndicationType(indicationType), isGsm, record);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::callRingInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::simStatusChangedInd(android::Parcel &p, int slotId, int requestNumber,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::simStatusChangedInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->simStatusChanged(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::simStatusChangedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::cdmaNewSmsInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                         int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_CDMA_SMS_Message)) {
            RLOGE("radio::cdmaNewSmsInd: invalid response");
            return 0;
        }

        CdmaSmsMessage msg = {};
        RIL_CDMA_SMS_Message *rilMsg = (RIL_CDMA_SMS_Message *) response;
        msg.teleserviceId = rilMsg->uTeleserviceID;
        msg.isServicePresent = rilMsg->bIsServicePresent;
        msg.serviceCategory = rilMsg->uServicecategory;
        msg.address.digitMode =
                (android::hardware::radio::V1_0::CdmaSmsDigitMode) rilMsg->sAddress.digit_mode;
        msg.address.numberMode =
                (android::hardware::radio::V1_0::CdmaSmsNumberMode) rilMsg->sAddress.number_mode;
        msg.address.numberType =
                (android::hardware::radio::V1_0::CdmaSmsNumberType) rilMsg->sAddress.number_type;
        msg.address.numberPlan =
                (android::hardware::radio::V1_0::CdmaSmsNumberPlan) rilMsg->sAddress.number_plan;

        int digitLimit = MIN((rilMsg->sAddress.number_of_digits), RIL_CDMA_SMS_ADDRESS_MAX);
        msg.address.digits.setToExternal(rilMsg->sAddress.digits, digitLimit);

        msg.subAddress.subaddressType = (android::hardware::radio::V1_0::CdmaSmsSubaddressType)
                rilMsg->sSubAddress.subaddressType;
        msg.subAddress.odd = rilMsg->sSubAddress.odd;

        digitLimit= MIN((rilMsg->sSubAddress.number_of_digits), RIL_CDMA_SMS_SUBADDRESS_MAX);
        msg.subAddress.digits.setToExternal(rilMsg->sSubAddress.digits, digitLimit);

        digitLimit = MIN((rilMsg->uBearerDataLen), RIL_CDMA_SMS_BEARER_DATA_MAX);
        msg.bearerData.setToExternal(rilMsg->aBearerData, digitLimit);

        RLOGD("radio::cdmaNewSmsInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->cdmaNewSms(
                convertIntToRadioIndicationType(indicationType), msg);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::cdmaNewSmsInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::newBroadcastSmsInd(android::Parcel &p, int slotId, int requestNumber,
                              int indicationType, int token, RIL_Errno e, void *response,
                              size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("radio::newBroadcastSmsInd: invalid response");
            return 0;
        }

        hidl_vec<uint8_t> data;
        data.setToExternal((uint8_t *) response, responseLen);
        RLOGD("radio::newBroadcastSmsInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->newBroadcastSms(
                convertIntToRadioIndicationType(indicationType), data);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::newBroadcastSmsInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::cdmaRuimSmsStorageFullInd(android::Parcel &p, int slotId, int requestNumber,
                                     int indicationType, int token, RIL_Errno e, void *response,
                                     size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::cdmaRuimSmsStorageFullInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->cdmaRuimSmsStorageFull(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::cdmaRuimSmsStorageFullInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::restrictedStateChangedInd(android::Parcel &p, int slotId, int requestNumber,
                                     int indicationType, int token, RIL_Errno e, void *response,
                                     size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("radio::restrictedStateChangedInd: invalid response");
            return 0;
        }
        int32_t state = ((int32_t *) response)[0];
        RLOGD("radio::restrictedStateChangedInd: state %d", state);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->restrictedStateChanged(
                convertIntToRadioIndicationType(indicationType), (PhoneRestrictedState) state);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::restrictedStateChangedInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::enterEmergencyCallbackModeInd(android::Parcel &p, int slotId, int requestNumber,
                                         int indicationType, int token, RIL_Errno e, void *response,
                                         size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::enterEmergencyCallbackModeInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->enterEmergencyCallbackMode(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::enterEmergencyCallbackModeInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::cdmaCallWaitingInd(android::Parcel &p, int slotId, int requestNumber,
                              int indicationType, int token, RIL_Errno e, void *response,
                              size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_CDMA_CallWaiting_v6)) {
            RLOGE("radio::cdmaCallWaitingInd: invalid response");
            return 0;
        }

        CdmaCallWaiting callWaitingRecord = {};
        RIL_CDMA_CallWaiting_v6 *callWaitingRil = ((RIL_CDMA_CallWaiting_v6 *) response);
        callWaitingRecord.number = convertCharPtrToHidlString(callWaitingRil->number);
        callWaitingRecord.numberPresentation =
                (CdmaCallWaitingNumberPresentation) callWaitingRil->numberPresentation;
        callWaitingRecord.name = convertCharPtrToHidlString(callWaitingRil->name);
        convertRilCdmaSignalInfoRecordToHal(&callWaitingRil->signalInfoRecord,
                callWaitingRecord.signalInfoRecord);
        callWaitingRecord.numberType = (CdmaCallWaitingNumberType) callWaitingRil->number_type;
        callWaitingRecord.numberPlan = (CdmaCallWaitingNumberPlan) callWaitingRil->number_plan;

        RLOGD("radio::cdmaCallWaitingInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->cdmaCallWaiting(
                convertIntToRadioIndicationType(indicationType), callWaitingRecord);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::cdmaCallWaitingInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::cdmaOtaProvisionStatusInd(android::Parcel &p, int slotId, int requestNumber,
                                     int indicationType, int token, RIL_Errno e, void *response,
                                     size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("radio::cdmaOtaProvisionStatusInd: invalid response");
            return 0;
        }
        int32_t status = ((int32_t *) response)[0];
        RLOGD("radio::cdmaOtaProvisionStatusInd: status %d", status);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->cdmaOtaProvisionStatus(
                convertIntToRadioIndicationType(indicationType), (CdmaOtaProvisionStatus) status);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::cdmaOtaProvisionStatusInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::cdmaInfoRecInd(android::Parcel &p, int slotId, int requestNumber,
                          int indicationType, int token, RIL_Errno e, void *response,
                          size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_CDMA_InformationRecords)) {
            RLOGE("radio::cdmaInfoRecInd: invalid response");
            return 0;
        }

        CdmaInformationRecords records = {};
        RIL_CDMA_InformationRecords *recordsRil = (RIL_CDMA_InformationRecords *) response;

        char* string8 = NULL;
        int num = MIN(recordsRil->numberOfInfoRecs, RIL_CDMA_MAX_NUMBER_OF_INFO_RECS);
        if (recordsRil->numberOfInfoRecs > RIL_CDMA_MAX_NUMBER_OF_INFO_RECS) {
            RLOGE("radio::cdmaInfoRecInd: received %d recs which is more than %d, dropping "
                    "additional ones", recordsRil->numberOfInfoRecs,
                    RIL_CDMA_MAX_NUMBER_OF_INFO_RECS);
        }
        records.infoRec.resize(num);
        for (int i = 0 ; i < num ; i++) {
            CdmaInformationRecord *record = &records.infoRec[i];
            RIL_CDMA_InformationRecord *infoRec = &recordsRil->infoRec[i];
            record->name = (CdmaInfoRecName) infoRec->name;
            // All vectors should be size 0 except one which will be size 1. Set everything to
            // size 0 initially.
            record->display.resize(0);
            record->number.resize(0);
            record->signal.resize(0);
            record->redir.resize(0);
            record->lineCtrl.resize(0);
            record->clir.resize(0);
            record->audioCtrl.resize(0);
            switch (infoRec->name) {
                case RIL_CDMA_DISPLAY_INFO_REC:
                case RIL_CDMA_EXTENDED_DISPLAY_INFO_REC: {
                    if (infoRec->rec.display.alpha_len > CDMA_ALPHA_INFO_BUFFER_LENGTH) {
                        RLOGE("radio::cdmaInfoRecInd: invalid display info response length %d "
                                "expected not more than %d", (int) infoRec->rec.display.alpha_len,
                                CDMA_ALPHA_INFO_BUFFER_LENGTH);
                        return 0;
                    }
                    string8 = (char*) malloc((infoRec->rec.display.alpha_len + 1) * sizeof(char));
                    if (string8 == NULL) {
                        RLOGE("radio::cdmaInfoRecInd: Memory allocation failed for "
                                "responseCdmaInformationRecords");
                        return 0;
                    }
                    memcpy(string8, infoRec->rec.display.alpha_buf, infoRec->rec.display.alpha_len);
                    string8[(int)infoRec->rec.display.alpha_len] = '\0';

                    record->display.resize(1);
                    record->display[0].alphaBuf = string8;
                    free(string8);
                    string8 = NULL;
                    break;
                }

                case RIL_CDMA_CALLED_PARTY_NUMBER_INFO_REC:
                case RIL_CDMA_CALLING_PARTY_NUMBER_INFO_REC:
                case RIL_CDMA_CONNECTED_NUMBER_INFO_REC: {
                    if (infoRec->rec.number.len > CDMA_NUMBER_INFO_BUFFER_LENGTH) {
                        RLOGE("radio::cdmaInfoRecInd: invalid display info response length %d "
                                "expected not more than %d", (int) infoRec->rec.number.len,
                                CDMA_NUMBER_INFO_BUFFER_LENGTH);
                        return 0;
                    }
                    string8 = (char*) malloc((infoRec->rec.number.len + 1) * sizeof(char));
                    if (string8 == NULL) {
                        RLOGE("radio::cdmaInfoRecInd: Memory allocation failed for "
                                "responseCdmaInformationRecords");
                        return 0;
                    }
                    memcpy(string8, infoRec->rec.number.buf, infoRec->rec.number.len);
                    string8[(int)infoRec->rec.number.len] = '\0';

                    record->number.resize(1);
                    record->number[0].number = string8;
                    free(string8);
                    string8 = NULL;
                    record->number[0].numberType = infoRec->rec.number.number_type;
                    record->number[0].numberPlan = infoRec->rec.number.number_plan;
                    record->number[0].pi = infoRec->rec.number.pi;
                    record->number[0].si = infoRec->rec.number.si;
                    break;
                }

                case RIL_CDMA_SIGNAL_INFO_REC: {
                    record->signal.resize(1);
                    record->signal[0].isPresent = infoRec->rec.signal.isPresent;
                    record->signal[0].signalType = infoRec->rec.signal.signalType;
                    record->signal[0].alertPitch = infoRec->rec.signal.alertPitch;
                    record->signal[0].signal = infoRec->rec.signal.signal;
                    break;
                }

                case RIL_CDMA_REDIRECTING_NUMBER_INFO_REC: {
                    if (infoRec->rec.redir.redirectingNumber.len >
                                                  CDMA_NUMBER_INFO_BUFFER_LENGTH) {
                        RLOGE("radio::cdmaInfoRecInd: invalid display info response length %d "
                                "expected not more than %d\n",
                                (int)infoRec->rec.redir.redirectingNumber.len,
                                CDMA_NUMBER_INFO_BUFFER_LENGTH);
                        return 0;
                    }
                    string8 = (char*) malloc((infoRec->rec.redir.redirectingNumber.len + 1) *
                            sizeof(char));
                    if (string8 == NULL) {
                        RLOGE("radio::cdmaInfoRecInd: Memory allocation failed for "
                                "responseCdmaInformationRecords");
                        return 0;
                    }
                    memcpy(string8, infoRec->rec.redir.redirectingNumber.buf,
                            infoRec->rec.redir.redirectingNumber.len);
                    string8[(int)infoRec->rec.redir.redirectingNumber.len] = '\0';

                    record->redir.resize(1);
                    record->redir[0].redirectingNumber.number = string8;
                    free(string8);
                    string8 = NULL;
                    record->redir[0].redirectingNumber.numberType =
                            infoRec->rec.redir.redirectingNumber.number_type;
                    record->redir[0].redirectingNumber.numberPlan =
                            infoRec->rec.redir.redirectingNumber.number_plan;
                    record->redir[0].redirectingNumber.pi = infoRec->rec.redir.redirectingNumber.pi;
                    record->redir[0].redirectingNumber.si = infoRec->rec.redir.redirectingNumber.si;
                    record->redir[0].redirectingReason =
                            (CdmaRedirectingReason) infoRec->rec.redir.redirectingReason;
                    break;
                }

                case RIL_CDMA_LINE_CONTROL_INFO_REC: {
                    record->lineCtrl.resize(1);
                    record->lineCtrl[0].lineCtrlPolarityIncluded =
                            infoRec->rec.lineCtrl.lineCtrlPolarityIncluded;
                    record->lineCtrl[0].lineCtrlToggle = infoRec->rec.lineCtrl.lineCtrlToggle;
                    record->lineCtrl[0].lineCtrlReverse = infoRec->rec.lineCtrl.lineCtrlReverse;
                    record->lineCtrl[0].lineCtrlPowerDenial =
                            infoRec->rec.lineCtrl.lineCtrlPowerDenial;
                    break;
                }

                case RIL_CDMA_T53_CLIR_INFO_REC: {
                    record->clir.resize(1);
                    record->clir[0].cause = infoRec->rec.clir.cause;
                    break;
                }

                case RIL_CDMA_T53_AUDIO_CONTROL_INFO_REC: {
                    record->audioCtrl.resize(1);
                    record->audioCtrl[0].upLink = infoRec->rec.audioCtrl.upLink;
                    record->audioCtrl[0].downLink = infoRec->rec.audioCtrl.downLink;
                    break;
                }

                case RIL_CDMA_T53_RELEASE_INFO_REC:
                    RLOGE("radio::cdmaInfoRecInd: RIL_CDMA_T53_RELEASE_INFO_REC: INVALID");
                    return 0;

                default:
                    RLOGE("radio::cdmaInfoRecInd: Incorrect name value");
                    return 0;
            }
        }

        RLOGD("radio::cdmaInfoRecInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->cdmaInfoRec(
                convertIntToRadioIndicationType(indicationType), records);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::cdmaInfoRecInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::indicateRingbackToneInd(android::Parcel &p, int slotId, int requestNumber,
                                   int indicationType, int token, RIL_Errno e, void *response,
                                   size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("radio::indicateRingbackToneInd: invalid response");
            return 0;
        }
        bool start = ((int32_t *) response)[0];
        RLOGD("radio::indicateRingbackToneInd: start %d", start);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->indicateRingbackTone(
                convertIntToRadioIndicationType(indicationType), start);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::indicateRingbackToneInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::resendIncallMuteInd(android::Parcel &p, int slotId, int requestNumber,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::resendIncallMuteInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->resendIncallMute(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::resendIncallMuteInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::cdmaSubscriptionSourceChangedInd(android::Parcel &p, int slotId, int requestNumber,
                                            int indicationType, int token, RIL_Errno e,
                                            void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("radio::cdmaSubscriptionSourceChangedInd: invalid response");
            return 0;
        }
        int32_t cdmaSource = ((int32_t *) response)[0];
        RLOGD("radio::cdmaSubscriptionSourceChangedInd: cdmaSource %d", cdmaSource);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->
                cdmaSubscriptionSourceChanged(convertIntToRadioIndicationType(indicationType),
                (CdmaSubscriptionSource) cdmaSource);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::cdmaSubscriptionSourceChangedInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::cdmaPrlChangedInd(android::Parcel &p, int slotId, int requestNumber,
                             int indicationType, int token, RIL_Errno e, void *response,
                             size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("radio::cdmaPrlChangedInd: invalid response");
            return 0;
        }
        int32_t version = ((int32_t *) response)[0];
        RLOGD("radio::cdmaPrlChangedInd: version %d", version);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->cdmaPrlChanged(
                convertIntToRadioIndicationType(indicationType), version);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::cdmaPrlChangedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::exitEmergencyCallbackModeInd(android::Parcel &p, int slotId, int requestNumber,
                                        int indicationType, int token, RIL_Errno e, void *response,
                                        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::exitEmergencyCallbackModeInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->exitEmergencyCallbackMode(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::exitEmergencyCallbackModeInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::rilConnectedInd(android::Parcel &p, int slotId, int requestNumber,
                           int indicationType, int token, RIL_Errno e, void *response,
                           size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::rilConnectedInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->rilConnected(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::rilConnectedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::voiceRadioTechChangedInd(android::Parcel &p, int slotId, int requestNumber,
                                    int indicationType, int token, RIL_Errno e, void *response,
                                    size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("radio::voiceRadioTechChangedInd: invalid response");
            return 0;
        }
        int32_t rat = ((int32_t *) response)[0];
        RLOGD("radio::voiceRadioTechChangedInd: rat %d", rat);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->voiceRadioTechChanged(
                convertIntToRadioIndicationType(indicationType), (RadioTechnology) rat);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::voiceRadioTechChangedInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

void convertRilCellInfoListToHal(void *response, size_t responseLen, hidl_vec<CellInfo>& records) {
    int num = responseLen / sizeof(RIL_CellInfo_v12);
    records.resize(num);

    RIL_CellInfo_v12 *rillCellInfo = (RIL_CellInfo_v12 *) response;
    for (int i = 0; i < num; i++) {
        records[i].cellInfoType = (CellInfoType) rillCellInfo->cellInfoType;
        records[i].registered = rillCellInfo->registered;
        records[i].timeStampType = (TimeStampType) rillCellInfo->timeStampType;
        records[i].timeStamp = rillCellInfo->timeStamp;
        // All vectors should be size 0 except one which will be size 1. Set everything to
        // size 0 initially.
        records[i].gsm.resize(0);
        records[i].wcdma.resize(0);
        records[i].cdma.resize(0);
        records[i].lte.resize(0);
        records[i].tdscdma.resize(0);
        switch(rillCellInfo->cellInfoType) {
            case RIL_CELL_INFO_TYPE_GSM: {
                records[i].gsm.resize(1);
                CellInfoGsm *cellInfoGsm = &records[i].gsm[0];
                cellInfoGsm->cellIdentityGsm.mcc =
                        std::to_string(rillCellInfo->CellInfo.gsm.cellIdentityGsm.mcc);
                cellInfoGsm->cellIdentityGsm.mnc =
                        std::to_string(rillCellInfo->CellInfo.gsm.cellIdentityGsm.mnc);
                cellInfoGsm->cellIdentityGsm.lac =
                        rillCellInfo->CellInfo.gsm.cellIdentityGsm.lac;
                cellInfoGsm->cellIdentityGsm.cid =
                        rillCellInfo->CellInfo.gsm.cellIdentityGsm.cid;
                cellInfoGsm->cellIdentityGsm.arfcn =
                        rillCellInfo->CellInfo.gsm.cellIdentityGsm.arfcn;
                cellInfoGsm->cellIdentityGsm.bsic =
                        rillCellInfo->CellInfo.gsm.cellIdentityGsm.bsic;
                cellInfoGsm->signalStrengthGsm.signalStrength =
                        rillCellInfo->CellInfo.gsm.signalStrengthGsm.signalStrength;
                cellInfoGsm->signalStrengthGsm.bitErrorRate =
                        rillCellInfo->CellInfo.gsm.signalStrengthGsm.bitErrorRate;
                cellInfoGsm->signalStrengthGsm.timingAdvance =
                        rillCellInfo->CellInfo.gsm.signalStrengthGsm.timingAdvance;
                break;
            }

            case RIL_CELL_INFO_TYPE_WCDMA: {
                records[i].wcdma.resize(1);
                CellInfoWcdma *cellInfoWcdma = &records[i].wcdma[0];
                cellInfoWcdma->cellIdentityWcdma.mcc =
                        std::to_string(rillCellInfo->CellInfo.wcdma.cellIdentityWcdma.mcc);
                cellInfoWcdma->cellIdentityWcdma.mnc =
                        std::to_string(rillCellInfo->CellInfo.wcdma.cellIdentityWcdma.mnc);
                cellInfoWcdma->cellIdentityWcdma.lac =
                        rillCellInfo->CellInfo.wcdma.cellIdentityWcdma.lac;
                cellInfoWcdma->cellIdentityWcdma.cid =
                        rillCellInfo->CellInfo.wcdma.cellIdentityWcdma.cid;
                cellInfoWcdma->cellIdentityWcdma.psc =
                        rillCellInfo->CellInfo.wcdma.cellIdentityWcdma.psc;
                cellInfoWcdma->cellIdentityWcdma.uarfcn =
                        rillCellInfo->CellInfo.wcdma.cellIdentityWcdma.uarfcn;
                cellInfoWcdma->signalStrengthWcdma.signalStrength =
                        rillCellInfo->CellInfo.wcdma.signalStrengthWcdma.signalStrength;
                cellInfoWcdma->signalStrengthWcdma.bitErrorRate =
                        rillCellInfo->CellInfo.wcdma.signalStrengthWcdma.bitErrorRate;
                break;
            }

            case RIL_CELL_INFO_TYPE_CDMA: {
                records[i].cdma.resize(1);
                CellInfoCdma *cellInfoCdma = &records[i].cdma[0];
                cellInfoCdma->cellIdentityCdma.networkId =
                        rillCellInfo->CellInfo.cdma.cellIdentityCdma.networkId;
                cellInfoCdma->cellIdentityCdma.systemId =
                        rillCellInfo->CellInfo.cdma.cellIdentityCdma.systemId;
                cellInfoCdma->cellIdentityCdma.baseStationId =
                        rillCellInfo->CellInfo.cdma.cellIdentityCdma.basestationId;
                cellInfoCdma->cellIdentityCdma.longitude =
                        rillCellInfo->CellInfo.cdma.cellIdentityCdma.longitude;
                cellInfoCdma->cellIdentityCdma.latitude =
                        rillCellInfo->CellInfo.cdma.cellIdentityCdma.latitude;
                cellInfoCdma->signalStrengthCdma.dbm =
                        rillCellInfo->CellInfo.cdma.signalStrengthCdma.dbm;
                cellInfoCdma->signalStrengthCdma.ecio =
                        rillCellInfo->CellInfo.cdma.signalStrengthCdma.ecio;
                cellInfoCdma->signalStrengthEvdo.dbm =
                        rillCellInfo->CellInfo.cdma.signalStrengthEvdo.dbm;
                cellInfoCdma->signalStrengthEvdo.ecio =
                        rillCellInfo->CellInfo.cdma.signalStrengthEvdo.ecio;
                cellInfoCdma->signalStrengthEvdo.signalNoiseRatio =
                        rillCellInfo->CellInfo.cdma.signalStrengthEvdo.signalNoiseRatio;
                break;
            }

            case RIL_CELL_INFO_TYPE_LTE: {
                records[i].lte.resize(1);
                CellInfoLte *cellInfoLte = &records[i].lte[0];
                cellInfoLte->cellIdentityLte.mcc =
                        std::to_string(rillCellInfo->CellInfo.lte.cellIdentityLte.mcc);
                cellInfoLte->cellIdentityLte.mnc =
                        std::to_string(rillCellInfo->CellInfo.lte.cellIdentityLte.mnc);
                cellInfoLte->cellIdentityLte.ci =
                        rillCellInfo->CellInfo.lte.cellIdentityLte.ci;
                cellInfoLte->cellIdentityLte.pci =
                        rillCellInfo->CellInfo.lte.cellIdentityLte.pci;
                cellInfoLte->cellIdentityLte.tac =
                        rillCellInfo->CellInfo.lte.cellIdentityLte.tac;
                cellInfoLte->cellIdentityLte.earfcn =
                        rillCellInfo->CellInfo.lte.cellIdentityLte.earfcn;
                cellInfoLte->signalStrengthLte.signalStrength =
                        rillCellInfo->CellInfo.lte.signalStrengthLte.signalStrength;
                cellInfoLte->signalStrengthLte.rsrp =
                        rillCellInfo->CellInfo.lte.signalStrengthLte.rsrp;
                cellInfoLte->signalStrengthLte.rsrq =
                        rillCellInfo->CellInfo.lte.signalStrengthLte.rsrq;
                cellInfoLte->signalStrengthLte.rssnr =
                        rillCellInfo->CellInfo.lte.signalStrengthLte.rssnr;
                cellInfoLte->signalStrengthLte.cqi =
                        rillCellInfo->CellInfo.lte.signalStrengthLte.cqi;
                cellInfoLte->signalStrengthLte.timingAdvance =
                        rillCellInfo->CellInfo.lte.signalStrengthLte.timingAdvance;
                break;
            }

            case RIL_CELL_INFO_TYPE_TD_SCDMA: {
                records[i].tdscdma.resize(1);
                CellInfoTdscdma *cellInfoTdscdma = &records[i].tdscdma[0];
                cellInfoTdscdma->cellIdentityTdscdma.mcc =
                        std::to_string(rillCellInfo->CellInfo.tdscdma.cellIdentityTdscdma.mcc);
                cellInfoTdscdma->cellIdentityTdscdma.mnc =
                        std::to_string(rillCellInfo->CellInfo.tdscdma.cellIdentityTdscdma.mnc);
                cellInfoTdscdma->cellIdentityTdscdma.lac =
                        rillCellInfo->CellInfo.tdscdma.cellIdentityTdscdma.lac;
                cellInfoTdscdma->cellIdentityTdscdma.cid =
                        rillCellInfo->CellInfo.tdscdma.cellIdentityTdscdma.cid;
                cellInfoTdscdma->cellIdentityTdscdma.cpid =
                        rillCellInfo->CellInfo.tdscdma.cellIdentityTdscdma.cpid;
                cellInfoTdscdma->signalStrengthTdscdma.rscp =
                        rillCellInfo->CellInfo.tdscdma.signalStrengthTdscdma.rscp;
                break;
            }
            default: {
                break;
            }
        }
        rillCellInfo += 1;
    }
}

int radio::cellInfoListInd(android::Parcel &p, int slotId, int requestNumber,
                           int indicationType, int token, RIL_Errno e, void *response,
                           size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen % sizeof(RIL_CellInfo_v12) != 0) {
            RLOGE("radio::cellInfoListInd: invalid response");
            return 0;
        }

        hidl_vec<CellInfo> records;
        convertRilCellInfoListToHal(response, responseLen, records);

        RLOGD("radio::cellInfoListInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->cellInfoList(
                convertIntToRadioIndicationType(indicationType), records);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::cellInfoListInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::imsNetworkStateChangedInd(android::Parcel &p, int slotId, int requestNumber,
                                     int indicationType, int token, RIL_Errno e, void *response,
                                     size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::imsNetworkStateChangedInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->imsNetworkStateChanged(
                convertIntToRadioIndicationType(indicationType));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::imsNetworkStateChangedInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::subscriptionStatusChangedInd(android::Parcel &p, int slotId, int requestNumber,
                                        int indicationType, int token, RIL_Errno e, void *response,
                                        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("radio::subscriptionStatusChangedInd: invalid response");
            return 0;
        }
        bool activate = ((int32_t *) response)[0];
        RLOGD("radio::subscriptionStatusChangedInd: activate %d", activate);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->subscriptionStatusChanged(
                convertIntToRadioIndicationType(indicationType), activate);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::subscriptionStatusChangedInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::srvccStateNotifyInd(android::Parcel &p, int slotId, int requestNumber,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("radio::srvccStateNotifyInd: invalid response");
            return 0;
        }
        int32_t state = ((int32_t *) response)[0];
        RLOGD("radio::srvccStateNotifyInd: rat %d", state);
        Return<void> retStatus = radioService[slotId]->mRadioIndication->srvccStateNotify(
                convertIntToRadioIndicationType(indicationType), (SrvccState) state);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::srvccStateNotifyInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

void convertRilHardwareConfigListToHal(void *response, size_t responseLen,
        hidl_vec<HardwareConfig>& records) {
    int num = responseLen / sizeof(RIL_HardwareConfig);
    records.resize(num);

    RIL_HardwareConfig *rilHardwareConfig = (RIL_HardwareConfig *) response;
    for (int i = 0; i < num; i++) {
        records[i].type = (HardwareConfigType) rilHardwareConfig[i].type;
        records[i].uuid = convertCharPtrToHidlString(rilHardwareConfig[i].uuid);
        records[i].state = (HardwareConfigState) rilHardwareConfig[i].state;
        switch (rilHardwareConfig[i].type) {
            case RIL_HARDWARE_CONFIG_MODEM: {
                records[i].modem.resize(1);
                records[i].sim.resize(0);
                HardwareConfigModem *hwConfigModem = &records[i].modem[0];
                hwConfigModem->rat = rilHardwareConfig[i].cfg.modem.rat;
                hwConfigModem->maxVoice = rilHardwareConfig[i].cfg.modem.maxVoice;
                hwConfigModem->maxData = rilHardwareConfig[i].cfg.modem.maxData;
                hwConfigModem->maxStandby = rilHardwareConfig[i].cfg.modem.maxStandby;
                break;
            }

            case RIL_HARDWARE_CONFIG_SIM: {
                records[i].sim.resize(1);
                records[i].modem.resize(0);
                records[i].sim[0].modemUuid =
                        convertCharPtrToHidlString(rilHardwareConfig[i].cfg.sim.modemUuid);
                break;
            }
        }
    }
}

int radio::hardwareConfigChangedInd(android::Parcel &p, int slotId, int requestNumber,
                                    int indicationType, int token, RIL_Errno e, void *response,
                                    size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen % sizeof(RIL_HardwareConfig) != 0) {
            RLOGE("radio::hardwareConfigChangedInd: invalid response");
            return 0;
        }

        hidl_vec<HardwareConfig> configs;
        convertRilHardwareConfigListToHal(response, responseLen, configs);

        RLOGD("radio::hardwareConfigChangedInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->hardwareConfigChanged(
                convertIntToRadioIndicationType(indicationType), configs);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::hardwareConfigChangedInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

void convertRilRadioCapabilityToHal(void *response, size_t responseLen, RadioCapability& rc) {
    RIL_RadioCapability *rilRadioCapability = (RIL_RadioCapability *) response;
    rc.session = rilRadioCapability->session;
    rc.phase = (android::hardware::radio::V1_0::RadioCapabilityPhase) rilRadioCapability->phase;
    rc.raf = rilRadioCapability->rat;
    rc.logicalModemUuid = convertCharPtrToHidlString(rilRadioCapability->logicalModemUuid);
    rc.status = (android::hardware::radio::V1_0::RadioCapabilityStatus) rilRadioCapability->status;
}

int radio::radioCapabilityIndicationInd(android::Parcel &p, int slotId, int requestNumber,
                                        int indicationType, int token, RIL_Errno e, void *response,
                                        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_RadioCapability)) {
            RLOGE("radio::radioCapabilityIndicationInd: invalid response");
            return 0;
        }

        RadioCapability rc = {};
        convertRilRadioCapabilityToHal(response, responseLen, rc);

        RLOGD("radio::radioCapabilityIndicationInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->radioCapabilityIndication(
                convertIntToRadioIndicationType(indicationType), rc);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::radioCapabilityIndicationInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

bool isServiceTypeCfQuery(RIL_SsServiceType serType, RIL_SsRequestType reqType) {
    if ((reqType == SS_INTERROGATION) &&
        (serType == SS_CFU ||
         serType == SS_CF_BUSY ||
         serType == SS_CF_NO_REPLY ||
         serType == SS_CF_NOT_REACHABLE ||
         serType == SS_CF_ALL ||
         serType == SS_CF_ALL_CONDITIONAL)) {
        return true;
    }
    return false;
}

int radio::onSupplementaryServiceIndicationInd(android::Parcel &p, int slotId, int requestNumber,
                                               int indicationType, int token, RIL_Errno e,
                                               void *response, size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_StkCcUnsolSsResponse)) {
            RLOGE("radio::onSupplementaryServiceIndicationInd: invalid response");
            return 0;
        }

        RIL_StkCcUnsolSsResponse *rilSsResponse = (RIL_StkCcUnsolSsResponse *) response;
        StkCcUnsolSsResult ss = {};
        ss.serviceType = (SsServiceType) rilSsResponse->serviceType;
        ss.requestType = (SsRequestType) rilSsResponse->requestType;
        ss.teleserviceType = (SsTeleserviceType) rilSsResponse->teleserviceType;
        ss.serviceClass = rilSsResponse->serviceClass;
        ss.result = (RadioError) rilSsResponse->result;

        if (isServiceTypeCfQuery(rilSsResponse->serviceType, rilSsResponse->requestType)) {
            RLOGD("radio::onSupplementaryServiceIndicationInd CF type, num of Cf elements %d",
                    rilSsResponse->cfData.numValidIndexes);
            if (rilSsResponse->cfData.numValidIndexes > NUM_SERVICE_CLASSES) {
                RLOGE("radio::onSupplementaryServiceIndicationInd numValidIndexes is greater than "
                        "max value %d, truncating it to max value", NUM_SERVICE_CLASSES);
                rilSsResponse->cfData.numValidIndexes = NUM_SERVICE_CLASSES;
            }

            ss.cfData.resize(1);
            ss.ssInfo.resize(0);

            /* number of call info's */
            ss.cfData[0].cfInfo.resize(rilSsResponse->cfData.numValidIndexes);

            for (int i = 0; i < rilSsResponse->cfData.numValidIndexes; i++) {
                 RIL_CallForwardInfo cf = rilSsResponse->cfData.cfInfo[i];
                 CallForwardInfo *cfInfo = &ss.cfData[0].cfInfo[i];

                 cfInfo->status = (CallForwardInfoStatus) cf.status;
                 cfInfo->reason = cf.reason;
                 cfInfo->serviceClass = cf.serviceClass;
                 cfInfo->toa = cf.toa;
                 cfInfo->number = convertCharPtrToHidlString(cf.number);
                 cfInfo->timeSeconds = cf.timeSeconds;
                 RLOGD("radio::onSupplementaryServiceIndicationInd: "
                        "Data: %d,reason=%d,cls=%d,toa=%d,num=%s,tout=%d],", cf.status,
                        cf.reason, cf.serviceClass, cf.toa, (char*)cf.number, cf.timeSeconds);
            }
        } else {
            ss.ssInfo.resize(1);
            ss.cfData.resize(0);

            /* each int */
            ss.ssInfo[0].ssInfo.resize(SS_INFO_MAX);
            for (int i = 0; i < SS_INFO_MAX; i++) {
                 RLOGD("radio::onSupplementaryServiceIndicationInd: Data: %d",
                        rilSsResponse->ssInfo[i]);
                 ss.ssInfo[0].ssInfo[i] = rilSsResponse->ssInfo[i];
            }
        }

        RLOGD("radio::onSupplementaryServiceIndicationInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->
                onSupplementaryServiceIndication(convertIntToRadioIndicationType(indicationType),
                ss);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::onSupplementaryServiceIndicationInd: "
                "radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::stkCallControlAlphaNotifyInd(android::Parcel &p, int slotId, int requestNumber,
                                        int indicationType, int token, RIL_Errno e, void *response,
                                        size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("radio::stkCallControlAlphaNotifyInd: invalid response");
            return 0;
        }
        RLOGD("radio::stkCallControlAlphaNotifyInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->stkCallControlAlphaNotify(
                convertIntToRadioIndicationType(indicationType),
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::stkCallControlAlphaNotifyInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

void convertRilLceDataInfoToHal(void *response, size_t responseLen, LceDataInfo& lce) {
    RIL_LceDataInfo *rilLceDataInfo = (RIL_LceDataInfo *)response;
    lce.lastHopCapacityKbps = rilLceDataInfo->last_hop_capacity_kbps;
    lce.confidenceLevel = rilLceDataInfo->confidence_level;
    lce.lceSuspended = rilLceDataInfo->lce_suspended;
}

int radio::lceDataInd(android::Parcel &p, int slotId, int requestNumber,
                      int indicationType, int token, RIL_Errno e, void *response,
                      size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_LceDataInfo)) {
            RLOGE("radio::lceDataInd: invalid response");
            return 0;
        }

        LceDataInfo lce = {};
        convertRilLceDataInfoToHal(response, responseLen, lce);
        RLOGD("radio::lceDataInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->lceData(
                convertIntToRadioIndicationType(indicationType), lce);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::lceDataInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::pcoDataInd(android::Parcel &p, int slotId, int requestNumber,
                      int indicationType, int token, RIL_Errno e, void *response,
                      size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_PCO_Data)) {
            RLOGE("radio::pcoDataInd: invalid response");
            return 0;
        }

        PcoDataInfo pco = {};
        RIL_PCO_Data *rilPcoData = (RIL_PCO_Data *)response;
        pco.cid = rilPcoData->cid;
        pco.bearerProto = convertCharPtrToHidlString(rilPcoData->bearer_proto);
        pco.pcoId = rilPcoData->pco_id;
        pco.contents.setToExternal((uint8_t *) rilPcoData->contents, rilPcoData->contents_length);

        RLOGD("radio::pcoDataInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->pcoData(
                convertIntToRadioIndicationType(indicationType), pco);
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::pcoDataInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::modemResetInd(android::Parcel &p, int slotId, int requestNumber,
                         int indicationType, int token, RIL_Errno e, void *response,
                         size_t responseLen) {
    if (radioService[slotId] != NULL && radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("radio::modemResetInd: invalid response");
            return 0;
        }
        RLOGD("radio::modemResetInd");
        Return<void> retStatus = radioService[slotId]->mRadioIndication->modemReset(
                convertIntToRadioIndicationType(indicationType),
                convertCharPtrToHidlString((char *) response));
        radioService[slotId]->checkReturnStatus(retStatus);
    } else {
        RLOGE("radio::modemResetInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::oemHookRawInd(android::Parcel &p, int slotId, int requestNumber,
                         int indicationType, int token, RIL_Errno e, void *response,
                         size_t responseLen) {
    if (oemHookService[slotId] != NULL && oemHookService[slotId]->mOemHookIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("radio::oemHookRawInd: invalid response");
            return 0;
        }

        hidl_vec<uint8_t> data;
        data.setToExternal((uint8_t *) response, responseLen);
        RLOGD("radio::oemHookRawInd");
        Return<void> retStatus = oemHookService[slotId]->mOemHookIndication->oemHookRaw(
                convertIntToRadioIndicationType(indicationType), data);
        checkReturnStatus(slotId, retStatus);
    } else {
        RLOGE("radio::oemHookRawInd: oemHookService[%d]->mOemHookIndication == NULL", slotId);
    }

    return 0;
}

void radio::registerService(RIL_RadioFunctions *callbacks, CommandInfo *commands) {
    using namespace android::hardware;
    int simCount = 1;
    const char *serviceNames[] = {
            android::RIL_getRilSocketName()
            #if (SIM_COUNT >= 2)
            , SOCKET2_NAME_RIL
            #if (SIM_COUNT >= 3)
            , SOCKET3_NAME_RIL
            #if (SIM_COUNT >= 4)
            , SOCKET4_NAME_RIL
            #endif
            #endif
            #endif
            };

    const char *oemHookServiceNames[] = {
            OEM_HOOK_SERVICE_NAME
            #if (SIM_COUNT >= 2)
            , OEM_HOOK2_SERVICE_NAME
            #if (SIM_COUNT >= 3)
            , OEM_HOOK3_SERVICE_NAME
            #if (SIM_COUNT >= 4)
            , OEM_HOOK4_SERVICE_NAME
            #endif
            #endif
            #endif
            };

    #if (SIM_COUNT >= 2)
    simCount = SIM_COUNT;
    #endif

    configureRpcThreadpool(1, true /* callerWillJoin */);
    for (int i = 0; i < simCount; i++) {
        pthread_rwlock_t *radioServiceRwlockPtr = getRadioServiceRwlock(i);
        int ret = pthread_rwlock_wrlock(radioServiceRwlockPtr);
        assert(ret == 0);

        radioService[i] = new RadioImpl;
        radioService[i]->mSlotId = i;
        oemHookService[i] = new OemHookImpl;
        oemHookService[i]->mSlotId = i;
        RLOGD("radio::registerService: starting IRadio %s", serviceNames[i]);
        android::status_t status = radioService[i]->registerAsService(serviceNames[i]);
        status = oemHookService[i]->registerAsService(oemHookServiceNames[i]);

        ret = pthread_rwlock_unlock(radioServiceRwlockPtr);
        assert(ret == 0);
    }

    s_vendorFunctions = callbacks;
    s_commands = commands;
}

void rilc_thread_pool() {
    joinRpcThreadpool();
}

pthread_rwlock_t * radio::getRadioServiceRwlock(int slotId) {
    pthread_rwlock_t *radioServiceRwlockPtr = &radioServiceRwlock;

    #if (SIM_COUNT >= 2)
    if (slotId == 2) radioServiceRwlockPtr = &radioServiceRwlock2;
    #if (SIM_COUNT >= 3)
    if (slotId == 3) radioServiceRwlockPtr = &radioServiceRwlock3;
    #if (SIM_COUNT >= 4)
    if (slotId == 4) radioServiceRwlockPtr = &radioServiceRwlock4;
    #endif
    #endif
    #endif

    return radioServiceRwlockPtr;
}
