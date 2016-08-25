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

#include <hidl/IServiceManager.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <ril_service.h>

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

RIL_RadioFunctions *s_callbacks = NULL;
static CommandInfo *s_commands;
sp<IRadioResponse> radioResponse;
sp<IRadioIndication> radioIndication;

struct RadioImpl : public IRadio {
    int32_t slotId;
    Return<void> setResponseFunctions(int32_t slotId,
            const ::android::sp<IRadioResponse>& radioResponse,
            const ::android::sp<IRadioIndication>& radioIndication);

    Return<void> getIccCardStatus(int32_t slotId, int32_t serial);

    Return<void> supplyIccPinForApp(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& pin,
            const ::android::hardware::hidl_string& aid);

    Return<void> supplyIccPukForApp(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& puk,
            const ::android::hardware::hidl_string& pin,
            const ::android::hardware::hidl_string& aid);

    Return<void> supplyIccPin2ForApp(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& pin2,
            const ::android::hardware::hidl_string& aid);

    Return<void> supplyIccPuk2ForApp(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& puk2,
            const ::android::hardware::hidl_string& pin2,
            const ::android::hardware::hidl_string& aid);

    Return<void> changeIccPinForApp(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& oldPin,
            const ::android::hardware::hidl_string& newPin,
            const ::android::hardware::hidl_string& aid);

    Return<void> changeIccPin2ForApp(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& oldPin2,
            const ::android::hardware::hidl_string& newPin2,
            const ::android::hardware::hidl_string& aid);

    Return<void> supplyNetworkDepersonalization(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& netPin);

    Return<void> getCurrentCalls(int32_t slotId, int32_t serial);

    Return<void> dial(int32_t slotId, int32_t serial,
            const Dial& dialInfo);

    Return<void> getImsiForApp(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& aid);

    Return<void> hangup(int32_t slotId, int32_t serial, int32_t gsmIndex);

    Return<void> hangupWaitingOrBackground(int32_t slotId, int32_t serial);

    Return<void> hangupForegroundResumeBackground(int32_t slotId, int32_t serial);

    Return<void> switchWaitingOrHoldingAndActive(int32_t slotId, int32_t serial);

    Return<void> conference(int32_t slotId, int32_t serial);

    Return<void> rejectCall(int32_t slotId, int32_t serial);

    Return<void> getLastCallFailCause(int32_t slotId, int32_t serial);

    Return<void> getSignalStrength(int32_t slotId, int32_t serial);

    Return<void> getVoiceRegistrationState(int32_t slotId, int32_t serial);

    Return<void> getDataRegistrationState(int32_t slotId, int32_t serial);

    Return<void> getOperator(int32_t slotId, int32_t serial);

    Return<void> setRadioPower(int32_t slotId, int32_t serial, bool on);

    Return<void> sendDtmf(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& s);

    Return<void> sendSms(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& smscPDU,
            const ::android::hardware::hidl_string& pdu);

    Return<void> sendSMSExpectMore(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& smscPDU,
            const ::android::hardware::hidl_string& pdu);

    Return<void> setupDataCall(int32_t slotId, int32_t serial,
            int32_t radioTechnology,
            int32_t profile,
            const ::android::hardware::hidl_string& apn,
            const ::android::hardware::hidl_string& user,
            const ::android::hardware::hidl_string& password,
            int32_t authType,
            const ::android::hardware::hidl_string& protocol);

    Return<void> iccIOForApp(int32_t slotId, int32_t serial,
            const IccIo& iccIo);

    Return<void> sendUssd(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& ussd);

    Return<void> cancelPendingUssd(int32_t slotId, int32_t serial);

    Return<void> getClir(int32_t slotId, int32_t serial);

    Return<void> setClir(int32_t slotId, int32_t serial, int32_t status);

    Return<void> getCallForwardStatus(int32_t slotId, int32_t serial,
            const CallForwardInfo& callInfo);

