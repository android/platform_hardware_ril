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

#ifndef RIL_SERVICE_H
#define RIL_SERVICE_H

#include <telephony/ril.h>
#include <ril_internal.h>

namespace radio {
void registerService(RIL_RadioFunctions *callbacks, android::CommandInfo *commands);
int getIccCardStatusResponse(android::Parcel &p, int slotId, int requestNumber, int responseType,
                            int token, RIL_Errno e, void *response, size_t responselen);

int supplyIccPinForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int supplyIccPukForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int supplyIccPin2ForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                               int responseType, int serial, RIL_Errno e, void *response,
                               size_t responselen);

int supplyIccPuk2ForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                               int responseType, int serial, RIL_Errno e, void *response,
                               size_t responselen);

int changeIccPinForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int changeIccPin2ForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                               int responseType, int serial, RIL_Errno e, void *response,
                               size_t responselen);

int supplyNetworkDepersonalizationResponse(android::Parcel &p, int slotId, int requestNumber,
                                          int responseType, int serial, RIL_Errno e,
                                          void *response, size_t responselen);

int getCurrentCallsResponse(android::Parcel &p, int slotId, int requestNumber,
                           int responseType, int serial, RIL_Errno e, void *response,
                           size_t responselen);

int dialResponse(android::Parcel &p, int slotId, int requestNumber,
                int responseType, int serial, RIL_Errno e, void *response, size_t responselen);

int getIMSIForAppResponse(android::Parcel &p, int slotId, int requestNumber, int responseType,
                         int serial, RIL_Errno e, void *response, size_t responselen);

int hangupConnectionResponse(android::Parcel &p, int slotId, int requestNumber, int responseType,
                            int serial, RIL_Errno e, void *response, size_t responselen);

int hangupWaitingOrBackgroundResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e, void *response,
                                     size_t responselen);

int hangupForegroundResumeBackgroundResponse(android::Parcel &p, int slotId, int requestNumber,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responselen);

int switchWaitingOrHoldingAndActiveResponse(android::Parcel &p, int slotId, int requestNumber,
                                           int responseType, int serial, RIL_Errno e,
                                           void *response, size_t responselen);

int conferenceResponse(android::Parcel &p, int slotId, int requestNumber, int responseType,
                      int serial, RIL_Errno e, void *response, size_t responselen);

int rejectCallResponse(android::Parcel &p, int slotId, int requestNumber, int responseType,
                      int serial, RIL_Errno e, void *response, size_t responselen);

int getLastCallFailCauseResponse(android::Parcel &p, int slotId, int requestNumber,
                                int responseType, int serial, RIL_Errno e, void *response,
                                size_t responselen);

int getSignalStrengthResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responseLen);

int getVoiceRegistrationStateResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e, void *response,
                                     size_t responselen);

int getDataRegistrationStateResponse(android::Parcel &p, int slotId, int requestNumber,
                                    int responseType, int serial, RIL_Errno e, void *response,
                                    size_t responselen);

int getOperatorResponse(android::Parcel &p, int slotId, int requestNumber,
                       int responseType, int serial, RIL_Errno e, void *response,
                       size_t responselen);

int setRadioPowerResponse(android::Parcel &p, int slotId, int requestNumber,
                         int responseType, int serial, RIL_Errno e, void *response,
                         size_t responselen);

int sendDtmfResponse(android::Parcel &p, int slotId, int requestNumber,
                    int responseType, int serial, RIL_Errno e, void *response,
                    size_t responselen);

int sendSmsResponse(android::Parcel &p, int slotId, int requestNumber,
                   int responseType, int serial, RIL_Errno e, void *response,
                   size_t responselen);

int sendSMSExpectMoreResponse(android::Parcel &p, int slotId, int requestNumber,
                             int responseType, int serial, RIL_Errno e, void *response,
                             size_t responselen);

int setupDataCallResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responseLen);

int iccIOForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                       int responseType, int serial, RIL_Errno e, void *response,
                       size_t responselen);

int sendUssdResponse(android::Parcel &p, int slotId, int requestNumber,
                    int responseType, int serial, RIL_Errno e, void *response,
                    size_t responselen);

