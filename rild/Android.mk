# Copyright 2006 The Android Open Source Project

ifndef ENABLE_VENDOR_RIL_SERVICE

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	rild.c

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libdl \
	liblog \
	libril

# Temporary hack for broken vendor RILs.
LOCAL_WHOLE_STATIC_LIBRARIES := \
	librilutils

LOCAL_CFLAGS := -DRIL_SHLIB
LOCAL_CFLAGS += -Wall -Wextra -Werror

ifeq ($(SIM_COUNT), 2)
    LOCAL_CFLAGS += -DANDROID_MULTI_SIM
    LOCAL_CFLAGS += -DANDROID_SIM_COUNT_2
endif

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE:= rild
LOCAL_LICENSE_KINDS:= SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS:= notice
LOCAL_NOTICE_FILE:= $(LOCAL_PATH)/NOTICE
ifeq ($(PRODUCT_COMPATIBLE_PROPERTY),true)
LOCAL_INIT_RC := rild.rc
LOCAL_CFLAGS += -DPRODUCT_COMPATIBLE_PROPERTY
else
LOCAL_INIT_RC := rild.legacy.rc
endif

include $(BUILD_EXECUTABLE)

endif