    Return<void> setCallForward(int32_t slotId, int32_t serial,
            const CallForwardInfo& callInfo);

    Return<void> getCallWaiting(int32_t slotId, int32_t serial, int32_t serviceClass);

    Return<void> setCallWaiting(int32_t slotId, int32_t serial, bool enable, int32_t serviceClass);

    Return<void> acknowledgeLastIncomingGsmSms(int32_t slotId, int32_t serial,
            bool success, SmsAcknowledgeFailCause cause);

    Return<void> acceptCall(int32_t slotId, int32_t serial);

    Return<void> deactivateDataCall(int32_t slotId, int32_t serial,
            int32_t cid, bool reasonRadioShutDown);

    Return<void> getFacilityLockForApp(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& facility,
            const ::android::hardware::hidl_string& password,
            int32_t serviceClass,
            const ::android::hardware::hidl_string& appId);

    Return<void> setFacilityLockForApp(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& facility,
            bool lockState,
            const ::android::hardware::hidl_string& password,
            int32_t serviceClass,
            const ::android::hardware::hidl_string& appId);

    Return<void> setBarringPassword(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& facility,
            const ::android::hardware::hidl_string& oldPassword,
            const ::android::hardware::hidl_string& newPassword);

    Return<void> getNetworkSelectionMode(int32_t slotId, int32_t serial);

    Return<void> setNetworkSelectionModeAutomatic(int32_t slotId, int32_t serial);

    Return<void> setNetworkSelectionModeManual(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& operatorNumeric);

    Return<void> getAvailableNetworks(int32_t slotId, int32_t serial);

    Return<void> startDtmf(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& s);

    Return<void> stopDtmf(int32_t slotId, int32_t serial);

    Return<void> getBasebandVersion(int32_t slotId, int32_t serial);

    Return<void> separateConnection(int32_t slotId, int32_t serial, int32_t gsmIndex);

    Return<void> setMute(int32_t slotId, int32_t serial, bool enable);

    Return<void> getMute(int32_t slotId, int32_t serial);

    Return<void> getClip(int32_t slotId, int32_t serial);

    Return<void> getDataCallList(int32_t slotId, int32_t serial);

    Return<void> sendOemRilRequestRaw(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_vec<uint8_t>& data);

    Return<void> sendOemRilRequestStrings(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& data);

    Return<void> sendScreenState(int32_t slotId, int32_t serial, bool enable);

    Return<void> setSuppServiceNotifications(int32_t slotId, int32_t serial, bool enable);

    Return<void> writeSmsToSim(int32_t slotId, int32_t serial,
            const SmsWriteArgs& smsWriteArgs);

    Return<void> deleteSmsOnSim(int32_t slotId, int32_t serial, int32_t index);

    Return<void> setBandMode(int32_t slotId, int32_t serial, RadioBandMode mode);

    Return<void> getAvailableBandModes(int32_t slotId, int32_t serial);

    Return<void> sendEnvelope(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& command);

    Return<void> sendTerminalResponseToSim(int32_t slotId, int32_t serial,
            const ::android::hardware::hidl_string& commandResponse);
};

Return<void> RadioImpl::setResponseFunctions(int32_t slotId,
        const ::android::sp<IRadioResponse>& radioResponseParam,
        const ::android::sp<IRadioIndication>& radioIndicationParam) {
    RLOGD("RadioImpl::setResponseFunctions");
    // todo: should have multiple callback objects, and use slotId as index
    radioResponse = radioResponseParam;
    radioIndication = radioIndicationParam;
    return Status::ok();
}