int cancelPendingUssdResponse(android::Parcel &p, int slotId, int requestNumber,
                             int responseType, int serial, RIL_Errno e, void *response,
                             size_t responselen);

int getClirResponse(android::Parcel &p, int slotId, int requestNumber,
                   int responseType, int serial, RIL_Errno e, void *response, size_t responselen);

int setClirResponse(android::Parcel &p, int slotId, int requestNumber,
                   int responseType, int serial, RIL_Errno e, void *response, size_t responselen);

int getCallForwardStatusResponse(android::Parcel &p, int slotId, int requestNumber,
                                int responseType, int serial, RIL_Errno e, void *response,
                                size_t responselen);

int setCallForwardResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responselen);

int getCallWaitingResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responselen);

int setCallWaitingResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responselen);

int acknowledgeLastIncomingGsmSmsResponse(android::Parcel &p, int slotId, int requestNumber,
                                         int responseType, int serial, RIL_Errno e, void *response,
                                         size_t responselen);

int acceptCallResponse(android::Parcel &p, int slotId, int requestNumber,
                      int responseType, int serial, RIL_Errno e, void *response,
                      size_t responselen);

int deactivateDataCallResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int getFacilityLockForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responselen);

int setFacilityLockForAppResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responselen);

int setBarringPasswordResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int getNetworkSelectionModeResponse(android::Parcel &p, int slotId, int requestNumber,
                                   int responseType, int serial, RIL_Errno e, void *response,
                                   size_t responselen);

int setNetworkSelectionModeAutomaticResponse(android::Parcel &p, int slotId, int requestNumber,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responselen);

int setNetworkSelectionModeManualResponse(android::Parcel &p, int slotId, int requestNumber,
                                         int responseType, int serial, RIL_Errno e, void *response,
                                         size_t responselen);

int getAvailableNetworksResponse(android::Parcel &p, int slotId, int requestNumber,
                                int responseType, int serial, RIL_Errno e, void *response,
                                size_t responselen);

int startDtmfResponse(android::Parcel &p, int slotId, int requestNumber,
                     int responseType, int serial, RIL_Errno e, void *response,
                     size_t responselen);

int stopDtmfResponse(android::Parcel &p, int slotId, int requestNumber,
                    int responseType, int serial, RIL_Errno e, void *response,
                    size_t responselen);

int getBasebandVersionResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int separateConnectionResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int setMuteResponse(android::Parcel &p, int slotId, int requestNumber,
                   int responseType, int serial, RIL_Errno e, void *response,
                   size_t responselen);

int getMuteResponse(android::Parcel &p, int slotId, int requestNumber,
                   int responseType, int serial, RIL_Errno e, void *response,
                   size_t responselen);

int getClipResponse(android::Parcel &p, int slotId, int requestNumber,
                   int responseType, int serial, RIL_Errno e, void *response,
                   size_t responselen);

int getDataCallListResponse(android::Parcel &p, int slotId, int requestNumber,
                            int responseType, int serial, RIL_Errno e,
                            void *response, size_t responseLen);

int sendScreenStateResponse(android::Parcel &p, int slotId, int requestNumber,
                           int responseType, int serial, RIL_Errno e, void *response,
                           size_t responselen);

int setSuppServiceNotificationsResponse(android::Parcel &p, int slotId, int requestNumber,
                                       int responseType, int serial, RIL_Errno e, void *response,
                                       size_t responselen);

int writeSmsToSimResponse(android::Parcel &p, int slotId, int requestNumber,
                         int responseType, int serial, RIL_Errno e, void *response,
                         size_t responselen);

int deleteSmsOnSimResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responselen);

int setBandModeResponse(android::Parcel &p, int slotId, int requestNumber,
                       int responseType, int serial, RIL_Errno e, void *response,
                       size_t responselen);

int getAvailableBandModesResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responselen);

int sendEnvelopeResponse(android::Parcel &p, int slotId, int requestNumber,
                        int responseType, int serial, RIL_Errno e, void *response,
                        size_t responselen);

int sendTerminalResponseToSimResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e, void *response,
                                     size_t responselen);

