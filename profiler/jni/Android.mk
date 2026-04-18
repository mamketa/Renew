LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := velfox_profiler
LOCAL_SRC_FILES := \
    ../src/main.cpp \
    ../src/FileIO.cpp \
    ../src/CpuFreq.cpp \
    ../src/DevFreq.cpp \
    ../src/MemoryManager.cpp \
    ../src/SocManager.cpp \
    ../src/Modes.cpp \
    ../src/MediaTek.cpp \
    ../src/Snapdragon.cpp \
    ../src/Exynos.cpp \
    ../src/Tensor.cpp \
    ../src/Unisoc.cpp

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include

LOCAL_CPPFLAGS := \
    -DNDEBUG \
    -O3 \
    -std=c++23 \
    -fPIC \
    -D_GNU_SOURCE \
    -D_FORTIFY_SOURCE=2 \
    -fstack-protector-strong \
    -funwind-tables \
    -ffunction-sections \
    -fdata-sections \
    -flto \
    -Wall \
    -Wextra \
    -Wno-unused-parameter \
    -Wno-unused-variable

LOCAL_LDFLAGS := \
    -flto \
    -Wl,--gc-sections \
    -Wl,-z,relro \
    -Wl,-z,now

LOCAL_LDLIBS += -llog

include $(BUILD_EXECUTABLE)
