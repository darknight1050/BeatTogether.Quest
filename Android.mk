HOST_NAME ?= master.beattogether.systems
PORT ?= 2328
STATUS_URL ?= "http://master.beattogether.systems/status"
LOCAL_PATH := $(call my-dir)

TARGET_ARCH_ABI := arm64-v8a

include $(CLEAR_VARS)
LOCAL_MODULE := hook
rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))
# Creating prebuilt for dependency: codegen - version: 0.8.1
include $(CLEAR_VARS)
LOCAL_MODULE := codegen_0_8_1
LOCAL_EXPORT_C_INCLUDES := extern/codegen
LOCAL_SRC_FILES := extern/libcodegen_0_8_1.so
LOCAL_CPP_FEATURES += exceptions
include $(PREBUILT_SHARED_LIBRARY)
# Creating prebuilt for dependency: beatsaber-hook - version: 1.3.3
include $(CLEAR_VARS)
LOCAL_MODULE := beatsaber-hook_1_3_3
LOCAL_EXPORT_C_INCLUDES := extern/beatsaber-hook
LOCAL_SRC_FILES := extern/libbeatsaber-hook_1_3_3.so
include $(PREBUILT_SHARED_LIBRARY)
# Creating prebuilt for dependency: modloader - version: 1.1.0
include $(CLEAR_VARS)
LOCAL_MODULE := modloader
LOCAL_EXPORT_C_INCLUDES := extern/modloader
LOCAL_SRC_FILES := extern/libmodloader.so
include $(PREBUILT_SHARED_LIBRARY)

# Creating prebuilt for dependency: songdownloader - version: dev
include $(CLEAR_VARS)
LOCAL_MODULE := songdownloader
LOCAL_EXPORT_C_INCLUDES := extern/songdownloader
LOCAL_SRC_FILES := extern/libsongdownloader.so
include $(PREBUILT_SHARED_LIBRARY)
# Creating prebuilt for dependency: songloader - version: 0.2.5
include $(CLEAR_VARS)
LOCAL_MODULE := songloader
LOCAL_EXPORT_C_INCLUDES := extern/songloader
LOCAL_SRC_FILES := extern/libsongloader.so
include $(PREBUILT_SHARED_LIBRARY)
# Creating prebuilt for dependency: questui - version: 0.6.10
include $(CLEAR_VARS)
LOCAL_MODULE := questui
LOCAL_EXPORT_C_INCLUDES := extern/questui
LOCAL_SRC_FILES := extern/libquestui.so
include $(PREBUILT_SHARED_LIBRARY)
# Creating prebuilt for dependency: custom-types - version: 0.8.3
include $(CLEAR_VARS)
LOCAL_MODULE := custom-types
LOCAL_EXPORT_C_INCLUDES := extern/custom-types
LOCAL_SRC_FILES := extern/libcustom-types.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := beattogether
LOCAL_SRC_FILES += $(call rwildcard,src/,*.cpp)
LOCAL_SRC_FILES += $(call rwildcard,extern/beatsaber-hook/src/inline-hook,*.cpp)
LOCAL_SRC_FILES += $(call rwildcard,extern/beatsaber-hook/src/inline-hook,*.c)
LOCAL_SHARED_LIBRARIES += modloader
LOCAL_SHARED_LIBRARIES += beatsaber-hook_1_3_3
LOCAL_SHARED_LIBRARIES += codegen_0_8_1
LOCAL_SHARED_LIBRARIES += songloader
LOCAL_SHARED_LIBRARIES += songdownloader
LOCAL_SHARED_LIBRARIES += questui
LOCAL_SHARED_LIBRARIES += custom-types
LOCAL_LDLIBS += -llog
LOCAL_CFLAGS += -DID='"BeatTogether"' -DVERSION='"$(VERSION)"' -DHOST_NAME='"$(HOST_NAME)"' -DPORT='$(PORT)' -DSTATUS_URL='$(STATUS_URL)'
LOCAL_CPPFLAGS += -std=c++2a
LOCAL_C_INCLUDES += ./include ./shared ./src ./extern ./extern/libil2cpp/il2cpp/libil2cpp ./extern/codegen/include
include $(BUILD_SHARED_LIBRARY)
