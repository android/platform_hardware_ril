/* //device/libs/telephony/ril_commands.h
**
** Copyright 2006, The Android Open Source Project
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
    {0, NULL, NULL},                   //none
    {RIL_REQUEST_GET_SIM_STATUS, dispatchVoid, radio::getIccCardStatusResponse},
    {RIL_REQUEST_ENTER_SIM_PIN, dispatchStrings, radio::supplyIccPinForAppResponse},
    {RIL_REQUEST_ENTER_SIM_PUK, dispatchStrings, radio::supplyIccPukForAppResponse},
    {RIL_REQUEST_ENTER_SIM_PIN2, dispatchStrings, radio::supplyIccPin2ForAppResponse},
    {RIL_REQUEST_ENTER_SIM_PUK2, dispatchStrings, radio::supplyIccPuk2ForAppResponse},
    {RIL_REQUEST_CHANGE_SIM_PIN, dispatchStrings, radio::changeIccPinForAppResponse},
    {RIL_REQUEST_CHANGE_SIM_PIN2, dispatchStrings, radio::changeIccPin2ForAppResponse},
    {RIL_REQUEST_ENTER_NETWORK_DEPERSONALIZATION, dispatchStrings, radio::supplyNetworkDepersonalizationResponse},
    {RIL_REQUEST_GET_CURRENT_CALLS, dispatchVoid, radio::getCurrentCallsResponse},
    {RIL_REQUEST_DIAL, dispatchDial, radio::dialResponse},
    {RIL_REQUEST_GET_IMSI, dispatchStrings, radio::getIMSIForAppResponse},
    {RIL_REQUEST_HANGUP, dispatchInts, radio::hangupConnectionResponse},
    {RIL_REQUEST_HANGUP_WAITING_OR_BACKGROUND, dispatchVoid, radio::hangupWaitingOrBackgroundResponse},
    {RIL_REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND, dispatchVoid, radio::hangupForegroundResumeBackgroundResponse},
    {RIL_REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE, dispatchVoid, radio::switchWaitingOrHoldingAndActiveResponse},
    {RIL_REQUEST_CONFERENCE, dispatchVoid, radio::conferenceResponse},
    {RIL_REQUEST_UDUB, dispatchVoid, radio::rejectCallResponse},
    {RIL_REQUEST_LAST_CALL_FAIL_CAUSE, dispatchVoid, radio::getLastCallFailCauseResponse},
    {RIL_REQUEST_SIGNAL_STRENGTH, dispatchVoid, responseRilSignalStrength},
    {RIL_REQUEST_VOICE_REGISTRATION_STATE, dispatchVoid, radio::getVoiceRegistrationStateResponse},
    {RIL_REQUEST_DATA_REGISTRATION_STATE, dispatchVoid, radio::getDataRegistrationStateResponse},
    {RIL_REQUEST_OPERATOR, dispatchVoid, radio::getOperatorResponse},
    {RIL_REQUEST_RADIO_POWER, dispatchInts, radio::setRadioPowerResponse},
    {RIL_REQUEST_DTMF, dispatchString, radio::sendDtmfResponse},
    {RIL_REQUEST_SEND_SMS, dispatchStrings, radio::sendSmsResponse},
    {RIL_REQUEST_SEND_SMS_EXPECT_MORE, dispatchStrings, radio::sendSMSExpectMoreResponse},
    {RIL_REQUEST_SETUP_DATA_CALL, dispatchDataCall, responseSetupDataCall},
    {RIL_REQUEST_SIM_IO, dispatchSIM_IO, radio::iccIOForAppResponse},
    {RIL_REQUEST_SEND_USSD, dispatchString, radio::sendUssdResponse},
    {RIL_REQUEST_CANCEL_USSD, dispatchVoid, radio::cancelPendingUssdResponse},
    {RIL_REQUEST_GET_CLIR, dispatchVoid, radio::getClirResponse},
    {RIL_REQUEST_SET_CLIR, dispatchInts, radio::setClirResponse},
    {RIL_REQUEST_QUERY_CALL_FORWARD_STATUS, dispatchCallForward, radio::getCallForwardStatusResponse},
    {RIL_REQUEST_SET_CALL_FORWARD, dispatchCallForward, radio::setCallForwardResponse},
    {RIL_REQUEST_QUERY_CALL_WAITING, dispatchInts, radio::getCallWaitingResponse},
    {RIL_REQUEST_SET_CALL_WAITING, dispatchInts, radio::setCallWaitingResponse},
    {RIL_REQUEST_SMS_ACKNOWLEDGE, dispatchInts, radio::acknowledgeLastIncomingGsmSmsResponse},
    {RIL_REQUEST_GET_IMEI, dispatchVoid, responseString},
    {RIL_REQUEST_GET_IMEISV, dispatchVoid, responseString},
    {RIL_REQUEST_ANSWER,dispatchVoid, radio::acceptCallResponse},
    {RIL_REQUEST_DEACTIVATE_DATA_CALL, dispatchStrings, radio::deactivateDataCallResponse},
    {RIL_REQUEST_QUERY_FACILITY_LOCK, dispatchStrings, radio::getFacilityLockForAppResponse},
    {RIL_REQUEST_SET_FACILITY_LOCK, dispatchStrings, radio::setFacilityLockForAppResponse},
    {RIL_REQUEST_CHANGE_BARRING_PASSWORD, dispatchStrings, radio::setBarringPasswordResponse},
    {RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE, dispatchVoid, radio::getNetworkSelectionModeResponse},
    {RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC, dispatchVoid, radio::setNetworkSelectionModeAutomaticResponse},
    {RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL, dispatchString, radio::setNetworkSelectionModeManualResponse},
    {RIL_REQUEST_QUERY_AVAILABLE_NETWORKS , dispatchVoid, radio::getAvailableNetworksResponse},
    {RIL_REQUEST_DTMF_START, dispatchString, radio::startDtmfResponse},
    {RIL_REQUEST_DTMF_STOP, dispatchVoid, radio::stopDtmfResponse},
    {RIL_REQUEST_BASEBAND_VERSION, dispatchVoid, radio::getBasebandVersionResponse},
    {RIL_REQUEST_SEPARATE_CONNECTION, dispatchInts, radio::separateConnectionResponse},
    {RIL_REQUEST_SET_MUTE, dispatchInts, radio::setMuteResponse},
    {RIL_REQUEST_GET_MUTE, dispatchVoid, radio::getMuteResponse},
    {RIL_REQUEST_QUERY_CLIP, dispatchVoid, radio::getClipResponse},
    {RIL_REQUEST_LAST_DATA_CALL_FAIL_CAUSE, dispatchVoid, responseInts},
    {RIL_REQUEST_DATA_CALL_LIST, dispatchVoid, responseDataCallList},
    {RIL_REQUEST_RESET_RADIO, dispatchVoid, responseVoid},
    {RIL_REQUEST_OEM_HOOK_RAW, dispatchRaw, responseRaw},
    {RIL_REQUEST_OEM_HOOK_STRINGS, dispatchStrings, radio::sendOemRilRequestStringsResponse},
    {RIL_REQUEST_SCREEN_STATE, dispatchInts, radio::sendScreenStateResponse},
    {RIL_REQUEST_SET_SUPP_SVC_NOTIFICATION, dispatchInts, radio::setSuppServiceNotificationsResponse},
    {RIL_REQUEST_WRITE_SMS_TO_SIM, dispatchSmsWrite, radio::writeSmsToSimResponse},
    {RIL_REQUEST_DELETE_SMS_ON_SIM, dispatchInts, radio::deleteSmsOnSimResponse},
    {RIL_REQUEST_SET_BAND_MODE, dispatchInts, radio::setBandModeResponse},
    {RIL_REQUEST_QUERY_AVAILABLE_BAND_MODE, dispatchVoid, radio::getAvailableBandModesResponse},
    {RIL_REQUEST_STK_GET_PROFILE, dispatchVoid, responseString},
    {RIL_REQUEST_STK_SET_PROFILE, dispatchString, responseVoid},
    {RIL_REQUEST_STK_SEND_ENVELOPE_COMMAND, dispatchString, radio::sendEnvelopeResponse},
    {RIL_REQUEST_STK_SEND_TERMINAL_RESPONSE, dispatchString, radio::sendTerminalResponseToSimResponse},
    {RIL_REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM, dispatchInts, radio::handleStkCallSetupRequestFromSimResponse},
    {RIL_REQUEST_EXPLICIT_CALL_TRANSFER, dispatchVoid, radio::explicitCallTransferResponse},
    {RIL_REQUEST_SET_PREFERRED_NETWORK_TYPE, dispatchInts, radio::setPreferredNetworkTypeResponse},
    {RIL_REQUEST_GET_PREFERRED_NETWORK_TYPE, dispatchVoid, radio::getPreferredNetworkTypeResponse},
    {RIL_REQUEST_GET_NEIGHBORING_CELL_IDS, dispatchVoid, radio::getNeighboringCidsResponse},
    {RIL_REQUEST_SET_LOCATION_UPDATES, dispatchInts, radio::setLocationUpdatesResponse},
    {RIL_REQUEST_CDMA_SET_SUBSCRIPTION_SOURCE, dispatchInts, radio::setCdmaSubscriptionSourceResponse},
    {RIL_REQUEST_CDMA_SET_ROAMING_PREFERENCE, dispatchInts, radio::setCdmaRoamingPreferenceResponse},
    {RIL_REQUEST_CDMA_QUERY_ROAMING_PREFERENCE, dispatchVoid, radio::getCdmaRoamingPreferenceResponse},
    {RIL_REQUEST_SET_TTY_MODE, dispatchInts, radio::setTTYModeResponse},
    {RIL_REQUEST_QUERY_TTY_MODE, dispatchVoid, radio::getTTYModeResponse},
    {RIL_REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE, dispatchInts, radio::setPreferredVoicePrivacyResponse},
    {RIL_REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE, dispatchVoid, radio::getPreferredVoicePrivacyResponse},
    {RIL_REQUEST_CDMA_FLASH, dispatchString, radio::sendCDMAFeatureCodeResponse},
    {RIL_REQUEST_CDMA_BURST_DTMF, dispatchStrings, radio::sendBurstDtmfResponse},
    {RIL_REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY, dispatchString, responseVoid},
    {RIL_REQUEST_CDMA_SEND_SMS, dispatchCdmaSms, radio::sendCdmaSmsResponse},
    {RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE, dispatchCdmaSmsAck, radio::acknowledgeLastIncomingCdmaSmsResponse},
    {RIL_REQUEST_GSM_GET_BROADCAST_SMS_CONFIG, dispatchVoid, radio::getGsmBroadcastConfigResponse},
    {RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG, dispatchGsmBrSmsCnf, radio::setGsmBroadcastConfigResponse},
    {RIL_REQUEST_GSM_SMS_BROADCAST_ACTIVATION, dispatchInts, radio::setGsmBroadcastActivationResponse},
    {RIL_REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG, dispatchVoid, radio::getCdmaBroadcastConfigResponse},
    {RIL_REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG, dispatchCdmaBrSmsCnf, radio::setCdmaBroadcastConfigResponse},
    {RIL_REQUEST_CDMA_SMS_BROADCAST_ACTIVATION, dispatchInts, radio::setCdmaBroadcastActivationResponse},
    {RIL_REQUEST_CDMA_SUBSCRIPTION, dispatchVoid, radio::getCDMASubscriptionResponse},
    {RIL_REQUEST_CDMA_WRITE_SMS_TO_RUIM, dispatchRilCdmaSmsWriteArgs, radio::writeSmsToRuimResponse},
    {RIL_REQUEST_CDMA_DELETE_SMS_ON_RUIM, dispatchInts, radio::deleteSmsOnRuimResponse},
    {RIL_REQUEST_DEVICE_IDENTITY, dispatchVoid, radio::getDeviceIdentityResponse},
    {RIL_REQUEST_EXIT_EMERGENCY_CALLBACK_MODE, dispatchVoid, radio::exitEmergencyCallbackModeResponse},
    {RIL_REQUEST_GET_SMSC_ADDRESS, dispatchVoid, radio::getSmscAddressResponse},
    {RIL_REQUEST_SET_SMSC_ADDRESS, dispatchString, radio::setSmscAddressResponse},
    {RIL_REQUEST_REPORT_SMS_MEMORY_STATUS, dispatchInts, radio::reportSmsMemoryStatusResponse},
    {RIL_REQUEST_REPORT_STK_SERVICE_IS_RUNNING, dispatchVoid, responseVoid},
    {RIL_REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE, dispatchVoid, radio::getCdmaSubscriptionSourceResponse},
    {RIL_REQUEST_ISIM_AUTHENTICATION, dispatchString, radio::requestIsimAuthenticationResponse},
    {RIL_REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU, dispatchStrings, radio::acknowledgeIncomingGsmSmsWithPduResponse},
    {RIL_REQUEST_STK_SEND_ENVELOPE_WITH_STATUS, dispatchString, radio::sendEnvelopeWithStatusResponse},
    {RIL_REQUEST_VOICE_RADIO_TECH, dispatchVoid, radio::getVoiceRadioTechnologyResponse},
    {RIL_REQUEST_GET_CELL_INFO_LIST, dispatchVoid, responseCellInfoList},
    {RIL_REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE, dispatchInts, radio::setCellInfoListRateResponse},
    {RIL_REQUEST_SET_INITIAL_ATTACH_APN, dispatchSetInitialAttachApn, responseVoid},
    {RIL_REQUEST_IMS_REGISTRATION_STATE, dispatchVoid, radio::getImsRegistrationStateResponse},
    {RIL_REQUEST_IMS_SEND_SMS, dispatchImsSms, radio::sendImsSmsResponse},
    {RIL_REQUEST_SIM_TRANSMIT_APDU_BASIC, dispatchSIM_APDU, radio::iccTransmitApduBasicChannelResponse},
    {RIL_REQUEST_SIM_OPEN_CHANNEL, dispatchString, radio::iccOpenLogicalChannelResponse},
    {RIL_REQUEST_SIM_CLOSE_CHANNEL, dispatchInts, radio::iccCloseLogicalChannelResponse},
    {RIL_REQUEST_SIM_TRANSMIT_APDU_CHANNEL, dispatchSIM_APDU, radio::iccTransmitApduLogicalChannelResponse},
    {RIL_REQUEST_NV_READ_ITEM, dispatchNVReadItem, radio::nvReadItemResponse},
    {RIL_REQUEST_NV_WRITE_ITEM, dispatchNVWriteItem, radio::nvWriteItemResponse},
    {RIL_REQUEST_NV_WRITE_CDMA_PRL, dispatchRaw, radio::nvWriteCdmaPrlResponse},
    {RIL_REQUEST_NV_RESET_CONFIG, dispatchInts, radio::nvResetConfigResponse},
    {RIL_REQUEST_SET_UICC_SUBSCRIPTION, dispatchUiccSubscripton, radio::setUiccSubscriptionResponse},
    {RIL_REQUEST_ALLOW_DATA, dispatchInts, radio::setDataAllowedResponse},
    {RIL_REQUEST_GET_HARDWARE_CONFIG, dispatchVoid, responseHardwareConfig},
    {RIL_REQUEST_SIM_AUTHENTICATION, dispatchSimAuthentication, radio::requestIccSimAuthenticationResponse},
    {RIL_REQUEST_GET_DC_RT_INFO, dispatchVoid, responseDcRtInfo},
    {RIL_REQUEST_SET_DC_RT_INFO_RATE, dispatchInts, responseVoid},
    {RIL_REQUEST_SET_DATA_PROFILE, dispatchDataProfile, responseVoid},
    {RIL_REQUEST_SHUTDOWN, dispatchVoid, radio::requestShutdownResponse},
    {RIL_REQUEST_GET_RADIO_CAPABILITY, dispatchVoid, responseRadioCapability},
    {RIL_REQUEST_SET_RADIO_CAPABILITY, dispatchRadioCapability, responseRadioCapability},
    {RIL_REQUEST_START_LCE, dispatchInts, radio::startLceServiceResponse},
    {RIL_REQUEST_STOP_LCE, dispatchVoid, radio::stopLceServiceResponse},
    {RIL_REQUEST_PULL_LCEDATA, dispatchVoid, responseLceData},
    {RIL_REQUEST_GET_ACTIVITY_INFO, dispatchVoid, radio::getModemActivityInfoResponse},
    {RIL_REQUEST_SET_CARRIER_RESTRICTIONS, dispatchCarrierRestrictions, radio::setAllowedCarriersResponse},
    {RIL_REQUEST_GET_CARRIER_RESTRICTIONS, dispatchVoid, radio::getAllowedCarriersResponse},
