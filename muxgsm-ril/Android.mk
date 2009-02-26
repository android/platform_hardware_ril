# hardware/ril/muxgsm-ril/Android.mk
#
# Copyright 2009 Wind River Systems
#

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    muxgsm-ril.c \
    atchannel.c \
    misc.c \
    at_tok.c \
    crc_table.c

LOCAL_SHARED_LIBRARIES := libril libcutils libutils

LOCAL_CFLAGS := -D_GNU_SOURCE -DRIL_SHLIB
LOCAL_C_INCLUDES := $(KERNEL_HEADERS)

LOCAL_LDLIBS += -lpthread
LOCAL_PRELINK_MODULE:= false
LOCAL_MODULE:= libmuxgsm-ril

include $(BUILD_SHARED_LIBRARY)

##
## Extra etc files
##

include $(CLEAR_VARS)

define local_find_etc_files
$(patsubst ./%,%,$(shell cd $(LOCAL_PATH)/etc ; find . -type f -printf "%P\n"))
endef

LOCAL_ETC_DIR  := $(LOCAL_PATH)/etc

copy_from := $(call local_find_etc_files)
copy_to   := $(addprefix $(TARGET_OUT_ETC)/,$(copy_from))
copy_from := $(addprefix $(LOCAL_ETC_DIR),$(copy_from))

$(copy_to) : $(TARGET_OUT_ETC)/% : $(LOCAL_ETC_DIR)/% | $(ACP)
	$(transform-prebuilt-to-target)

ALL_PREBUILT += $(copy_to)
