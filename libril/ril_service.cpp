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

struct RadioImpl;

#if (SIM_COUNT >= 2)
sp<RadioImpl> radioService[SIM_COUNT];
#else
sp<RadioImpl> radioService[1];
#endif

struct RadioImpl : public IRadio {
    int32_t slotId;
    sp<IRadioResponse> radioResponse;
    sp<IRadioIndication> radioIndication;

    Return<void> setResponseFunctions(
            const ::android::sp<IRadioResponse>& radioResponse,
            const ::android::sp<IRadioIndication>& radioIndication);

    Return<void> getIccCardStatus(int32_t serial);

    Return<void> supplyIccPinForApp(int32_t serial,
            const ::android::hardware::hidl_string& pin,
            const ::android::hardware::hidl_string& aid);

    Return<void> supplyIccPukForApp(int32_t serial,
            const ::android::hardware::hidl_string& puk,
            const ::android::hardware::hidl_string& pin,
            const ::android::hardware::hidl_string& aid);

    Return<void> supplyIccPin2ForApp(int32_t serial,
            const ::android::hardware::hidl_string& pin2,
            const ::android::hardware::hidl_string& aid);

    Return<void> supplyIccPuk2ForApp(int32_t serial,
            const ::android::hardware::hidl_string& puk2,
            const ::android::hardware::hidl_string& pin2,
            const ::android::hardware::hidl_string& aid);

    Return<void> changeIccPinForApp(int32_t serial,
            const ::android::hardware::hidl_string& oldPin,
            const ::android::hardware::hidl_string& newPin,
            const ::android::hardware::hidl_string& aid);

    Return<void> changeIccPin2ForApp(int32_t serial,
            const ::android::hardware::hidl_string& oldPin2,
            const ::android::hardware::hidl_string& newPin2,
            const ::android::hardware::hidl_string& aid);

    Return<void> supplyNetworkDepersonalization(int32_t serial,
            const ::android::hardware::hidl_string& netPin);

    Return<void> getCurrentCalls(int32_t serial);

    Return<void> dial(int32_t serial,
            const Dial& dialInfo);

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
            const GsmBroadcastSmsConfigInfo& configInfo);

    Return<void> setGsmBroadcastActivation(int32_t serial, bool activate);

    Return<void> getCdmaBroadcastConfig(int32_t serial);

    Return<void> setCdmaBroadcastConfig(int32_t serial,
            const CdmaBroadcastSmsConfigInfo& configInfo);

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
};

Return<void> RadioImpl::setResponseFunctions(
        const ::android::sp<IRadioResponse>& radioResponseParam,
        const ::android::sp<IRadioIndication>& radioIndicationParam) {
    RLOGD("RadioImpl::setResponseFunctions");
    radioResponse = radioResponseParam;
    radioIndication = radioIndicationParam;
    return Status::ok();
}

