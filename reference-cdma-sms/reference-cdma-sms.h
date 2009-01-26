/* 
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef REFERENCE_CDMA_SMS_H 
#define REFERENCE_CDMA_SMS_H 1

#include <telephony/ril.h>
#include <telephony/ril_cdma_sms.h>

/**
 * TODO T:
 * check if Bearer data subparameter mask values (CDMS_SMA_MASK_*) have to be moved to ril_cdma_sms.h
 * vendor implementation needs to use ril_cdma_decode_sms and ril_cdma_encode_sms as function names
 */

/** Bearer data subparameter mask values: */
// TODO T: use enum from vendor .h
#define WMS_MASK_BD_NULL                 0x00000000
#define WMS_MASK_BD_MSG_ID               0x00000001
#define WMS_MASK_BD_USER_DATA            0x00000002
#define WMS_MASK_BD_USER_RESP            0x00000004
#define WMS_MASK_BD_MC_TIME              0x00000008
#define WMS_MASK_BD_VALID_ABS            0x00000010
#define WMS_MASK_BD_VALID_REL            0x00000020
#define WMS_MASK_BD_DEFER_ABS            0x00000040
#define WMS_MASK_BD_DEFER_REL            0x00000080
#define WMS_MASK_BD_PRIORITY             0x00000100
#define WMS_MASK_BD_PRIVACY              0x00000200
#define WMS_MASK_BD_REPLY_OPTION         0x00000400
#define WMS_MASK_BD_NUM_OF_MSGS          0x00000800
#define WMS_MASK_BD_ALERT                0x00001000
#define WMS_MASK_BD_LANGUAGE             0x00002000
#define WMS_MASK_BD_CALLBACK             0x00004000
#define WMS_MASK_BD_DISPLAY_MODE         0x00008000
#define WMS_MASK_BD_SCPT_DATA            0x00010000
#define WMS_MASK_BD_SCPT_RESULT          0x00020000
#define WMS_MASK_BD_DEPOSIT_INDEX        0x00040000
#define WMS_MASK_BD_DELIVERY_STATUS      0x00080000
#define WMS_MASK_BD_IP_ADDRESS           0x10000000
#define WMS_MASK_BD_RSN_NO_NOTIFY        0x20000000
#define WMS_MASK_BD_OTHER                0x40000000


/** Decode a CDMA SMS Message. */
RIL_Errno wmsts_ril_cdma_decode_sms (
    RIL_CDMA_Encoded_SMS *  encoded_sms,   /* Input */
    RIL_CDMA_SMS_ClientBd * client_bd      /* Output */
);

/** Encode a CDMA SMS Message. */
RIL_Errno wmsts_ril_cdma_encode_sms (
    RIL_CDMA_SMS_ClientBd * client_bd,     /* Input */
    RIL_CDMA_Encoded_SMS *  encoded_sms    /* Output */
);

#endif /*REFERENCE_CDMA_SMS_H*/

