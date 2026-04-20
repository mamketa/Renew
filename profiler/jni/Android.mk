LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE      := velfox_profiler
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
    
LOCAL_SRC_FILES := \
    ../main.c \
    ../src/utility_utils.c \
    ../src/cpu_utils.c \
    ../src//gpu_utils.c \
    ../src/devfreq_utils.c \
    ../src/memory_utils.c \
    ../src/io_utils.c \
    ../src/scheduler_utils.c \
    ../src/modes_utils.c \
    ../src/soc_dispatch.c \
    ../src/velfox_mediatek.c \
    ../src/velfox_snapdragon.c \
    ../src/velfox_exynos.c \
    ../src/velfox_tensor.c \
    ../src/velfox_unisoc.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include

LOCAL_CFLAGS := \
    -O3 \
    -std=c23 \
    -fPIE \
    -fPIC \
    -fvisibility=hidden \
    -ffunction-sections \
    -fdata-sections \
    -flto \
    -fomit-frame-pointer \
    -DNDEBUG \
    -D_GNU_SOURCE \
    -fstack-protector-strong \
    -Wall \
    -fno-plt \
    -fmerge-all-constants \
    -finline-functions

LOCAL_LDFLAGS := \
    -flto \
    -pie \
    -Wl,--gc-sections \
    -Wl,-z,relro \
    -Wl,-z,now \
    -Wl,--as-needed \
    -Wl,--strip-all \
    -Wl,--exclude-libs,ALL

LOCAL_LDLIBS += -llog

LOCAL_STRIP_MODULE := true

include $(BUILD_EXECUTABLE)