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


#include "reference-cdma-sms.h"

#undef LOG_TAG
#define LOG_TAG "CDMA"
#include <utils/Log.h>

RIL_Errno wmsts_ril_cdma_decode_sms( RIL_CDMA_Encoded_SMS *  encoded_sms,
        RIL_CDMA_SMS_ClientBd * client_bd) {
    LOGE("ril_cdma_decode_sms function not implemented\n");
    return RIL_E_GENERIC_FAILURE;
  }

RIL_Errno wmsts_ril_cdma_encode_sms(RIL_CDMA_SMS_ClientBd * client_bd, 
        RIL_CDMA_Encoded_SMS *  encoded_sms) {

    LOGE("ril_cdma_encode_sms function not implemented\n");
    return RIL_E_GENERIC_FAILURE;
  }