int handleStkCallSetupRequestFromSimResponse(android::Parcel &p, int slotId, int requestNumber,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responselen);

int explicitCallTransferResponse(android::Parcel &p, int slotId, int requestNumber,
                                int responseType, int serial, RIL_Errno e, void *response,
                                size_t responselen);

int setPreferredNetworkTypeResponse(android::Parcel &p, int slotId, int requestNumber,
                                   int responseType, int serial, RIL_Errno e, void *response,
                                   size_t responselen);

int getPreferredNetworkTypeResponse(android::Parcel &p, int slotId, int requestNumber,
                                   int responseType, int serial, RIL_Errno e, void *response,
                                   size_t responselen);

int getNeighboringCidsResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int setLocationUpdatesResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e, void *response,
                              size_t responselen);

int setCdmaSubscriptionSourceResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e, void *response,
                                     size_t responselen);

int setCdmaRoamingPreferenceResponse(android::Parcel &p, int slotId, int requestNumber,
                                    int responseType, int serial, RIL_Errno e, void *response,
                                    size_t responselen);

int getCdmaRoamingPreferenceResponse(android::Parcel &p, int slotId, int requestNumber,
                                    int responseType, int serial, RIL_Errno e, void *response,
                                    size_t responselen);

int setTTYModeResponse(android::Parcel &p, int slotId, int requestNumber,
                      int responseType, int serial, RIL_Errno e, void *response,
                      size_t responselen);

int getTTYModeResponse(android::Parcel &p, int slotId, int requestNumber,
                      int responseType, int serial, RIL_Errno e, void *response,
                      size_t responselen);

int setPreferredVoicePrivacyResponse(android::Parcel &p, int slotId, int requestNumber,
                                    int responseType, int serial, RIL_Errno e, void *response,
                                    size_t responselen);

int getPreferredVoicePrivacyResponse(android::Parcel &p, int slotId, int requestNumber,
                                    int responseType, int serial, RIL_Errno e, void *response,
                                    size_t responselen);

int sendCDMAFeatureCodeResponse(android::Parcel &p, int slotId, int requestNumber,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responselen);

int sendBurstDtmfResponse(android::Parcel &p, int slotId, int requestNumber,
                         int responseType, int serial, RIL_Errno e, void *response,
                         size_t responselen);

int sendCdmaSmsResponse(android::Parcel &p, int slotId, int requestNumber,
                       int responseType, int serial, RIL_Errno e, void *response,
                       size_t responselen);

int acknowledgeLastIncomingCdmaSmsResponse(android::Parcel &p, int slotId, int requestNumber,
                                          int responseType, int serial, RIL_Errno e, void *response,
                                          size_t responselen);

int getGsmBroadcastConfigResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responselen);

int setGsmBroadcastConfigResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e, void *response,
                                 size_t responselen);

int setGsmBroadcastActivationResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e, void *response,
                                     size_t responselen);

int getCdmaBroadcastConfigResponse(android::Parcel &p, int slotId, int requestNumber,
                                  int responseType, int serial, RIL_Errno e, void *response,
                                  size_t responselen);

int setCdmaBroadcastConfigResponse(android::Parcel &p, int slotId, int requestNumber,
                                  int responseType, int serial, RIL_Errno e, void *response,
                                  size_t responselen);

int setCdmaBroadcastActivationResponse(android::Parcel &p, int slotId, int requestNumber,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responselen);

int getCDMASubscriptionResponse(android::Parcel &p, int slotId, int requestNumber,
                               int responseType, int serial, RIL_Errno e, void *response,
                               size_t responselen);

int writeSmsToRuimResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responselen);

int deleteSmsOnRuimResponse(android::Parcel &p, int slotId, int requestNumber,
                           int responseType, int serial, RIL_Errno e, void *response,
                           size_t responselen);

int getDeviceIdentityResponse(android::Parcel &p, int slotId, int requestNumber,
                             int responseType, int serial, RIL_Errno e, void *response,
                             size_t responselen);

int exitEmergencyCallbackModeResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e, void *response,
                                     size_t responselen);

int getSmscAddressResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e, void *response,
                          size_t responselen);

