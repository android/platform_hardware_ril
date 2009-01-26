# Copyright 2008 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    reference-cdma-sms.c

LOCAL_SHARED_LIBRARIES := \
	libcutils libutils libril

	# for asprinf
LOCAL_CFLAGS := -D_GNU_SOURCE

LOCAL_C_INCLUDES := $(KERNEL_HEADERS)

LOCAL_SHARED_LIBRARIES += \
  libcutils libutils
LOCAL_LDLIBS += -lpthread
LOCAL_MODULE:= libreference-cdma-sms
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