Return<void> RadioImpl::getIccCardStatus(int32_t slotId, int32_t serial) {
    RLOGD("RadioImpl::getIccCardStatus: serial %d", serial);
    RequestInfo *pRI;
    pRI = (RequestInfo *)calloc(1, sizeof(RequestInfo));
    if (pRI == NULL) {
        RLOGE("RadioImpl::getIccCardStatus: Memory allocation failed for request %s",
                requestToString(RIL_REQUEST_GET_SIM_STATUS));
        return Void();
    }

    pRI->token = serial;
    pRI->pCI = &(s_commands[RIL_REQUEST_GET_SIM_STATUS]);
    pRI->socket_id = (RIL_SOCKET_ID) slotId;
    android::addRequestToList(pRI, RIL_REQUEST_GET_SIM_STATUS, serial, pRI->socket_id);

    s_callbacks->onRequest(RIL_REQUEST_GET_SIM_STATUS, NULL, 0, pRI);

    return Status::ok();
}

Return<void> RadioImpl::supplyIccPinForApp(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& pin,
        const ::android::hardware::hidl_string& aid) {return Status::ok();}

Return<void> RadioImpl::supplyIccPukForApp(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& puk,
        const ::android::hardware::hidl_string& pin,
        const ::android::hardware::hidl_string& aid) {return Status::ok();}

Return<void> RadioImpl::supplyIccPin2ForApp(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& pin2,
        const ::android::hardware::hidl_string& aid) {return Status::ok();}

Return<void> RadioImpl::supplyIccPuk2ForApp(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& puk2,
        const ::android::hardware::hidl_string& pin2,
        const ::android::hardware::hidl_string& aid) {return Status::ok();}

Return<void> RadioImpl::changeIccPinForApp(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& oldPin,
        const ::android::hardware::hidl_string& newPin,
        const ::android::hardware::hidl_string& aid) {return Status::ok();}

Return<void> RadioImpl::changeIccPin2ForApp(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& oldPin2,
        const ::android::hardware::hidl_string& newPin2,
        const ::android::hardware::hidl_string& aid) {return Status::ok();}

Return<void> RadioImpl::supplyNetworkDepersonalization(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& netPin) {return Status::ok();}

