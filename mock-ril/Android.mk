# Copyright 2010 The Android Open Source Project
#
# not currently building V8 for x86 targets

LOCAL_PATH:= $(call my-dir)

# Mock-ril only buid for debug variants
ifneq ($(filter userdebug eng tests, $(TARGET_BUILD_VARIANT)),)

# Directories of source files
src_cpp=src/cpp
src_java=src/java
src_py=src/py
src_js=src/js
src_proto=src/proto
src_generated=src/generated

# Directories of generated source files
gen_src_cpp=$(src_generated)/cpp
gen_src_java=$(src_generated)/java
gen_src_py=$(src_generated)/python
gen_src_desc=$(src_generated)/desc

ifeq ($(TARGET_ARCH),arm)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    $(src_cpp)/ctrl_server.cpp \
    $(src_cpp)/experiments.cpp \
    $(src_cpp)/js_support.cpp \
    $(src_cpp)/mock_ril.cpp \
    $(src_cpp)/node_buffer.cpp \
    $(src_cpp)/node_util.cpp \
    $(src_cpp)/protobuf_v8.cpp \
    $(src_cpp)/responses.cpp \
    $(src_cpp)/requests.cpp \
    $(src_cpp)/util.cpp \
    $(src_cpp)/worker.cpp \
    $(src_cpp)/worker_v8.cpp \
    $(gen_src_cpp)/ril.pb.cpp \
    $(gen_src_cpp)/ctrl.pb.cpp \
    $(gen_src_cpp)/msgheader.pb.cpp


LOCAL_SHARED_LIBRARIES := \
    libz libcutils libutils libril

LOCAL_STATIC_LIBRARIES := \
    libprotobuf-cpp-2.3.0-full libv8

LOCAL_CFLAGS := -D_GNU_SOURCE -UNDEBUG -DGOOGLE_PROTOBUF_NO_RTTI -DRIL_SHLIB

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/$(src_cpp) \
    $(LOCAL_PATH)/$(gen_src_cpp) \
    external/protobuf/src \
    external/v8/include \
    bionic \
    $(KERNEL_HEADERS)

# stlport conflicts with the host stl library
ifneq ($(TARGET_SIMULATOR),true)
LOCAL_SHARED_LIBRARIES += libstlport
LOCAL_C_INCLUDES += external/stlport/stlport
endif

# build shared library but don't require it be prelinked
# __BSD_VISIBLE for htolexx macros.
LOCAL_STRIP_MODULE := true
LOCAL_PRELINK_MODULE := false
LOCAL_LDLIBS += -lpthread
LOCAL_CFLAGS += -DMOCK_RIL -D__BSD_VISIBLE
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE:= libmock_ril

include $(BUILD_SHARED_LIBRARY)

endif # TARGET_ARCH == arm

# Java librilproto
# =======================================================
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := debug
LOCAL_MODULE := librilproto-java

LOCAL_STATIC_JAVA_LIBRARIES := libprotobuf-java-2.3.0-micro

LOCAL_SRC_FILES := $(call all-java-files-under, $(src_java) $(gen_src_java))

include $(BUILD_STATIC_JAVA_LIBRARY)
# =======================================================

endif # TARGET_BUILD_VARIANT
