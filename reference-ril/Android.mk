# Copyright 2006 The Android Open Source Project

# XXX using libutils for simulator build only...
#
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    reference-ril.c \
    atchannel.c \
    misc.c \
    at_tok.c

LOCAL_SHARED_LIBRARIES := \
    liblog libcutils libutils libril librilutils

LOCAL_STATIC_LIBRARIES := libbase

# for asprinf
LOCAL_CFLAGS := -D_GNU_SOURCE
LOCAL_CFLAGS += -Wall -Wextra -Wno-unused-variable -Wno-unused-function -Werror

LOCAL_C_INCLUDES :=

ifeq ($(TARGET_DEVICE),sooner)
  LOCAL_CFLAGS += -DUSE_TI_COMMANDS
endif

ifeq ($(TARGET_DEVICE),surf)
  LOCAL_CFLAGS += -DPOLL_CALL_STATE -DUSE_QMI
endif

ifeq ($(TARGET_DEVICE),dream)
  LOCAL_CFLAGS += -DPOLL_CALL_STATE -DUSE_QMI
endif

LOCAL_VENDOR_MODULE:= true

ifeq (foo,foo)
  #build shared library
  LOCAL_SHARED_LIBRARIES += \
      libcutils libutils
  LOCAL_CFLAGS += -DRIL_SHLIB
  LOCAL_MODULE:= libreference-ril
  LOCAL_LICENSE_KINDS:= SPDX-license-identifier-Apache-2.0
  LOCAL_LICENSE_CONDITIONS:= notice
  LOCAL_NOTICE_FILE:= $(LOCAL_PATH)/NOTICE
  include $(BUILD_SHARED_LIBRARY)
else
  #build executable
  LOCAL_SHARED_LIBRARIES += \
      libril
  LOCAL_MODULE:= reference-ril
  LOCAL_LICENSE_KINDS:= SPDX-license-identifier-Apache-2.0
  LOCAL_LICENSE_CONDITIONS:= notice
  LOCAL_NOTICE_FILE:= $(LOCAL_PATH)/NOTICE
  include $(BUILD_EXECUTABLE)
endif
