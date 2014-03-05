/*
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ANDROID_RIL_MSIM_H
#define ANDROID_RIL_MSIM_H 1

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  RIL_UICC_SUBSCRIPTION_DEACTIVATE = 0,
  RIL_UICC_SUBSCRIPTION_ACTIVATE = 1
} RIL_UiccSubActStatus;

typedef enum {
  RIL_SUBSCRIPTION_1 = 0,
  RIL_SUBSCRIPTION_2 = 1,
  RIL_SUBSCRIPTION_3 = 2
} RIL_SubscriptionType;

typedef struct {
  int   slot;                        /* 0, 1, ... etc. */
  int   app_index;                   /* array subscriptor from applications[RIL_CARD_MAX_APPS] in
                                        RIL_REQUEST_GET_SIM_STATUS */
  RIL_SubscriptionType  sub_type;    /* Indicates subscription 1 or subscription 2 */
  RIL_UiccSubActStatus  act_status;
} RIL_SelectUiccSub;

#ifdef __cplusplus
}
#endif

#endif /*ANDROID_RIL_MSIM_H*/
