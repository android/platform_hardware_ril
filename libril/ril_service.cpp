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

#include <android/hardware/radio/1.0/IRadio.h>

#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <ril_service.h>
#include <hidl/HidlTransportSupport.h>
#include <utils/SystemClock.h>

#define INVALID_HEX_CHAR 16

using namespace android::hardware::radio::V1_0;
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

RIL_RadioFunctions *s_callbacks = NULL;
static CommandInfo *s_commands;

struct RadioImpl;

#if (SIM_COUNT >= 2)
sp<RadioImpl> radioService[SIM_COUNT];
#else
sp<RadioImpl> radioService[1];
#endif

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
            int32_t radioTechnology,
            int32_t profile,
            const ::android::hardware::hidl_string& apn,
            const ::android::hardware::hidl_string& user,
            const ::android::hardware::hidl_string& password,
            ApnAuthType authType,
            const ::android::hardware::hidl_string& protocol);

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

    Return<void> sendOemRadioRequestRaw(int32_t serial,
            const ::android::hardware::hidl_vec<uint8_t>& data);

    Return<void> sendOemRadioRequestStrings(int32_t serial,
            const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& data);

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

    Return<void> setInitialAttachApn(int32_t serial,
            const ::android::hardware::hidl_string& apn,
            const ::android::hardware::hidl_string& protocol,
            ApnAuthType authType,
            const ::android::hardware::hidl_string& username,
            const ::android::hardware::hidl_string& password);

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
            const ::android::hardware::hidl_vec<DataProfileInfo>& profiles);

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

    Return<void> responseAcknowledgement();
};

void dispatchStrings(RequestInfo *pRI, int countStrings, ...) {
    char **pStrings;
    android::Parcel p;   // TODO: should delete this after translation of all commands is complete
    pStrings = (char **)calloc(countStrings, sizeof(char *));
    if (pStrings == NULL) {
        RLOGE("Memory allocation failed for request %s",
            requestToString(pRI->pCI->requestNumber));
        pRI->pCI->responseFunction(p, (int) pRI->socket_id, pRI->pCI->requestNumber,
                (int) RadioResponseType::SOLICITED, pRI->token, RIL_E_NO_MEMORY, NULL, 0);
        return;
    }
    va_list ap;
    va_start(ap, countStrings);
    for (int i = 0; i < countStrings; i++) {
        const char* str = va_arg(ap, const char *);
        int len = strlen(str);
        pStrings[i] = (char *) calloc(len + 1, sizeof(char));
        if (pStrings[i] == NULL) {
            RLOGE("Memory allocation failed for request %s",
                    requestToString(pRI->pCI->requestNumber));
            va_end(ap);
            pRI->pCI->responseFunction(p, (int) pRI->socket_id, pRI->pCI->requestNumber,
                    (int) RadioResponseType::SOLICITED, pRI->token, RIL_E_NO_MEMORY, NULL, 0);
            for (int j = 0; j < i; j++) {
#ifdef MEMSET_FREED
                memsetString (pStrings[j]);
#endif
                free(pStrings[j]);
            }
            free(pStrings);
            return;
        }
        strncpy(pStrings[i], str, len + 1);
    }
    va_end(ap);

    s_callbacks->onRequest(pRI->pCI->requestNumber, pStrings, countStrings * sizeof(char *), pRI);

    if (pStrings != NULL) {
        for (int i = 0 ; i < countStrings ; i++) {
#ifdef MEMSET_FREED
            memsetString (pStrings[i]);
#endif
            free(pStrings[i]);
        }

#ifdef MEMSET_FREED
        memset(pStrings, 0, countStrings * sizeof(char *));
#endif
        free(pStrings);
    }
}

Return<void> RadioImpl::setResponseFunctions(
        const ::android::sp<IRadioResponse>& radioResponseParam,
        const ::android::sp<IRadioIndication>& radioIndicationParam) {
    RLOGD("RadioImpl::setResponseFunctions");
    mRadioResponse = radioResponseParam;
    mRadioIndication = radioIndicationParam;
    return Status::ok();
}

