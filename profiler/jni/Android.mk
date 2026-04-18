LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := velfox_profiler
LOCAL_SRC_FILES := \
    ../main.c \
    ../src/cpufreq_utils.c \
    ../src/devreq_utils.c \
    ../src/modes_utils.c \
    ../src/utility_utils.c \
    ../src/velfox_mediatek.c \
    ../src/velfox_snapdragon.c \
    ../src/velfox_exynos.c \
    ../src/velfox_unisoc.c \
    ../src/velfox_tensor.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include

LOCAL_CFLAGS := \
    -O3 \
    -std=c17 \
    -fPIE \
    -fPIC \
    -fvisibility=hidden \
    -fdebug-prefix-map=$(LOCAL_PATH)=. \
    -DNDEBUG \
    -D_GNU_SOURCE \
    -D_FORTIFY_SOURCE=2 \
    -fstack-protector-strong \
    -ffunction-sections \
    -fdata-sections \
    -flto \
    -Wall \
    -Wextra \
    -Wno-unused-parameter

LOCAL_LDFLAGS := \
    -flto \
    -pie \
    -Wl,--gc-sections \
    -Wl,-z,relro \
    -Wl,-z,now

LOCAL_LDLIBS += -llog

LOCAL_STRIP_MODULE := true

include $(BUILD_EXECUTABLE)