Return<void> RadioImpl::getCurrentCalls(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::dial(int32_t slotId, int32_t serial,
        const Dial& dialInfo) {return Status::ok();}

Return<void> RadioImpl::getImsiForApp(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& aid) {return Status::ok();}

Return<void> RadioImpl::hangup(int32_t slotId, int32_t serial, int32_t gsmIndex) {return Status::ok();}

Return<void> RadioImpl::hangupWaitingOrBackground(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::hangupForegroundResumeBackground(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::switchWaitingOrHoldingAndActive(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::conference(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::rejectCall(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getLastCallFailCause(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getSignalStrength(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getVoiceRegistrationState(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getDataRegistrationState(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getOperator(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setRadioPower(int32_t slotId, int32_t serial, bool on) {return Status::ok();}

Return<void> RadioImpl::sendDtmf(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& s) {return Status::ok();}

Return<void> RadioImpl::sendSms(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& smscPDU,
        const ::android::hardware::hidl_string& pdu) {return Status::ok();}

Return<void> RadioImpl::sendSMSExpectMore(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& smscPDU,
        const ::android::hardware::hidl_string& pdu) {return Status::ok();}

Return<void> RadioImpl::setupDataCall(int32_t slotId, int32_t serial,
        int32_t radioTechnology,
        int32_t profile,
        const ::android::hardware::hidl_string& apn,
        const ::android::hardware::hidl_string& user,
        const ::android::hardware::hidl_string& password,
        int32_t authType,
        const ::android::hardware::hidl_string& protocol) {return Status::ok();}

Return<void> RadioImpl::iccIOForApp(int32_t slotId, int32_t serial,
        const IccIo& iccIo) {return Status::ok();}

Return<void> RadioImpl::sendUssd(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& ussd) {return Status::ok();}

Return<void> RadioImpl::cancelPendingUssd(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getClir(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setClir(int32_t slotId, int32_t serial, int32_t status) {return Status::ok();}

Return<void> RadioImpl::getCallForwardStatus(int32_t slotId, int32_t serial,
        const CallForwardInfo& callInfo) {return Status::ok();}

Return<void> RadioImpl::setCallForward(int32_t slotId, int32_t serial,
        const CallForwardInfo& callInfo) {return Status::ok();}

Return<void> RadioImpl::getCallWaiting(int32_t slotId, int32_t serial, int32_t serviceClass) {return Status::ok();}

Return<void> RadioImpl::setCallWaiting(int32_t slotId, int32_t serial, bool enable, int32_t serviceClass) {return Status::ok();}

Return<void> RadioImpl::acknowledgeLastIncomingGsmSms(int32_t slotId, int32_t serial,
        bool success, SmsAcknowledgeFailCause cause) {return Status::ok();}

Return<void> RadioImpl::acceptCall(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::deactivateDataCall(int32_t slotId, int32_t serial,
        int32_t cid, bool reasonRadioShutDown) {return Status::ok();}

Return<void> RadioImpl::getFacilityLockForApp(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& facility,
        const ::android::hardware::hidl_string& password,
        int32_t serviceClass,
        const ::android::hardware::hidl_string& appId) {return Status::ok();}

Return<void> RadioImpl::setFacilityLockForApp(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& facility,
        bool lockState,
        const ::android::hardware::hidl_string& password,
        int32_t serviceClass,
        const ::android::hardware::hidl_string& appId) {return Status::ok();}

Return<void> RadioImpl::setBarringPassword(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& facility,
        const ::android::hardware::hidl_string& oldPassword,
        const ::android::hardware::hidl_string& newPassword) {return Status::ok();}

Return<void> RadioImpl::getNetworkSelectionMode(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setNetworkSelectionModeAutomatic(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setNetworkSelectionModeManual(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& operatorNumeric) {return Status::ok();}

Return<void> RadioImpl::getAvailableNetworks(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::startDtmf(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& s) {return Status::ok();}

Return<void> RadioImpl::stopDtmf(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getBasebandVersion(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::separateConnection(int32_t slotId, int32_t serial, int32_t gsmIndex) {return Status::ok();}

Return<void> RadioImpl::setMute(int32_t slotId, int32_t serial, bool enable) {return Status::ok();}

Return<void> RadioImpl::getMute(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getClip(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::getDataCallList(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::sendOemRilRequestRaw(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_vec<uint8_t>& data) {return Status::ok();}

Return<void> RadioImpl::sendOemRilRequestStrings(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& data) {return Status::ok();}

Return<void> RadioImpl::sendScreenState(int32_t slotId, int32_t serial, bool enable) {return Status::ok();}

Return<void> RadioImpl::setSuppServiceNotifications(int32_t slotId, int32_t serial, bool enable) {return Status::ok();}

Return<void> RadioImpl::writeSmsToSim(int32_t slotId, int32_t serial,
        const SmsWriteArgs& smsWriteArgs) {return Status::ok();}

Return<void> RadioImpl::deleteSmsOnSim(int32_t slotId, int32_t serial, int32_t index) {return Status::ok();}

Return<void> RadioImpl::setBandMode(int32_t slotId, int32_t serial, RadioBandMode mode) {return Status::ok();}

Return<void> RadioImpl::getAvailableBandModes(int32_t slotId, int32_t serial) {return Status::ok();}

Return<void> RadioImpl::sendEnvelope(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& command) {return Status::ok();}

Return<void> RadioImpl::sendTerminalResponseToSim(int32_t slotId, int32_t serial,
        const ::android::hardware::hidl_string& commandResponse) {return Status::ok();}

void radio::iccCardStatusResponse(int32_t responseType, int32_t serial, RIL_Errno e, void *response,
        size_t responselen) {
    RLOGD("iccCardStatusResponse: serial %d", serial);
    if (radioResponse != NULL) {
        RadioResponseInfo responseInfo;
        responseInfo.type = (RadioResponseType) responseType;
        responseInfo.serial = serial;
        responseInfo.error = (RadioError) e;
        CardStatus cardStatus;
        RIL_CardStatus_v6 *p_cur = ((RIL_CardStatus_v6 *) response);
        cardStatus.cardState = (CardState) p_cur->card_state;
        cardStatus.universalPinState = (PinState) p_cur->universal_pin_state;
        cardStatus.gsmUmtsSubscriptionAppIndex = p_cur->gsm_umts_subscription_app_index;
        cardStatus.cdmaSubscriptionAppIndex = p_cur->cdma_subscription_app_index;
        cardStatus.imsSubscriptionAppIndex = p_cur->ims_subscription_app_index;
        cardStatus.numApplications = p_cur->num_applications;

        RIL_AppStatus *rilAppStatus = p_cur->applications;
        AppStatus *appStatus = cardStatus.applications.data();
        RLOGD("iccCardStatusResponse: num_applications %d", p_cur->num_applications);
        for (int i = 0; i < p_cur->num_applications; i++) {
            appStatus[i].appType = (AppType) rilAppStatus[i].app_type;
            appStatus[i].appState = (AppState) rilAppStatus[i].app_state;
            appStatus[i].persoSubstate = (PersoSubstate) rilAppStatus[i].perso_substate;
            appStatus[i].aidPtr = (const char*)(rilAppStatus[i].aid_ptr);
            appStatus[i].appLabelPtr = (const char*)(rilAppStatus[i].app_label_ptr);
            appStatus[i].pin1Replaced = rilAppStatus[i].pin1_replaced;
            appStatus[i].pin1 = (PinState) rilAppStatus[i].pin1;
            appStatus[i].pin2 = (PinState) rilAppStatus[i].pin2;
        }

        radioResponse->iccCardStatusResponse(responseInfo, cardStatus);
    } else {
        RLOGE("responseGetSimStatus: radioResponse == NULL");
    }
}

void radio::radioStateChanged(RIL_RadioState radioState) {
    RLOGD("radioStateChanged: radioState %d", radioState);
    if (radioIndication != NULL) {
        radioIndication->radioStateChanged((RadioState) radioState);
    } else {
        RLOGE("radioStateChanged: radioIndication == NULL");
    }
}

void radio::registerService(RIL_RadioFunctions *callbacks, CommandInfo *commands) {
    using namespace android::hardware;
    sp<RadioImpl> radioService = new RadioImpl;
    radioService->slotId = 0;

    s_callbacks = callbacks;
    s_commands = commands;

    RLOGD("registerService: starting IRadio (ril_service0)");
    android::status_t status = radioService->registerAsService("ril_service0");

    /* Start more services for multi-sim */
    #if (SIM_COUNT >= 2)
    sp<RadioImpl> radioService = new RadioImpl;
    radioService->slotId = 1;

    RLOGD("registerService: starting IRadio (ril_service1)");
    android::status_t status = radioService->registerAsService("ril_service1");

    #if (SIM_COUNT >= 3)
    sp<RadioImpl> radioService = new RadioImpl;
    radioService->slotId = 2;

    RLOGD("registerService: starting IRadio (ril_service2)");
    android::status_t status = radioService->registerAsService("ril_service2");

    #if (SIM_COUNT >= 4)
    sp<RadioImpl> radioService = new RadioImpl;
    radioService->slotId = 3;

    RLOGD("registerService: starting IRadio (ril_service3)");
    android::status_t status = radioService->registerAsService("ril_service3");
    #endif // SIM_COUNT >= 2
    #endif // SIM_COUNT >= 3
    #endif // SIM_COUNT >= 4
}

void rilc_thread_pool() {
    android::hardware::ProcessState::self()->setThreadPoolMaxThreadCount(0);
    android::hardware::ProcessState::self()->startThreadPool();
    android::hardware::IPCThreadState::self()->joinThreadPool();
}