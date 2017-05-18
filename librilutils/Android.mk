# Copyright 2013 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)

PROTO_PATH := $(LOCAL_PATH)/proto

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    librilutils.c \
    record_stream.c \
    proto/sap-api.proto \

LOCAL_C_INCLUDES += \
    external/nanopb-c/ \
    $(LOCAL_PATH)/../include
LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/../include

LOCAL_PROTOC_OPTIMIZE_OPTIONS := -f $(LOCAL_PATH)/proto/sap-api.options
LOCAL_PROTOC_OPTIMIZE_TYPE := nanopb-c-enable_malloc

LOCAL_MODULE:= librilutils

LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_EXPORT_C_INCLUDE_DIRS += $(call local-generated-sources-dir)/proto/

LOCAL_PROTOC_FLAGS := --proto_path=$(PROTO_PATH)

include $(BUILD_SHARED_LIBRARY)


# Create static library for those that want it
# =========================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    librilutils.c \
    record_stream.c \
    proto/sap-api.proto \

LOCAL_C_INCLUDES += \
    external/nanopb-c/ \
    $(LOCAL_PATH)/../include
LOCAL_EXPORT_C_INCLUDE_DIRS := \
    $(LOCAL_PATH)/../include

LOCAL_PROTOC_OPTIMIZE_OPTIONS := -f $(LOCAL_PATH)/proto/sap-api.options
LOCAL_PROTOC_OPTIMIZE_TYPE := nanopb-c-enable_malloc

LOCAL_MODULE:= librilutils_static
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_EXPORT_C_INCLUDE_DIRS += $(call local-generated-sources-dir)/proto/

LOCAL_PROTOC_FLAGS := --proto_path=$(PROTO_PATH)

include $(BUILD_STATIC_LIBRARY)

# Create java protobuf code

include $(CLEAR_VARS)

src_proto := $(LOCAL_PATH)
LOCAL_MODULE := sap-api-java-static
LOCAL_SRC_FILES := proto/sap-api.proto
LOCAL_PROTOC_OPTIMIZE_TYPE := micro

include $(BUILD_STATIC_JAVA_LIBRARY)