int setCdmaBroadcastActivationResponse(android::Parcel &p, int slotId, int requestNumber,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responselen);

int setSmscAddressResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responselen);

int reportSmsMemoryStatusResponse(android::Parcel &p, int slotId, int requestNumber,
                                 int responseType, int serial, RIL_Errno e,
                                 void *response, size_t responselen);

int reportStkServiceIsRunningResponse(android::Parcel &p, int slotId, int requestNumber,
                                      int responseType, int serial, RIL_Errno e,
                                      void *response, size_t responseLen);

int getCdmaSubscriptionSourceResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e, void *response,
                                     size_t responselen);

int requestIsimAuthenticationResponse(android::Parcel &p, int slotId, int requestNumber,
                                     int responseType, int serial, RIL_Errno e, void *response,
                                     size_t responselen);

int acknowledgeIncomingGsmSmsWithPduResponse(android::Parcel &p, int slotId, int requestNumber,
                                            int responseType, int serial, RIL_Errno e,
                                            void *response, size_t responselen);

int sendEnvelopeWithStatusResponse(android::Parcel &p, int slotId, int requestNumber,
                                  int responseType, int serial, RIL_Errno e, void *response,
                                  size_t responselen);

int getVoiceRadioTechnologyResponse(android::Parcel &p, int slotId, int requestNumber,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responselen);

int getCellInfoListResponse(android::Parcel &p, int slotId,
                            int requestNumber, int responseType,
                            int serial, RIL_Errno e, void *response,
                            size_t responseLen);

int setCellInfoListRateResponse(android::Parcel &p, int slotId, int requestNumber,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responselen);

int setInitialAttachApnResponse(android::Parcel &p, int slotId, int requestNumber,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responselen);

int getImsRegistrationStateResponse(android::Parcel &p, int slotId, int requestNumber,
                                   int responseType, int serial, RIL_Errno e,
                                   void *response, size_t responselen);

int sendImsSmsResponse(android::Parcel &p, int slotId, int requestNumber, int responseType,
                      int serial, RIL_Errno e, void *response, size_t responselen);

int iccTransmitApduBasicChannelResponse(android::Parcel &p, int slotId, int requestNumber,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responselen);

int iccOpenLogicalChannelResponse(android::Parcel &p, int slotId, int requestNumber,
                                  int responseType, int serial, RIL_Errno e, void *response,
                                  size_t responselen);


int iccCloseLogicalChannelResponse(android::Parcel &p, int slotId, int requestNumber,
                                  int responseType, int serial, RIL_Errno e,
                                  void *response, size_t responselen);

int iccTransmitApduLogicalChannelResponse(android::Parcel &p, int slotId, int requestNumber,
                                         int responseType, int serial, RIL_Errno e,
                                         void *response, size_t responselen);

int nvReadItemResponse(android::Parcel &p, int slotId, int requestNumber,
                      int responseType, int serial, RIL_Errno e,
                      void *response, size_t responselen);


int nvWriteItemResponse(android::Parcel &p, int slotId, int requestNumber,
                       int responseType, int serial, RIL_Errno e,
                       void *response, size_t responselen);

int nvWriteCdmaPrlResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responselen);

int nvResetConfigResponse(android::Parcel &p, int slotId, int requestNumber,
                         int responseType, int serial, RIL_Errno e,
                         void *response, size_t responselen);

int setUiccSubscriptionResponse(android::Parcel &p, int slotId, int requestNumber,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responselen);

int setDataAllowedResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responselen);

int getHardwareConfigResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responseLen);

int requestIccSimAuthenticationResponse(android::Parcel &p, int slotId, int requestNumber,
                                       int responseType, int serial, RIL_Errno e,
                                       void *response, size_t responselen);

int setDataProfileResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responselen);

int requestShutdownResponse(android::Parcel &p, int slotId, int requestNumber,
                           int responseType, int serial, RIL_Errno e,
                           void *response, size_t responselen);

int getRadioCapabilityResponse(android::Parcel &p, int slotId, int requestNumber,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen);

int setRadioCapabilityResponse(android::Parcel &p, int slotId, int requestNumber,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen);

