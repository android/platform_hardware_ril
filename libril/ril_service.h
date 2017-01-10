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
        int responseType, int serial, RIL_Errno e, void *response, size_t responselen);
int supplyIccPukForAppResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responselen);
int supplyIccPin2ForAppResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responselen);
int supplyIccPuk2ForAppResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responselen);
int changeIccPinForAppResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responselen);
int changeIccPin2ForAppResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responselen);
int supplyNetworkDepersonalizationResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responselen);
int getCurrentCallsResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responselen);
int dialResponse(android::Parcel &p, int slotId, int requestNumber,
        int responseType, int serial, RIL_Errno e, void *response, size_t responselen);
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

pthread_rwlock_t * getRadioServiceRwlock(int slotId);

}   // namespace radio

#endif  // RIL_SERVICE_H