Return<void> RadioImpl::getIccCardStatus(int32_t serial) {
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

Return<void> RadioImpl::supplyIccPinForApp(int32_t serial,
        const ::android::hardware::hidl_string& pin,
        const ::android::hardware::hidl_string& aid) {return Status::ok();}

Return<void> RadioImpl::supplyIccPukForApp(int32_t serial,
        const ::android::hardware::hidl_string& puk,
        const ::android::hardware::hidl_string& pin,
        const ::android::hardware::hidl_string& aid) {return Status::ok();}

Return<void> RadioImpl::supplyIccPin2ForApp(int32_t serial,
        const ::android::hardware::hidl_string& pin2,
        const ::android::hardware::hidl_string& aid) {return Status::ok();}

Return<void> RadioImpl::supplyIccPuk2ForApp(int32_t serial,
        const ::android::hardware::hidl_string& puk2,
        const ::android::hardware::hidl_string& pin2,
        const ::android::hardware::hidl_string& aid) {return Status::ok();}

Return<void> RadioImpl::changeIccPinForApp(int32_t serial,
        const ::android::hardware::hidl_string& oldPin,
        const ::android::hardware::hidl_string& newPin,
        const ::android::hardware::hidl_string& aid) {return Status::ok();}

Return<void> RadioImpl::changeIccPin2ForApp(int32_t serial,
        const ::android::hardware::hidl_string& oldPin2,
        const ::android::hardware::hidl_string& newPin2,
        const ::android::hardware::hidl_string& aid) {return Status::ok();}

Return<void> RadioImpl::supplyNetworkDepersonalization(int32_t serial,
        const ::android::hardware::hidl_string& netPin) {return Status::ok();}

Return<void> RadioImpl::getCurrentCalls(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::dial(int32_t serial,
        const Dial& dialInfo) {return Status::ok();}

Return<void> RadioImpl::getImsiForApp(int32_t serial,
        const ::android::hardware::hidl_string& aid) {return Status::ok();}

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
        const GsmBroadcastSmsConfigInfo& configInfo) {return Status::ok();}

Return<void> RadioImpl::setGsmBroadcastActivation(int32_t serial, bool activate) {return Status::ok();}

Return<void> RadioImpl::getCdmaBroadcastConfig(int32_t serial) {return Status::ok();}

Return<void> RadioImpl::setCdmaBroadcastConfig(int32_t serial,
        const CdmaBroadcastSmsConfigInfo& configInfo) {return Status::ok();}

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

int radio::iccCardStatusResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responselen) {
    RLOGD("iccCardStatusResponse: serial %d", serial);

    if (radioService[slotId]->radioResponse != NULL) {
        RadioResponseInfo responseInfo;
        CardStatus cardStatus;
        responseInfo.serial = serial;

        switch (responseType) {
            case RESPONSE_SOLICITED:
                responseInfo.type = RadioResponseType::SOLICITED;
                break;
            case RESPONSE_SOLICITED_ACK:
                responseInfo.type = RadioResponseType::SOLICITED_ACK;
                break;
            case RESPONSE_SOLICITED_ACK_EXP:
                responseInfo.type = RadioResponseType::SOLICITED_ACK_EXP;
                break;
        }

        if (response == NULL && responselen != 0) {
            RLOGE("iccCardStatusResponse: invalid response: NULL");
            //todo: it used to be -1 (RIL_ERRNO_INVALID_RESPONSE) but adding that to interface
            // doesn't make sense since this will eventually be part of vendor ril. Options to
            // handle this:
            // 1. Add -1 to interface and use that, and update interface to say it's a valid error
            // 2. Add GENERIC_FAILURE as valid error to interface
            // 3. Assume this will never happen and not handle it
            responseInfo.error = RadioError::GENERIC_FAILURE;
        } else {
            responseInfo.error = (RadioError) e;

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
        }

        radioService[slotId]->radioResponse->iccCardStatusResponse(responseInfo, cardStatus);
    } else {
        RLOGE("iccCardStatusResponse: radioService[%d]->radioResponse == NULL", slotId);
    }

    return 0;
}

void radio::radioStateChanged(int slotId, int indicationType, RIL_RadioState radioState) {
    RLOGD("radioStateChanged: radioState %d", radioState);
    if (radioService[slotId]->radioIndication != NULL) {
        RadioIndicationType type = indicationType == RESPONSE_UNSOLICITED ?
                (RadioIndicationType::UNSOLICITED) :
                (RadioIndicationType::UNSOLICITED_ACK_EXP);
        radioService[slotId]->radioIndication->radioStateChanged(type, (RadioState) radioState);
    } else {
        RLOGE("radioStateChanged: radioService[%d]->radioIndication == NULL", slotId);
    }
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

    for (int i = 0; i < simCount; i++) {
        radioService[i] = new RadioImpl;
        radioService[i]->slotId = i;
        RLOGD("registerService: starting IRadio %s", serviceNames[i]);
        android::status_t status = radioService[i]->registerAsService(serviceNames[i]);
    }

    s_callbacks = callbacks;
    s_commands = commands;
}

void rilc_thread_pool() {
    android::hardware::ProcessState::self()->setThreadPoolMaxThreadCount(0);
    android::hardware::ProcessState::self()->startThreadPool();
    android::hardware::IPCThreadState::self()->joinThreadPool();
}