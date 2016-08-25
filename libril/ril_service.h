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
void iccCardStatusResponse(int32_t slotId, int32_t responseType, int32_t serial, RIL_Errno e,
        void *response, size_t responselen);
void radioStateChanged(int slotId, RIL_RadioState radioState);

}   // namespace android

#endif  // RIL_SERVICE_H