int startLceServiceResponse(android::Parcel &p, int slotId, int requestNumber,
                           int responseType, int serial, RIL_Errno e,
                           void *response, size_t responselen);

int stopLceServiceResponse(android::Parcel &p, int slotId, int requestNumber,
                          int responseType, int serial, RIL_Errno e,
                          void *response, size_t responselen);

int pullLceDataResponse(android::Parcel &p, int slotId, int requestNumber,
                        int responseType, int serial, RIL_Errno e,
                        void *response, size_t responseLen);

int getModemActivityInfoResponse(android::Parcel &p, int slotId, int requestNumber,
                                int responseType, int serial, RIL_Errno e,
                                void *response, size_t responselen);

int setAllowedCarriersResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responselen);

int getAllowedCarriersResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responselen);

int setSimCardPowerResponse(android::Parcel &p, int slotId, int requestNumber,
                              int responseType, int serial, RIL_Errno e,
                              void *response, size_t responselen);

void acknowledgeRequest(int slotId, int serial);

void radioStateChangedInd(int slotId, int indicationType, RIL_RadioState radioState);

int callStateChangedInd(android::Parcel &p, int slotId, int requestNumber, int indType, int token,
                        RIL_Errno e, void *response, size_t responselen);

int networkStateChangedInd(android::Parcel &p, int slotId, int requestNumber, int indType,
                                int token, RIL_Errno e, void *response, size_t responselen);

int newSmsInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
              int token, RIL_Errno e, void *response, size_t responselen);

int newSmsStatusReportInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                          int token, RIL_Errno e, void *response, size_t responselen);

int newSmsOnSimInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                   int token, RIL_Errno e, void *response, size_t responselen);

int onUssdInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
              int token, RIL_Errno e, void *response, size_t responselen);

int nitzTimeReceivedInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                        int token, RIL_Errno e, void *response, size_t responselen);

int currentSignalStrengthInd(android::Parcel &p, int slotId, int requestNumber,
                             int indicationType, int token, RIL_Errno e,
                             void *response, size_t responselen);

int dataCallListChangedInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                           int token, RIL_Errno e, void *response, size_t responselen);

int suppSvcNotifyInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                     int token, RIL_Errno e, void *response, size_t responselen);

int stkSessionEndInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                     int token, RIL_Errno e, void *response, size_t responselen);

int stkProactiveCommandInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                           int token, RIL_Errno e, void *response, size_t responselen);

int stkEventNotifyInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                      int token, RIL_Errno e, void *response, size_t responselen);

int stkCallSetupInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                    int token, RIL_Errno e, void *response, size_t responselen);

int simSmsStorageFullInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                         int token, RIL_Errno e, void *response, size_t responselen);

int simRefreshInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                  int token, RIL_Errno e, void *response, size_t responselen);

int callRingInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                int token, RIL_Errno e, void *response, size_t responselen);

int simStatusChangedInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                        int token, RIL_Errno e, void *response, size_t responselen);

int cdmaNewSmsInd(android::Parcel &p, int slotId, int requestNumber, int indicationType,
                  int token, RIL_Errno e, void *response, size_t responselen);

int newBroadcastSmsInd(android::Parcel &p, int slotId, int requestNumber,
                       int indicationType, int token, RIL_Errno e, void *response,
                       size_t responselen);

int cdmaRuimSmsStorageFullInd(android::Parcel &p, int slotId, int requestNumber,
                              int indicationType, int token, RIL_Errno e, void *response,
                              size_t responselen);

int restrictedStateChangedInd(android::Parcel &p, int slotId, int requestNumber,
                              int indicationType, int token, RIL_Errno e, void *response,
                              size_t responselen);

int enterEmergencyCallbackModeInd(android::Parcel &p, int slotId, int requestNumber,
                                  int indicationType, int token, RIL_Errno e, void *response,
                                  size_t responselen);

int cdmaCallWaitingInd(android::Parcel &p, int slotId, int requestNumber,
                       int indicationType, int token, RIL_Errno e, void *response,
                       size_t responselen);