Return<void> RadioImpl::getIccCardStatus(int32_t serial) {
    RLOGD("RadioImpl::getIccCardStatus: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_GET_SIM_STATUS);
    if (pRI == NULL) {
        return Void();
    }
    s_callbacks->onRequest(RIL_REQUEST_GET_SIM_STATUS, NULL, 0, pRI);

    return Status::ok();
}

Return<void> RadioImpl::supplyIccPinForApp(int32_t serial, const hidl_string& pin,
        const hidl_string& aid) {
    RLOGD("RadioImpl::supplyIccPinForApp: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_ENTER_SIM_PIN);
    if (pRI == NULL) {
        return Void();
    }

    dispatchStrings(pRI, 2, (const char *)pin, (const char *)aid);
    return Status::ok();
}

Return<void> RadioImpl::supplyIccPukForApp(int32_t serial, const hidl_string& puk,
        const hidl_string& pin, const hidl_string& aid) {
    RLOGD("RadioImpl::supplyIccPukForApp: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_ENTER_SIM_PUK);
    if (pRI == NULL) {
        return Void();
    }

    dispatchStrings(pRI, 3, (const char *)puk, (const char *)pin, (const char *)aid);
    return Status::ok();
}

Return<void> RadioImpl::supplyIccPin2ForApp(int32_t serial, const hidl_string& pin2,
        const hidl_string& aid) {
    RLOGD("RadioImpl::supplyIccPin2ForApp: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_ENTER_SIM_PIN2);
    if (pRI == NULL) {
        return Void();
    }

    dispatchStrings(pRI, 2, (const char *)pin2, (const char *)aid);
    return Status::ok();
}

Return<void> RadioImpl::supplyIccPuk2ForApp(int32_t serial, const hidl_string& puk2,
        const hidl_string& pin2, const hidl_string& aid) {
    RLOGD("RadioImpl::supplyIccPuk2ForApp: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_ENTER_SIM_PUK2);
    if (pRI == NULL) {
        return Void();
    }

    dispatchStrings(pRI, 3, (const char *)puk2, (const char *)pin2, (const char *)aid);
    return Status::ok();
}

Return<void> RadioImpl::changeIccPinForApp(int32_t serial, const hidl_string& oldPin,
        const hidl_string& newPin, const hidl_string& aid) {
    RLOGD("RadioImpl::changeIccPinForApp: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_CHANGE_SIM_PIN);
    if (pRI == NULL) {
        return Void();
    }

    dispatchStrings(pRI, 3, (const char *)oldPin, (const char *)newPin,
            (const char *)aid);
    return Status::ok();
}

Return<void> RadioImpl::changeIccPin2ForApp(int32_t serial, const hidl_string& oldPin2,
        const hidl_string& newPin2, const hidl_string& aid) {
    RLOGD("RadioImpl::changeIccPin2ForApp: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_CHANGE_SIM_PIN2);
    if (pRI == NULL) {
        return Void();
    }

    dispatchStrings(pRI, 3, (const char *)oldPin2, (const char *)newPin2,
            (const char *)aid);
    return Status::ok();
}

Return<void> RadioImpl::supplyNetworkDepersonalization(int32_t serial,
        const hidl_string& netPin) {
    RLOGD("RadioImpl::supplyNetworkDepersonalization: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId,
            RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION);
    if (pRI == NULL) {
        return Void();
    }

    dispatchStrings(pRI, 1, (const char *)netPin);
    return Status::ok();
}

Return<void> RadioImpl::getCurrentCalls(int32_t serial) {
    RLOGD("RadioImpl::getCurrentCalls: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_GET_CURRENT_CALLS);
    if (pRI == NULL) {
        return Void();
    }

    s_callbacks->onRequest(RIL_REQUEST_GET_CURRENT_CALLS, NULL, 0, pRI);
    return Status::ok();
}

Return<void> RadioImpl::dial(int32_t serial, const Dial& dialInfo) {
    RLOGD("RadioImpl::dial: serial %d", serial);
    RequestInfo *pRI = android::addRequestToList(serial, mSlotId, RIL_REQUEST_DIAL);
    if (pRI == NULL) {
        return Void();
    }
    RIL_Dial dial;
    RIL_UUS_Info uusInfo;
    int32_t sizeOfDial = sizeof(dial);

    memset (&dial, 0, sizeOfDial);

    dial.address = (char *) calloc(dialInfo.address.size() + 1, sizeof(char));
    if (dial.address == NULL) {
        android::Parcel p;
        RLOGE("Memory allocation failed for request %s", requestToString(pRI->pCI->requestNumber));
        pRI->pCI->responseFunction(p, (int) pRI->socket_id, pRI->pCI->requestNumber,
                (int) RadioResponseType::SOLICITED, pRI->token, RIL_E_NO_MEMORY, NULL, 0);
        return Void();
    }
    strcpy(dial.address, dialInfo.address.c_str());
    dial.clir = (int)dialInfo.clir;

    memset(&uusInfo, 0, sizeof(RIL_UUS_Info));
    if (dialInfo.uusInfo.size() != 0) {
        int32_t len;

        uusInfo.uusType = (RIL_UUS_Type) dialInfo.uusInfo[0].uusType;
        uusInfo.uusDcs = (RIL_UUS_DCS) dialInfo.uusInfo[0].uusDcs;

        if (dialInfo.uusInfo[0].uusData.size() == 0) {
            uusInfo.uusData = NULL;
            len = 0;
        } else {
            len = dialInfo.uusInfo[0].uusData.size();
            uusInfo.uusData = (char*) calloc(len + 1, sizeof(char));
            // check if the length is invalid
            if (uusInfo.uusData == NULL) {
                RLOGE("Memory allocation failed for request %s",
                        requestToString(pRI->pCI->requestNumber));
                android::Parcel p;
                pRI->pCI->responseFunction(p, (int) pRI->socket_id, pRI->pCI->requestNumber,
                        (int) RadioResponseType::SOLICITED, pRI->token, RIL_E_NO_MEMORY, NULL, 0);
                free(dial.address);
                return Void();
            }
            strcpy(uusInfo.uusData, dialInfo.uusInfo[0].uusData.c_str());
        }

        uusInfo.uusLength = len;
        dial.uusInfo = &uusInfo;
    }

    s_callbacks->onRequest(RIL_REQUEST_DIAL, &dial, sizeOfDial, pRI);

#ifdef MEMSET_FREED
    memsetString (dial.address);
#endif
    free (dial.address);

#ifdef MEMSET_FREED
    memsetString (uusInfo.uusData);
#endif
    if (uusInfo.uusData != NULL) {
        free(uusInfo.uusData);
    }

    return Status::ok();
}

Return<void> RadioImpl::getImsiForApp(int32_t serial,
        const hidl_string& aid) {return Status::ok();}

Return<void> RadioImpl::hangup(int32_t serial, int32_t gsmIndex) {return Status::ok();}

Return<void> RadioImpl::hangupWaitingOrBackground(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::hangupForegroundResumeBackground(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::switchWaitingOrHoldingAndActive(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::conference(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::rejectCall(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getLastCallFailCause(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getSignalStrength(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getVoiceRegistrationState(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getDataRegistrationState(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getOperator(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setRadioPower(int32_t serial, bool on) {return Status::ok();}

Return<void> RadioImpl::sendDtmf(int32_t serial,
        const ::android::hardware::hidl_string& s) {return Status::ok();}

Return<void> RadioImpl::sendSms(int32_t serial, const GsmSmsMessage& message) {return Status::ok();}

Return<void> RadioImpl::sendSMSExpectMore(int32_t serial, const GsmSmsMessage& message) {return Status::ok();}

Return<void> RadioImpl::setupDataCall(int32_t serial,
        int32_t radioTechnology,
        int32_t profile,
        const ::android::hardware::hidl_string& apn,
        const ::android::hardware::hidl_string& user,
        const ::android::hardware::hidl_string& password,
        ApnAuthType authType,
        const ::android::hardware::hidl_string& protocol) {return Status::ok();}

Return<void> RadioImpl::iccIOForApp(int32_t serial,
        const IccIo& iccIo) {return Status::ok();}

Return<void> RadioImpl::sendUssd(int32_t serial,
        const ::android::hardware::hidl_string& ussd) {return Status::ok();}

Return<void> RadioImpl::cancelPendingUssd(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getClir(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setClir(int32_t serial, int32_t status) {return Status::ok();}

Return<void> RadioImpl::getCallForwardStatus(int32_t serial,
        const CallForwardInfo& callInfo) {return Status::ok();}

Return<void> RadioImpl::setCallForward(int32_t serial,
        const CallForwardInfo& callInfo) {return Status::ok();}

Return<void> RadioImpl::getCallWaiting(int32_t serial, int32_t serviceClass) {return Status::ok();}

Return<void> RadioImpl::setCallWaiting(int32_t serial, bool enable, int32_t serviceClass) {return Status::ok();}

Return<void> RadioImpl::acknowledgeLastIncomingGsmSms(int32_t serial,
        bool success, SmsAcknowledgeFailCause cause) {return Status::ok();}

Return<void> RadioImpl::acceptCall(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::deactivateDataCall(int32_t serial,
        int32_t cid, bool reasonRadioShutDown) {return Status::ok();}

Return<void> RadioImpl::getFacilityLockForApp(int32_t serial,
        const ::android::hardware::hidl_string& facility,
        const ::android::hardware::hidl_string& password,
        int32_t serviceClass,
        const ::android::hardware::hidl_string& appId) {return Status::ok();}

Return<void> RadioImpl::setFacilityLockForApp(int32_t serial,
        const ::android::hardware::hidl_string& facility,
        bool lockState,
        const ::android::hardware::hidl_string& password,
        int32_t serviceClass,
        const ::android::hardware::hidl_string& appId) {return Status::ok();}

Return<void> RadioImpl::setBarringPassword(int32_t serial,
        const ::android::hardware::hidl_string& facility,
        const ::android::hardware::hidl_string& oldPassword,
        const ::android::hardware::hidl_string& newPassword) {return Status::ok();}

Return<void> RadioImpl::getNetworkSelectionMode(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setNetworkSelectionModeAutomatic(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setNetworkSelectionModeManual(int32_t serial,
        const ::android::hardware::hidl_string& operatorNumeric) {return Status::ok();}

Return<void> RadioImpl::getAvailableNetworks(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::startDtmf(int32_t serial,
        const ::android::hardware::hidl_string& s) {return Status::ok();}

Return<void> RadioImpl::stopDtmf(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getBasebandVersion(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::separateConnection(int32_t serial, int32_t gsmIndex) {return Status::ok();}

Return<void> RadioImpl::setMute(int32_t serial, bool enable) {return Status::ok();}

Return<void> RadioImpl::getMute(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getClip(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getDataCallList(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::sendOemRadioRequestRaw(int32_t serial,
        const ::android::hardware::hidl_vec<uint8_t>& data) {return Status::ok();}

Return<void> RadioImpl::sendOemRadioRequestStrings(int32_t serial,
        const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& data) {return Status::ok();}

Return<void> RadioImpl::sendScreenState(int32_t serial, bool enable) {return Status::ok();}

Return<void> RadioImpl::setSuppServiceNotifications(int32_t serial, bool enable) {return Status::ok();}

Return<void> RadioImpl::writeSmsToSim(int32_t serial,
        const SmsWriteArgs& smsWriteArgs) {return Status::ok();}

Return<void> RadioImpl::deleteSmsOnSim(int32_t serial, int32_t index) {return Status::ok();}

Return<void> RadioImpl::setBandMode(int32_t serial, RadioBandMode mode) {return Status::ok();}

Return<void> RadioImpl::getAvailableBandModes(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::sendEnvelope(int32_t serial,
        const ::android::hardware::hidl_string& command) {return Status::ok();}

Return<void> RadioImpl::sendTerminalResponseToSim(int32_t serial,
        const ::android::hardware::hidl_string& commandResponse) {return Status::ok();}

Return<void> RadioImpl::handleStkCallSetupRequestFromSim(int32_t serial, bool accept) {return Status::ok();}

Return<void> RadioImpl::explicitCallTransfer(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setPreferredNetworkType(int32_t serial, PreferredNetworkType nwType) {return Status::ok();}

Return<void> RadioImpl::getPreferredNetworkType(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getNeighboringCids(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setLocationUpdates(int32_t serial, bool enable) {return Status::ok();}

Return<void> RadioImpl::setCdmaSubscriptionSource(int32_t serial, CdmaSubscriptionSource cdmaSub) {return Status::ok();}

Return<void> RadioImpl::setCdmaRoamingPreference(int32_t serial, CdmaRoamingType type) {return Status::ok();}

Return<void> RadioImpl::getCdmaRoamingPreference(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setTTYMode(int32_t serial, TtyMode mode) {return Status::ok();}

Return<void> RadioImpl::getTTYMode(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setPreferredVoicePrivacy(int32_t serial, bool enable) {return Status::ok();}

Return<void> RadioImpl::getPreferredVoicePrivacy(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::sendCDMAFeatureCode(int32_t serial,
        const ::android::hardware::hidl_string& featureCode) {return Status::ok();}

Return<void> RadioImpl::sendBurstDtmf(int32_t serial,
        const ::android::hardware::hidl_string& dtmf,
        int32_t on,
        int32_t off) {return Status::ok();}

Return<void> RadioImpl::sendCdmaSms(int32_t serial, const CdmaSmsMessage& sms) {return Status::ok();}

Return<void> RadioImpl::acknowledgeLastIncomingCdmaSms(int32_t serial, const CdmaSmsAck& smsAck) {return Status::ok();}

Return<void> RadioImpl::getGsmBroadcastConfig(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setGsmBroadcastConfig(int32_t serial,
        const hidl_vec<GsmBroadcastSmsConfigInfo>& configInfo) {return Status::ok();}

Return<void> RadioImpl::setGsmBroadcastActivation(int32_t serial, bool activate) {return Status::ok();}

Return<void> RadioImpl::getCdmaBroadcastConfig(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setCdmaBroadcastConfig(int32_t serial,
        const hidl_vec<CdmaBroadcastSmsConfigInfo>& configInfo) {return Status::ok();}

Return<void> RadioImpl::setCdmaBroadcastActivation(int32_t serial, bool activate) {return Status::ok();}

Return<void> RadioImpl::getCDMASubscription(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::writeSmsToRuim(int32_t serial, const CdmaSmsWriteArgs& cdmaSms) {return Status::ok();}

Return<void> RadioImpl::deleteSmsOnRuim(int32_t serial, int32_t index) {return Status::ok();}

Return<void> RadioImpl::getDeviceIdentity(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::exitEmergencyCallbackMode(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getSmscAddress(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setSmscAddress(int32_t serial,
        const ::android::hardware::hidl_string& smsc) {return Status::ok();}

Return<void> RadioImpl::reportSmsMemoryStatus(int32_t serial, bool available) {return Status::ok();}

Return<void> RadioImpl::reportStkServiceIsRunning(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getCdmaSubscriptionSource(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::requestIsimAuthentication(int32_t serial,
        const ::android::hardware::hidl_string& challenge) {return Status::ok();}

Return<void> RadioImpl::acknowledgeIncomingGsmSmsWithPdu(int32_t serial,
        bool success,
        const ::android::hardware::hidl_string& ackPdu) {return Status::ok();}

Return<void> RadioImpl::sendEnvelopeWithStatus(int32_t serial,
        const ::android::hardware::hidl_string& contents) {return Status::ok();}

Return<void> RadioImpl::getVoiceRadioTechnology(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getCellInfoList(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setCellInfoListRate(int32_t serial, int32_t rate) {return Status::ok();}

Return<void> RadioImpl::setInitialAttachApn(int32_t serial,
        const ::android::hardware::hidl_string& apn,
        const ::android::hardware::hidl_string& protocol,
        ApnAuthType authType,
        const ::android::hardware::hidl_string& username,
        const ::android::hardware::hidl_string& password) {return Status::ok();}

Return<void> RadioImpl::getImsRegistrationState(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::sendImsSms(int32_t serial, const ImsSmsMessage& message) {return Status::ok();}

Return<void> RadioImpl::iccTransmitApduBasicChannel(int32_t serial, const SimApdu& message) {return Status::ok();}

Return<void> RadioImpl::iccOpenLogicalChannel(int32_t serial,
        const ::android::hardware::hidl_string& aid) {return Status::ok();}

Return<void> RadioImpl::iccCloseLogicalChannel(int32_t serial, int32_t channelId) {return Status::ok();}

Return<void> RadioImpl::iccTransmitApduLogicalChannel(int32_t serial, const SimApdu& message) {return Status::ok();}

Return<void> RadioImpl::nvReadItem(int32_t serial, NvItem itemId) {return Status::ok();}

Return<void> RadioImpl::nvWriteItem(int32_t serial, const NvWriteItem& item) {return Status::ok();}

Return<void> RadioImpl::nvWriteCdmaPrl(int32_t serial,
        const ::android::hardware::hidl_vec<uint8_t>& prl) {return Status::ok();}

Return<void> RadioImpl::nvResetConfig(int32_t serial, ResetNvType resetType) {return Status::ok();}

Return<void> RadioImpl::setUiccSubscription(int32_t serial, const SelectUiccSub& uiccSub) {return Status::ok();}

Return<void> RadioImpl::setDataAllowed(int32_t serial, bool allow) {return Status::ok();}

Return<void> RadioImpl::getHardwareConfig(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::requestIccSimAuthentication(int32_t serial,
        int32_t authContext,
        const ::android::hardware::hidl_string& authData,
        const ::android::hardware::hidl_string& aid) {return Status::ok();}

Return<void> RadioImpl::setDataProfile(int32_t serial,
        const ::android::hardware::hidl_vec<DataProfileInfo>& profiles) {return Status::ok();}

Return<void> RadioImpl::requestShutdown(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getRadioCapability(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setRadioCapability(int32_t serial, const RadioCapability& rc) {return Status::ok();}

Return<void> RadioImpl::startLceService(int32_t serial, int32_t reportInterval, bool pullMode) {return Status::ok();}

Return<void> RadioImpl::stopLceService(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::pullLceData(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getModemActivityInfo(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setAllowedCarriers(int32_t serial,
        bool allAllowed,
        const CarrierRestrictions& carriers) {return Status::ok();}

Return<void> RadioImpl::getAllowedCarriers(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::responseAcknowledgement() {
    android::releaseWakeLock();
    return Status::ok();
}

hidl_string convertCharPtrToHidlString(char *ptr) {
    hidl_string ret;
    if (ptr != NULL) {
        ret.setToExternal(ptr, strlen(ptr));
    }
    return ret;
}

void radio::acknowledgeRequest(int slotId, int serial) {
    if (radioService[slotId]->mRadioResponse != NULL) {
        radioService[slotId]->mRadioResponse->acknowledgeRequest(serial);
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
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("radio::getIccCardStatusResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo;
        populateResponseInfo(responseInfo, serial, responseType, e);
        CardStatus cardStatus;
        if (response == NULL || responseLen != sizeof(RIL_CardStatus_v6)) {
            RLOGE("radio::getIccCardStatusResponse: Invalid response");
            if (e == RIL_E_SUCCESS) responseInfo.error = RadioError::INVALID_RESPONSE;
            memset(&cardStatus, 0, sizeof(cardStatus));
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

        radioService[slotId]->mRadioResponse->getIccCardStatusResponse(responseInfo, cardStatus);
    } else {
        RLOGE("radio::getIccCardStatusResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::supplyIccPinForAppResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("radio::supplyIccPinForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo;
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        radioService[slotId]->mRadioResponse->supplyIccPinForAppResponse(responseInfo, ret);
    } else {
        RLOGE("radio::supplyIccPinForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::supplyIccPukForAppResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("radio::supplyIccPukForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo;
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        radioService[slotId]->mRadioResponse->supplyIccPukForAppResponse(responseInfo, ret);
    } else {
        RLOGE("radio::supplyIccPukForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::supplyIccPin2ForAppResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("radio::supplyIccPin2ForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo;
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        radioService[slotId]->mRadioResponse->supplyIccPin2ForAppResponse(responseInfo, ret);
    } else {
        RLOGE("radio::supplyIccPin2ForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::supplyIccPuk2ForAppResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("radio::supplyIccPuk2ForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo;
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        radioService[slotId]->mRadioResponse->supplyIccPuk2ForAppResponse(responseInfo, ret);
    } else {
        RLOGE("radio::supplyIccPuk2ForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::changeIccPinForAppResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("radio::changeIccPinForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo;
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        radioService[slotId]->mRadioResponse->changeIccPinForAppResponse(responseInfo, ret);
    } else {
        RLOGE("radio::changeIccPinForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::changeIccPin2ForAppResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("radio::changeIccPin2ForAppResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo;
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        radioService[slotId]->mRadioResponse->changeIccPin2ForAppResponse(responseInfo, ret);
    } else {
        RLOGE("radio::changeIccPin2ForAppResponse: radioService[%d]->mRadioResponse == NULL",
                slotId);
    }

    return 0;
}

int radio::supplyNetworkDepersonalizationResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("radio::supplyNetworkDepersonalizationResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo;
        int ret = responseInt(responseInfo, serial, responseType, e, response, responseLen);
        radioService[slotId]->mRadioResponse->supplyNetworkDepersonalizationResponse(responseInfo,
                ret);
    } else {
        RLOGE("radio::supplyNetworkDepersonalizationResponse: radioService[%d]->mRadioResponse == \
                NULL", slotId);
    }

    return 0;
}

int radio::getCurrentCallsResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("radio::getCurrentCallsResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo;
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
                if (!(p_cur->uusInfo == NULL || p_cur->uusInfo->uusData == NULL)) {
                    RIL_UUS_Info *uusInfo = p_cur->uusInfo;
                    calls[i].uusInfo[0].uusType = (UusType) uusInfo->uusType;
                    calls[i].uusInfo[0].uusDcs = (UusDcs) uusInfo->uusDcs;
                    // convert uusInfo->uusData to a null-terminated string
                    if (uusInfo->uusData != NULL) {
                        char *nullTermStr = strndup(uusInfo->uusData, uusInfo->uusLength);
                        calls[i].uusInfo[0].uusData = nullTermStr;
                        free(nullTermStr);
                    } else {
                        hidl_string emptyString;
                        calls[i].uusInfo[0].uusData = emptyString;
                    }
                }
            }
        }

        radioService[slotId]->mRadioResponse->getCurrentCallsResponse(responseInfo, calls);
    } else {
        RLOGE("radio::getCurrentCallsResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

int radio::dialResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responseLen) {
    RLOGD("radio::dialResponse: serial %d", serial);

    if (radioService[slotId]->mRadioResponse != NULL) {
        RadioResponseInfo responseInfo;
        populateResponseInfo(responseInfo, serial, responseType, e);
        radioService[slotId]->mRadioResponse->dialResponse(responseInfo);
    } else {
        RLOGE("radio::dialResponse: radioService[%d]->mRadioResponse == NULL", slotId);
    }

    return 0;
}

RadioIndicationType convertIntToRadioIndicationType(int indicationType) {
    return indicationType == RESPONSE_UNSOLICITED ? (RadioIndicationType::UNSOLICITED) :
            (RadioIndicationType::UNSOLICITED_ACK_EXP);
}

void radio::radioStateChangedInd(int slotId, int indicationType, RIL_RadioState radioState) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::radioStateChangedInd: radioState %d", radioState);
        radioService[slotId]->mRadioIndication->radioStateChanged(
                convertIntToRadioIndicationType(indicationType), (RadioState) radioState);
    } else {
        RLOGE("radio::radioStateChangedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }
}

int radio::callStateChangedInd(android::Parcel &p, int slotId, int requestNumber,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::callStateChangedInd");
        radioService[slotId]->mRadioIndication->callStateChanged(
                convertIntToRadioIndicationType(indicationType));
    } else {
        RLOGE("radio::callStateChangedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::voiceNetworkStateChangedInd(android::Parcel &p, int slotId, int requestNumber,
                                       int indicationType, int token, RIL_Errno e, void *response,
                                       size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::voiceNetworkStateChangedInd");
        radioService[slotId]->mRadioIndication->voiceNetworkStateChanged(
                convertIntToRadioIndicationType(indicationType));
    } else {
        RLOGE("radio::voiceNetworkStateChangedInd: radioService[%d]->mRadioIndication == NULL", slotId);
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

    for (int i = 0; i < responseLen; i += 2) {
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
    if (radioService[slotId]->mRadioIndication != NULL) {
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
        radioService[slotId]->mRadioIndication->newSms(
                convertIntToRadioIndicationType(indicationType), pdu);
        free(bytes);
    } else {
        RLOGE("radio::newSmsInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::newSmsStatusReportInd(android::Parcel &p, int slotId, int requestNumber,
                                 int indicationType, int token, RIL_Errno e, void *response,
                                 size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
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
        radioService[slotId]->mRadioIndication->newSmsStatusReport(
                convertIntToRadioIndicationType(indicationType), pdu);
        free(bytes);
    } else {
        RLOGE("radio::newSmsStatusReportInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::newSmsOnSimInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                          int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("radio::newSmsOnSimInd: invalid response");
            return 0;
        }
        int32_t recordNumber = ((int32_t *) response)[0];
        RLOGD("radio::newSmsOnSimInd: slotIndex %d", recordNumber);
        radioService[slotId]->mRadioIndication->newSmsOnSim(
                convertIntToRadioIndicationType(indicationType), recordNumber);
    } else {
        RLOGE("radio::newSmsOnSimInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::onUssdInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                     int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != 2 * sizeof(char *)) {
            RLOGE("radio::onUssdInd: invalid response");
            return 0;
        }
        char **strings = (char **) response;
        char *mode = strings[0];
        hidl_string msg = convertCharPtrToHidlString(strings[1]);
        UssdModeType modeType = (UssdModeType) atoi(mode);
        RLOGD("radio::onUssdInd: mode %s", mode);
        radioService[slotId]->mRadioIndication->onUssd(
                convertIntToRadioIndicationType(indicationType), modeType, msg);
    } else {
        RLOGE("radio::onUssdInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::nitzTimeReceivedInd(android::Parcel &p, int slotId, int requestNumber,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("radio::nitzTimeReceivedInd: invalid response");
            return 0;
        }
        hidl_string nitzTime = convertCharPtrToHidlString((char *) response);
        int64_t timeReceived = android::elapsedRealtime();
        RLOGD("radio::nitzTimeReceivedInd: nitzTime %s receivedTime %ld", nitzTime.c_str(), timeReceived);
        radioService[slotId]->mRadioIndication->nitzTimeReceived(
                convertIntToRadioIndicationType(indicationType), nitzTime, timeReceived);
    } else {
        RLOGE("radio::nitzTimeReceivedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

void convertRilDataCallToHal(RIL_Data_Call_Response_v11 *dcResponse,
        SetupDataCallResult& dcResult) {
    dcResult.status = dcResponse->status;
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
    if (radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen % sizeof(RIL_Data_Call_Response_v11) != 0) {
            RLOGE("radio::dataCallListChangedInd: invalid response");
            return 0;
        }
        hidl_vec<SetupDataCallResult> dcList;
        convertRilDataCallListToHal(response, responseLen, dcList);
        RLOGD("radio::dataCallListChangedInd");
        radioService[slotId]->mRadioIndication->dataCallListChanged(
                convertIntToRadioIndicationType(indicationType), dcList);
    } else {
        RLOGE("radio::dataCallListChangedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::suppSvcNotifyInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                            int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_SuppSvcNotification)) {
            RLOGE("radio::suppSvcNotifyInd: invalid response");
            return 0;
        }

        SuppSvcNotification suppSvc;
        RIL_SuppSvcNotification *ssn = (RIL_SuppSvcNotification *) response;
        suppSvc.isMT = ssn->notificationType;
        suppSvc.code = ssn->code;
        suppSvc.index = ssn->index;
        suppSvc.type = ssn->type;
        suppSvc.number = convertCharPtrToHidlString(ssn->number);

        RLOGD("radio::suppSvcNotifyInd: isMT %d code %d index %d type %d",
                suppSvc.isMT, suppSvc.code, suppSvc.index, suppSvc.type);
        radioService[slotId]->mRadioIndication->suppSvcNotify(
                convertIntToRadioIndicationType(indicationType), suppSvc);
    } else {
        RLOGE("radio::suppSvcNotifyInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::stkSessionEndInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                            int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::stkSessionEndInd");
        radioService[slotId]->mRadioIndication->stkSessionEnd(
                convertIntToRadioIndicationType(indicationType));
    } else {
        RLOGE("radio::stkSessionEndInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::stkProactiveCommandInd(android::Parcel &p, int slotId, int requestNumber,
                                  int indicationType, int token, RIL_Errno e, void *response,
                                  size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("radio::stkProactiveCommandInd: invalid response");
            return 0;
        }
        RLOGD("radio::stkProactiveCommandInd");
        radioService[slotId]->mRadioIndication->stkProactiveCommand(
                convertIntToRadioIndicationType(indicationType),
                convertCharPtrToHidlString((char *) response));
    } else {
        RLOGE("radio::stkProactiveCommandInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::stkEventNotifyInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                             int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("radio::stkEventNotifyInd: invalid response");
            return 0;
        }
        RLOGD("radio::stkEventNotifyInd");
        radioService[slotId]->mRadioIndication->stkEventNotify(
                convertIntToRadioIndicationType(indicationType),
                convertCharPtrToHidlString((char *) response));
    } else {
        RLOGE("radio::stkEventNotifyInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::stkCallSetupInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                           int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("radio::stkCallSetupInd: invalid response");
            return 0;
        }
        int32_t timeout = ((int32_t *) response)[0];
        RLOGD("radio::stkCallSetupInd: timeout %d", timeout);
        radioService[slotId]->mRadioIndication->stkCallSetup(
                convertIntToRadioIndicationType(indicationType), timeout);
    } else {
        RLOGE("radio::stkCallSetupInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::simSmsStorageFullInd(android::Parcel &p, int slotId, int requestNumber,
                                int indicationType, int token, RIL_Errno e, void *response,
                                size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::simSmsStorageFullInd");
        radioService[slotId]->mRadioIndication->simSmsStorageFull(
                convertIntToRadioIndicationType(indicationType));
    } else {
        RLOGE("radio::simSmsStorageFullInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::simRefreshInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                         int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_SimRefreshResponse_v7)) {
            RLOGE("radio::simRefreshInd: invalid response");
            return 0;
        }

        SimRefreshResult refreshResult;
        RIL_SimRefreshResponse_v7 *simRefreshResponse = ((RIL_SimRefreshResponse_v7 *) response);
        refreshResult.type =
                (android::hardware::radio::V1_0::SimRefreshType) simRefreshResponse->result;
        refreshResult.efId = simRefreshResponse->ef_id;
        refreshResult.aid = convertCharPtrToHidlString(simRefreshResponse->aid);

        RLOGD("radio::simRefreshInd: type %d efId %d", refreshResult.type, refreshResult.efId);
        radioService[slotId]->mRadioIndication->simRefresh(
                convertIntToRadioIndicationType(indicationType), refreshResult);
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
    if (radioService[slotId]->mRadioIndication != NULL) {
        bool isGsm;
        CdmaSignalInfoRecord record;
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
        radioService[slotId]->mRadioIndication->callRing(
                convertIntToRadioIndicationType(indicationType), isGsm, record);
    } else {
        RLOGE("radio::callRingInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::simStatusChangedInd(android::Parcel &p, int slotId, int requestNumber,
                               int indicationType, int token, RIL_Errno e, void *response,
                               size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::simStatusChangedInd");
        radioService[slotId]->mRadioIndication->simStatusChanged(
                convertIntToRadioIndicationType(indicationType));
    } else {
        RLOGE("radio::simStatusChangedInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::cdmaNewSmsInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                         int token, RIL_Errno e, void *response, size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_CDMA_SMS_Message)) {
            RLOGE("radio::cdmaNewSmsInd: invalid response");
            return 0;
        }

        CdmaSmsMessage msg;
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
        radioService[slotId]->mRadioIndication->cdmaNewSms(
                convertIntToRadioIndicationType(indicationType), msg);
    } else {
        RLOGE("radio::cdmaNewSmsInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::newBroadcastSmsInd(android::Parcel &p, int slotId, int requestNumber,
                              int indicationType, int token, RIL_Errno e, void *response,
                              size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen == 0) {
            RLOGE("radio::newBroadcastSmsInd: invalid response");
            return 0;
        }

        hidl_vec<uint8_t> data;
        data.setToExternal((uint8_t *) response, responseLen);
        RLOGD("radio::newBroadcastSmsInd");
        radioService[slotId]->mRadioIndication->newBroadcastSms(
                convertIntToRadioIndicationType(indicationType), data);
    } else {
        RLOGE("radio::newBroadcastSmsInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::cdmaRuimSmsStorageFullInd(android::Parcel &p, int slotId, int requestNumber,
                                     int indicationType, int token, RIL_Errno e, void *response,
                                     size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::cdmaRuimSmsStorageFullInd");
        radioService[slotId]->mRadioIndication->cdmaRuimSmsStorageFull(
                convertIntToRadioIndicationType(indicationType));
    } else {
        RLOGE("radio::cdmaRuimSmsStorageFullInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::restrictedStateChangedInd(android::Parcel &p, int slotId, int requestNumber,
                                     int indicationType, int token, RIL_Errno e, void *response,
                                     size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("radio::restrictedStateChangedInd: invalid response");
            return 0;
        }
        int32_t state = ((int32_t *) response)[0];
        RLOGD("radio::restrictedStateChangedInd: state %d", state);
        radioService[slotId]->mRadioIndication->restrictedStateChanged(
                convertIntToRadioIndicationType(indicationType), (PhoneRestrictedState) state);
    } else {
        RLOGE("radio::restrictedStateChangedInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::enterEmergencyCallbackModeInd(android::Parcel &p, int slotId, int requestNumber,
                                         int indicationType, int token, RIL_Errno e, void *response,
                                         size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        RLOGD("radio::enterEmergencyCallbackModeInd");
        radioService[slotId]->mRadioIndication->enterEmergencyCallbackMode(
                convertIntToRadioIndicationType(indicationType));
    } else {
        RLOGE("radio::enterEmergencyCallbackModeInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::cdmaCallWaitingInd(android::Parcel &p, int slotId, int requestNumber,
                              int indicationType, int token, RIL_Errno e, void *response,
                              size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_CDMA_CallWaiting_v6)) {
            RLOGE("radio::cdmaCallWaitingInd: invalid response");
            return 0;
        }

        CdmaCallWaiting callWaitingRecord;
        RIL_CDMA_CallWaiting_v6 *callWaitingRil = ((RIL_CDMA_CallWaiting_v6 *) response);
        callWaitingRecord.number = convertCharPtrToHidlString(callWaitingRil->number);
        callWaitingRecord.numberPresentation =
                (CdmaCallWaitingNumberPresentation) callWaitingRil->numberPresentation;
        callWaitingRecord.name = convertCharPtrToHidlString(callWaitingRil->name);
        convertRilCdmaSignalInfoRecordToHal(&callWaitingRil->signalInfoRecord,
                callWaitingRecord.signalInfoRecord);
        callWaitingRecord.numbertype = (CdmaCallWaitingNumberType) callWaitingRil->number_type;
        callWaitingRecord.numberPlan = (CdmaCallWaitingNumberPlan) callWaitingRil->number_plan;

        RLOGD("radio::cdmaCallWaitingInd");
        radioService[slotId]->mRadioIndication->cdmaCallWaiting(
                convertIntToRadioIndicationType(indicationType), callWaitingRecord);
    } else {
        RLOGE("radio::cdmaCallWaitingInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

int radio::cdmaOtaProvisionStatusInd(android::Parcel &p, int slotId, int requestNumber,
                                     int indicationType, int token, RIL_Errno e, void *response,
                                     size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(int)) {
            RLOGE("radio::cdmaOtaProvisionStatusInd: invalid response");
            return 0;
        }
        int32_t status = ((int32_t *) response)[0];
        RLOGD("radio::cdmaOtaProvisionStatusInd: status %d", status);
        radioService[slotId]->mRadioIndication->cdmaOtaProvisionStatus(
                convertIntToRadioIndicationType(indicationType), (CdmaOtaProvisionStatus) status);
    } else {
        RLOGE("radio::cdmaOtaProvisionStatusInd: radioService[%d]->mRadioIndication == NULL",
                slotId);
    }

    return 0;
}

int radio::cdmaInfoRecInd(android::Parcel &p, int slotId, int requestNumber,
                          int indicationType, int token, RIL_Errno e, void *response,
                          size_t responseLen) {
    if (radioService[slotId]->mRadioIndication != NULL) {
        if (response == NULL || responseLen != sizeof(RIL_CDMA_InformationRecords)) {
            RLOGE("radio::cdmaInfoRecInd: invalid response");
            return 0;
        }

        CdmaInformationRecords records;
        RIL_CDMA_InformationRecords *recordsRil = (RIL_CDMA_InformationRecords *) response;

        char* string8 = NULL;
        int num = MIN(recordsRil->numberOfInfoRecs, RIL_CDMA_MAX_NUMBER_OF_INFO_RECS);
        if (recordsRil->numberOfInfoRecs > RIL_CDMA_MAX_NUMBER_OF_INFO_RECS) {
            RLOGE("radio::cdmaInfoRecInd: received %d recs which is more than %d, dropping \
                    additional ones",
                    recordsRil->numberOfInfoRecs,
                    RIL_CDMA_MAX_NUMBER_OF_INFO_RECS);
        }
        records.infoRec.resize(num);
        for (int i = 0 ; i < num ; i++) {
            CdmaInformationRecord *record = &records.infoRec[i];
            RIL_CDMA_InformationRecord *infoRec = &recordsRil->infoRec[i];
            record->name = (CdmaInfoRecName) infoRec->name;
            switch (infoRec->name) {
                case RIL_CDMA_DISPLAY_INFO_REC:
                case RIL_CDMA_EXTENDED_DISPLAY_INFO_REC: {
                    if (infoRec->rec.display.alpha_len > CDMA_ALPHA_INFO_BUFFER_LENGTH) {
                        RLOGE("radio::cdmaInfoRecInd: invalid display info response length %d \
                                expected not more than %d", (int) infoRec->rec.display.alpha_len,
                                CDMA_ALPHA_INFO_BUFFER_LENGTH);
                        return 0;
                    }
                    string8 = (char*) malloc((infoRec->rec.display.alpha_len + 1) * sizeof(char));
                    if (string8 == NULL) {
                        RLOGE("radio::cdmaInfoRecInd: Memory allocation failed for \
                                responseCdmaInformationRecords");
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
                        RLOGE("radio::cdmaInfoRecInd: invalid display info response length %d \
                                expected not more than %d", (int) infoRec->rec.number.len,
                                CDMA_NUMBER_INFO_BUFFER_LENGTH);
                        return 0;
                    }
                    string8 = (char*) malloc((infoRec->rec.number.len + 1) * sizeof(char));
                    if (string8 == NULL) {
                        RLOGE("radio::cdmaInfoRecInd: Memory allocation failed for \
                                responseCdmaInformationRecords");
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
                        RLOGE("radio::cdmaInfoRecInd: invalid display info response length %d \
                                expected not more than %d\n",
                                (int)infoRec->rec.redir.redirectingNumber.len,
                                CDMA_NUMBER_INFO_BUFFER_LENGTH);
                        return 0;
                    }
                    string8 = (char*) malloc((infoRec->rec.redir.redirectingNumber.len + 1) *
                            sizeof(char));
                    if (string8 == NULL) {
                        RLOGE("radio::cdmaInfoRecInd: Memory allocation failed for \
                                responseCdmaInformationRecords");
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
        radioService[slotId]->mRadioIndication->cdmaInfoRec(
                convertIntToRadioIndicationType(indicationType), records);
    } else {
        RLOGE("radio::cdmaInfoRecInd: radioService[%d]->mRadioIndication == NULL", slotId);
    }

    return 0;
}

void radio::registerService(RIL_RadioFunctions *callbacks, CommandInfo *commands) {
    using namespace android::hardware;
    int simCount = 1;
    char *serviceNames[] = {
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

    #if (SIM_COUNT >= 2)
    simCount = SIM_COUNT;
    #endif

    configureRpcThreadpool(1, true /* callerWillJoin */);
    for (int i = 0; i < simCount; i++) {
        radioService[i] = new RadioImpl;
        radioService[i]->mSlotId = i;
        RLOGD("radio::registerService: starting IRadio %s", serviceNames[i]);
        android::status_t status = radioService[i]->registerAsService(serviceNames[i]);
    }

    s_callbacks = callbacks;
    s_commands = commands;
}

void rilc_thread_pool() {
    joinRpcThreadpool();
}