int cdmaOtaProvisionStatusInd(android::Parcel &p, int slotId, int requestNumber,
                              int indicationType, int token, RIL_Errno e, void *response,
                              size_t responselen);

int cdmaInfoRecInd(android::Parcel &p, int slotId, int requestNumber,
                   int indicationType, int token, RIL_Errno e, void *response,
                   size_t responselen);

int oemHookRawInd(android::Parcel &p, int slotId, int requestNumber,
                  int indicationType, int token, RIL_Errno e, void *response,
                  size_t responselen);

int indicateRingbackToneInd(android::Parcel &p, int slotId, int requestNumber,
                            int indicationType, int token, RIL_Errno e, void *response,
                            size_t responselen);

int resendIncallMuteInd(android::Parcel &p, int slotId, int requestNumber,
                        int indicationType, int token, RIL_Errno e, void *response,
                        size_t responselen);

int cdmaSubscriptionSourceChangedInd(android::Parcel &p, int slotId, int requestNumber,
                                     int indicationType, int token, RIL_Errno e,
                                     void *response, size_t responselen);

int cdmaPrlChangedInd(android::Parcel &p, int slotId, int requestNumber,
                      int indicationType, int token, RIL_Errno e, void *response,
                      size_t responselen);

int exitEmergencyCallbackModeInd(android::Parcel &p, int slotId, int requestNumber,
                                 int indicationType, int token, RIL_Errno e, void *response,
                                 size_t responselen);

int rilConnectedInd(android::Parcel &p, int slotId, int requestNumber,
                    int indicationType, int token, RIL_Errno e, void *response,
                    size_t responselen);

int voiceRadioTechChangedInd(android::Parcel &p, int slotId, int requestNumber,
                             int indicationType, int token, RIL_Errno e, void *response,
                             size_t responselen);

int cellInfoListInd(android::Parcel &p, int slotId, int requestNumber,
                    int indicationType, int token, RIL_Errno e, void *response,
                    size_t responselen);

int imsNetworkStateChangedInd(android::Parcel &p, int slotId, int requestNumber,
                              int indicationType, int token, RIL_Errno e, void *response,
                              size_t responselen);

int subscriptionStatusChangedInd(android::Parcel &p, int slotId, int requestNumber,
                                 int indicationType, int token, RIL_Errno e, void *response,
                                 size_t responselen);

int srvccStateNotifyInd(android::Parcel &p, int slotId, int requestNumber,
                        int indicationType, int token, RIL_Errno e, void *response,
                        size_t responselen);

int hardwareConfigChangedInd(android::Parcel &p, int slotId, int requestNumber,
                             int indicationType, int token, RIL_Errno e, void *response,
                             size_t responselen);

int radioCapabilityIndicationInd(android::Parcel &p, int slotId, int requestNumber,
                                 int indicationType, int token, RIL_Errno e, void *response,
                                 size_t responselen);

int onSupplementaryServiceIndicationInd(android::Parcel &p, int slotId, int requestNumber,
                                        int indicationType, int token, RIL_Errno e,
                                        void *response, size_t responselen);

int stkCallControlAlphaNotifyInd(android::Parcel &p, int slotId, int requestNumber,
                                 int indicationType, int token, RIL_Errno e, void *response,
                                 size_t responselen);

int lceDataInd(android::Parcel &p, int slotId, int requestNumber,
               int indicationType, int token, RIL_Errno e, void *response,
               size_t responselen);

int pcoDataInd(android::Parcel &p, int slotId, int requestNumber,
               int indicationType, int token, RIL_Errno e, void *response,
               size_t responselen);

int modemResetInd(android::Parcel &p, int slotId, int requestNumber,
                  int indicationType, int token, RIL_Errno e, void *response,
                  size_t responselen);

int sendRequestRawResponse(android::Parcel &p, int slotId, int requestNumber,
                           int responseType, int serial, RIL_Errno e,
                           void *response, size_t responseLen);

int sendRequestStringsResponse(android::Parcel &p, int slotId, int requestNumber,
                               int responseType, int serial, RIL_Errno e,
                               void *response, size_t responseLen);

pthread_rwlock_t * getRadioServiceRwlock(int slotId);

}   // namespace radio

#endif  // RIL_SERVICE_H